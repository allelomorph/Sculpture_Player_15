// fs_utils.cpp
#include "fs_utils.h"
#include <filesystem>
#include <system_error>
namespace fs = std::filesystem;

#include <unistd.h>   // fork, execlp
#include <sys/wait.h> // waitpid
#include <signal.h>   // kill, SIGTERM, SIGKILL



// ---- Drop-in replacement ----
inline Path_Parsed Parse_Filename_Path(const std::string& Full_Path_In)
{
    Path_Parsed Path{};
    Path.errors = -1;

    if (Full_Path_In.empty()) return Path;

    fs::path p(Full_Path_In);

    // Path_Only (with trailing separator if non-empty, like your original)
    if (p.has_parent_path()) {
        Path.Path_Only = p.parent_path().string();
        if (!Path.Path_Only.empty() && Path.Path_Only.back() != fs::path::preferred_separator)
            Path.Path_Only.push_back(fs::path::preferred_separator);
    } else {
        Path.Path_Only.clear();
    }

    // Filename, stem, extension (extension without leading dot)
    Path.Filename         = p.filename().string();
    Path.Filename_No_Ext  = p.stem().string();

    Path.Ext = p.has_extension() ? p.extension().string() : std::string{};
    if (!Path.Ext.empty() && Path.Ext.front() == '.') Path.Ext.erase(0, 1);

    Path.errors = 0;
    return Path;
}

// Finds a file anywhere in the initial_path or any sub folder
inline std::string Find_File(const std::string& initial_path, const std::string& filename)
{
    std::error_code ec;
    if (!fs::exists(initial_path, ec) || !fs::is_directory(initial_path, ec))
        return "";

    // Be tolerant of unreadable dirs; skip permission errors instead of throwing
    fs::directory_options opts = fs::directory_options::skip_permission_denied;

    for (fs::recursive_directory_iterator it(initial_path, opts, ec), end;
         it != end; it.increment(ec))
    {
        if (ec) continue;                       // skip entries that error out
        const fs::directory_entry& de = *it;
        if (!de.is_regular_file(ec)) continue;  // only compare files
        if (de.path().filename() == filename)
            return de.path().string();
    }
    return "";
}


pid_t py_pid;

void start_external_gui(void)
{
    py_pid = fork();
    if (py_pid == 0)
    {
        execlp("python3", "python3", "../../GUI15_PY.py", NULL);
    }
}

void stop_external_gui(void)
{
    if (py_pid > 0)
        kill(py_pid, SIGTERM);
}