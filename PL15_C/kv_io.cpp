#include "kv_io.h"
#include "player_class_Mat.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>

#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// for mm:ss formatting
#include <sstream>
#include <iomanip>

using std::vector;

// -----------------------------------------------------------------------------
// GUI names (as you already had)
// -----------------------------------------------------------------------------
const vector<std::string> GUI_Player_Names =
{
    "Gain",
    "Black_Level",
    "Color_Gain",
    "Color_Hue",
    "Image_Gamma",
    "H_Shift",
    "Rotate",
    "Speed",
    "Filter_Type",

    "Pause_Toggle",
    "Fast_Forward",
    "Rewind",
    "QUIT"
};

// -----------------------------------------------------------------------------
// File-private state + helpers (NOT in kvio namespace)
// -----------------------------------------------------------------------------
namespace
{
    // ---- Internal state (module-local) ----
    int g_server_fd = -1;
    int g_client_fd = -1;
    std::string g_buf;

    // ---- Helpers ----
    inline std::string trim(const std::string &s)
    {
        size_t start = 0, end = s.size();
        while (start < end && std::isspace(static_cast<unsigned char>(s[start])))
            ++start;
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
            --end;
        return s.substr(start, end - start);
    }

    // Very small parser for {"Key":"Value"} (Value may also be unquoted digits)
    inline bool parse_line(const std::string &raw, std::string &key, std::string &val)
    {
        std::string s = trim(raw);
        if (s.empty() || s.front() != '{' || s.back() != '}')
            return false;

        std::string inner = s.substr(1, s.size() - 2);

        size_t q1 = inner.find('"');
        if (q1 == std::string::npos) return false;
        size_t q2 = inner.find('"', q1 + 1);
        if (q2 == std::string::npos) return false;
        key = inner.substr(q1 + 1, q2 - q1 - 1);

        size_t colon = inner.find(':', q2 + 1);
        if (colon == std::string::npos) return false;

        std::string vtxt = trim(inner.substr(colon + 1));
        if (!vtxt.empty() && vtxt.back() == ',')
            vtxt.pop_back();
        vtxt = trim(vtxt);

        if (!vtxt.empty() && vtxt.front() == '"' && vtxt.back() == '"')
            vtxt = vtxt.substr(1, vtxt.size() - 2);

        val = vtxt;
        return true;
    }

    bool accept_client()
    {
        sockaddr_in cli{};
        socklen_t clen = sizeof(cli);
        int cfd = ::accept(g_server_fd, reinterpret_cast<sockaddr *>(&cli), &clen);
        if (cfd < 0)
        {
            perror("accept");
            return false;
        }
        g_client_fd = cfd;

        char ip[INET_ADDRSTRLEN] = {0};
        ::inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
        std::cout << "Client connected: " << ip << ":" << ntohs(cli.sin_port) << "\n";
        return true;
    }

    // frames -> "m:ss" using fps
    static inline std::string format_mmss_from_frames(int frames, int fps)
    {
        if (fps <= 0) fps = 30;
        if (frames < 0) frames = 0;

        int total_sec = frames / fps;
        int mm = total_sec / 60;
        int ss = total_sec % 60;

        std::ostringstream oss;
        oss << mm << ":" << std::setw(2) << std::setfill('0') << ss;
        return oss.str();
    }

    // seconds -> "m:ss"
    static inline std::string format_mmss_from_seconds(int total_sec)
    {
        if (total_sec < 0) total_sec = 0;
        int mm = total_sec / 60;
        int ss = total_sec % 60;

        std::ostringstream oss;
        oss << mm << ":" << std::setw(2) << std::setfill('0') << ss;
        return oss.str();
    }

    struct Parsed_Panels
    {
        int player;            // 0 or 1
        std::string key;       // e.g. "Gain"
    };

    static Parsed_Panels parse_player_key(const std::string &k)
    {
        auto pos = k.find(':');
        if (pos == std::string::npos)
            return {0, k}; // default to Panel0 for old clients

        std::string player = k.substr(0, pos);
        std::string key    = k.substr(pos + 1);

        int idx = (player == "Player1") ? 1 : 0;
        return {idx, key};
    }

} // anonymous namespace

