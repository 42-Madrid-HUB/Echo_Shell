#pragma once
#include <string>
#include <vector>

namespace norm {

enum class Severity { Info, Warning, Error };

struct Issue {
    std::string file;
    std::size_t line;     // 1-based, 0 if not applicable
    std::string rule;     // e.g., "line-length", "trailing-space"
    std::string message;
    Severity severity;
};

struct Config {
    std::size_t maxLineLength = 80;
    bool allowTabs = false;
    bool requireFinalNewline = true;
    std::string headerPrefix; // e.g., "// 42 " or "/* ************************************************************************** */"
    std::vector<std::string> fileExtensions{".c", ".h", ".cpp", ".hpp", ".cc", ".hh"};
};

class Checker {
public:
    Checker() = default;

    std::vector<Issue> checkFile(const std::string& path, const Config& cfg) const;
    std::vector<Issue> run(const std::string& rootPath, const Config& cfg) const;

    static void reportConsole(const std::vector<Issue>& issues);
};

} // namespace norm
