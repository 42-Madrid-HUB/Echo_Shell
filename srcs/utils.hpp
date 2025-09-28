#pragma once
#include <string>
#include <map>
#include <vector>

std::string get_current_dir();
std::map<std::string, std::string> get_env_map();

// Template renderer
std::string render_template(const std::string& tpl, const std::map<std::string, std::string>& vars);

// Small helpers reused across the app
std::string trim(const std::string& s);
void clear_screen();
void sleep_ms(unsigned ms);
// Read one line using readline. Returns false on EOF (Ctrl+D). On success, stores into out.
bool read_line(const std::string& prompt, std::string& out);

// File helpers
std::vector<std::string> list_files_recursive(const std::string& root, const std::vector<std::string>& exts);
std::vector<std::string> read_file_lines(const std::string& path);
std::string read_text_file(const std::string& path);
