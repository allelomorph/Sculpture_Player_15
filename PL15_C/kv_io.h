#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "player_class_Mat.h"

// Public API
namespace kvio
{
    // Start the TCP server (bind/listen). Returns true on success.
    bool start_server(const std::string &host, std::uint16_t port);

    // Poll the socket(s) once and return ALL complete key/value pairs available.
    // Both key and value are strings.
    std::vector<std::pair<std::string, std::string>> poll_kv();

    // Send one key/value to the connected client.
    bool send_kv(const std::string &key, const std::string &val);

    // Apply any received GUI KV updates into VP_In + params
    bool check_and_load_new_kv_from_gui(Video_Player_With_Processing &VP_In,
                                        const std::vector<std::string> &names,
                                        std::vector<int> &params);

    // Dual-player variant (expects keys like "Player0:Gain", "Player1:Gain", etc.)
    bool check_and_load_new_kv_from_gui_dual(const std::vector<std::string> &names,
                                             std::vector<int> &Player_Params0,
                                             std::vector<int> &Player_Params1);

    // Convenience: start server with defaults
    int init_gui(std::string host = "127.0.0.1", std::uint16_t port = 5000);

    // Optional helpers you added (only declare if you implemented them in the .cpp)
    void send_frame_to_gui(int player_frame_location);
    void send_frame_to_gui_mmss(int player_frame_location_frames, int fps);

    void send_duration_to_gui_mmss(int duration_frames, int fps);
    void send_duration_to_gui_mmss_seconds(int duration_seconds);

    void send_one_gui_param(const std::string &name, const int &param);
    int send_all_gui_params(const std::vector<std::string> &names,
                            const std::vector<int> &params);

    void get_all_gui_params();

} // namespace kvio

// Names your GUI uses (defined in the .cpp)
extern const std::vector<std::string> GUI_Player_Names;