// -----------------------------------------------------------------------------
// All public API lives in ONE namespace kvio
// -----------------------------------------------------------------------------
namespace kvio
{
    bool start_server(const std::string &host, std::uint16_t port)
    {
        if (g_server_fd != -1)
            return true; // already started

        g_server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (g_server_fd < 0)
        {
            perror("socket");
            return false;
        }

        int opt = 1;
        (void)::setsockopt(g_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
        {
            std::cerr << "inet_pton failed for " << host << "\n";
            ::close(g_server_fd);
            g_server_fd = -1;
            return false;
        }
        if (::bind(g_server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
        {
            perror("bind");
            ::close(g_server_fd);
            g_server_fd = -1;
            return false;
        }
        if (::listen(g_server_fd, 4) < 0)
        {
            perror("listen");
            ::close(g_server_fd);
            g_server_fd = -1;
            return false;
        }

        std::cout << "Listening on " << host << ":" << port << "\n";
        return true;
    }

    std::vector<std::pair<std::string, std::string>> poll_kv()
    {
        std::vector<std::pair<std::string, std::string>> out;

        if (g_server_fd < 0)
            return out;

        // Accept a client if none connected yet (non-blocking accept)
        if (g_client_fd < 0)
        {
            fd_set afds;
            FD_ZERO(&afds);
            FD_SET(g_server_fd, &afds);
            timeval atv{0, 0};
            int ar = ::select(g_server_fd + 1, &afds, nullptr, nullptr, &atv);
            if (ar > 0 && FD_ISSET(g_server_fd, &afds))
                (void)accept_client();
        }

        if (g_client_fd < 0)
            return out;

        // Try reading any new data from client (non-blocking)
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(g_client_fd, &rfds);
        timeval tv{0, 0};
        int rv = ::select(g_client_fd + 1, &rfds, nullptr, nullptr, &tv);
        if (rv > 0 && FD_ISSET(g_client_fd, &rfds))
        {
            char tmp[2048];
            ssize_t n = ::recv(g_client_fd, tmp, sizeof(tmp), 0);
            if (n <= 0)
            {
                std::cout << "Client disconnected.\n";
                ::close(g_client_fd);
                g_client_fd = -1;
                g_buf.clear();
                return out;
            }
            g_buf.append(tmp, tmp + n);
        }

        // Drain all complete lines
        while (true)
        {
            size_t nl = g_buf.find('\n');
            if (nl == std::string::npos)
                break;

            std::string line = g_buf.substr(0, nl);
            g_buf.erase(0, nl + 1);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            std::string key, val;
            if (parse_line(line, key, val))
                out.emplace_back(std::move(key), std::move(val));
            else
                std::cerr << "⚠️ Parse failed: " << line << "\n";
        }

        return out;
    }

    bool send_kv(const std::string &key, const std::string &val)
    {
        if (g_client_fd < 0)
            return false;

        std::string line;
        line.reserve(key.size() + val.size() + 8);
        line += "{\"";
        line += key;
        line += "\":\"";
        for (char c : val)
        {
            if (c == '\\' || c == '"') line += '\\';
            line += c;
        }
        line += "\"}\n";

        // std::cout << "line " << line << std::endl;  // optional
        ssize_t n = ::send(g_client_fd, line.data(), line.size(), MSG_NOSIGNAL);
        return (n == static_cast<ssize_t>(line.size()));
    }

    bool check_and_load_new_kv_from_gui(Video_Player_With_Processing &VP_In,
                                        const std::vector<std::string> &names,
                                        std::vector<int> &params)
    {
        auto kvs = poll_kv();
        if (kvs.empty())
            return false;

        bool changed = false;

        for (auto &kv : kvs)
        {
            std::cout << "RX: " << kv.first << " = " << kv.second << "\n";
            try
            {
                for (std::size_t i = 0; i < names.size(); ++i)
                {
                    if (kv.first == names[i])
                    {
                        if (i < params.size())
                        {
                            int v = std::stoi(kv.second);
                            params[i] = v;
                            VP_In.Player_Params[i] = v;
                            changed = true;
                        }
                        break;
                    }
                }
            }
            catch (...)
            {
                std::cerr << "⚠️ Conversion failed for " << kv.first
                          << " with value '" << kv.second << "'\n";
            }
        }

        return changed;
    }

    void get_all_gui_params()
    {
        send_kv("command", "snapshot");
    }

    void send_one_gui_param(const std::string &name, const int &param)
    {
        send_kv(name, std::to_string(param));
    }

    int send_all_gui_params(const std::vector<std::string> &names,
                            const std::vector<int> &params)
    {
        if (names.size() != params.size())
        {
            std::cerr << "⚠️ Mismatched sizes: names=" << names.size()
                      << " params=" << params.size() << "\n";
            return 0;
        }
        for (std::size_t i = 0; i < params.size(); ++i)
            send_kv(names[i], std::to_string(params[i]));

        return static_cast<int>(params.size());
    }

    // void send_frame_to_gui(int player_frame_location)
    // {
    //     send_kv("Location", std::to_string(player_frame_location));
    // }

    void send_frame_to_gui_mmss(int player_frame_location_frames, int fps)
    {
        send_kv("Location", format_mmss_from_frames(player_frame_location_frames, fps));
    }

    void send_duration_to_gui_mmss(int duration_frames, int fps)
    {
        send_kv("Duration", format_mmss_from_frames(duration_frames, fps));
    }

    // void send_duration_to_gui_mmss_seconds(int duration_seconds)
    // {
    //     send_kv("Duration", format_mmss_from_seconds(duration_seconds));
    // }

    int init_gui(std::string host, uint16_t port)
    {
        return start_server(host, port) ? 0 : 1;
    }

    bool check_and_load_new_kv_from_gui_dual(const std::vector<std::string> &names,
                                             vector<int> &Player_Params0,
                                             vector<int> &Player_Params1)
    {
        auto kvs = poll_kv();
        if (kvs.empty())
            return false;

        bool changed = false;

        for (auto &kv : kvs)
        {
            std::cout << "RX: " << kv.first << " = " << kv.second << "\n";
            try
            {
                Parsed_Panels p = parse_player_key(kv.first);
                int val = std::stoi(kv.second);

                for (std::size_t i = 0; i < names.size(); ++i)
                {
                    if (p.key == names[i])
                    {
                        if (i < Player_Params0.size())
                        {
                            changed = true;
                            if (p.player == 1) Player_Params1[i] = val;
                            else               Player_Params0[i] = val;
                        }
                        break;
                    }
                }
            }
            catch (...)
            {
                std::cerr << "⚠️ Conversion failed for " << kv.first
                          << " with value '" << kv.second << "'\n";
            }
        }

        return changed;
    }

} // namespace kvio