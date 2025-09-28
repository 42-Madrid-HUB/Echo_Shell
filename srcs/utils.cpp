#include <unistd.h>
#include <map>
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype> // for isspace
#include <readline/readline.h>
#include <readline/history.h>
#include <filesystem>
#include <fstream>
#include <sstream>

std::string get_current_dir() {
    char buf[1024];
    getcwd(buf, sizeof(buf));
    return std::string(buf);
}

std::map<std::string, std::string> get_env_map() {
    std::map<std::string, std::string> env;
    extern char **environ;
    for (char **envp = environ; *envp != nullptr; ++envp) {
        std::string entry(*envp);
        size_t pos = entry.find('=');
        if (pos != std::string::npos) {
            env[entry.substr(0, pos)] = entry.substr(pos + 1);
        }
    }
    return env;
}

// Render a template with {{key}} placeholders replaced by vars[key].
// Unknown placeholders are left as-is. Spaces around keys are trimmed.
std::string render_template(const std::string& tpl, const std::map<std::string, std::string>& vars) {
    std::string out;
    out.reserve(tpl.size());
    for (size_t i = 0; i < tpl.size();) {
        if (tpl[i] == '{' && i + 1 < tpl.size() && tpl[i + 1] == '{') {
            size_t end = tpl.find("}}", i + 2);
            if (end != std::string::npos) {
                std::string key = tpl.substr(i + 2, end - (i + 2));
                // trim key
                size_t s = 0; while (s < key.size() && std::isspace(static_cast<unsigned char>(key[s]))) ++s;
                size_t e = key.size(); while (e > s && std::isspace(static_cast<unsigned char>(key[e - 1]))) --e;
                key = key.substr(s, e - s);
                auto it = vars.find(key);
                if (it != vars.end()) out += it->second;
                else out += "{{" + key + "}}";
                i = end + 2;
                continue;
            }
        }
        out += tpl[i++];
    }
    return out;
}

std::string trim(const std::string& s) {
    if (s.empty()) return s;
    size_t i = 0, j = s.size();
    while (i < j && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    while (j > i && std::isspace(static_cast<unsigned char>(s[j - 1]))) --j;
    return s.substr(i, j - i);
}

void clear_screen() {
    // ANSI clear + home
    write(STDOUT_FILENO, "\033[2J\033[H", 7);
}

void sleep_ms(unsigned ms) {
    usleep(ms * 1000);
}

bool read_line(const std::string& prompt, std::string& out) {
    char* in = readline(prompt.c_str());
    if (!in) return false; // EOF (Ctrl+D)
    out.assign(in);
    if (!out.empty()) add_history(out.c_str());
    free(in);
    return true;
}

// New helpers
std::vector<std::string> list_files_recursive(const std::string& root, const std::vector<std::string>& exts) {
    namespace fs = std::filesystem;
    std::vector<std::string> out;
    std::error_code ec;
    fs::path base(root);
    if (!fs::exists(base, ec)) return out;
    auto has_ext = [&](const fs::path& p) {
        if (exts.empty()) return true;
        std::string e = p.extension().string();
        for (const auto& x : exts) {
            if (e == x) return true;
        }
        return false;
    };
    for (fs::recursive_directory_iterator it(base, fs::directory_options::skip_permission_denied, ec), end; it != end; it.increment(ec)) {
        if (ec) continue;
        if (!it->is_regular_file(ec)) continue;
        if (has_ext(it->path())) out.push_back(it->path().string());
    }
    return out;
}

std::vector<std::string> read_file_lines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary);
    if (!ifs.is_open()) return lines;
    std::string line;
    while (std::getline(ifs, line)) {
        // keep potential '\r' for CRLF detection by caller
        lines.push_back(line);
    }
    return lines;
}

std::string read_text_file(const std::string& path) {
    std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary);
    if (!ifs.is_open()) return std::string();
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}