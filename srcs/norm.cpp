#include "norm.hpp"
#include "utils.hpp"
#include "log.hpp"
#include <algorithm>
#include <iostream>

namespace norm {

static bool has_ext(const std::string& path, const std::vector<std::string>& exts) {
    for (const auto& e : exts) {
        if (path.size() >= e.size() &&
            path.compare(path.size() - e.size(), e.size(), e) == 0) return true;
    }
    return false;
}

std::vector<Issue> Checker::checkFile(const std::string& path, const Config& cfg) const {
    std::vector<Issue> issues;

    // Read both content and lines
    std::string all = read_text_file(path);
    auto lines = read_file_lines(path);

    // Header check
    if (!cfg.headerPrefix.empty()) {
        if (lines.empty() || lines[0].find(cfg.headerPrefix) != 0) {
            issues.push_back({path, 1, "header-missing", "File header does not start with required prefix", Severity::Warning});
        }
    }

    // Final newline check
    if (cfg.requireFinalNewline) {
        if (all.empty() || all.back() != '\n') {
            issues.push_back({path, 0, "final-newline", "File does not end with a newline", Severity::Warning});
        }
    }

    // Per-line checks
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string& ln = lines[i];

        // CRLF detection
        if (!ln.empty() && ln.back() == '\r') {
            issues.push_back({path, i + 1, "crlf", "Windows CRLF line ending detected", Severity::Warning});
        }

        // Tabs
        if (!cfg.allowTabs && ln.find('\t') != std::string::npos) {
            issues.push_back({path, i + 1, "tabs", "Tab character is not allowed", Severity::Error});
        }

        // Trailing whitespace
        if (!ln.empty()) {
            std::size_t j = ln.size();
            while (j > 0 && (ln[j - 1] == ' ' || ln[j - 1] == '\t' || ln[j - 1] == '\r')) --j;
            if (j < ln.size()) {
                issues.push_back({path, i + 1, "trailing-space", "Trailing whitespace", Severity::Warning});
            }
        }

        // Line length
        std::size_t visualLen = ln.size();
        if (visualLen > cfg.maxLineLength) {
            issues.push_back({path, i + 1, "line-length", "Line exceeds max length of " + std::to_string(cfg.maxLineLength), Severity::Warning});
        }
    }

    return issues;
}

std::vector<Issue> Checker::run(const std::string& rootPath, const Config& cfg) const {
    std::vector<Issue> allIssues;
    auto files = list_files_recursive(rootPath, cfg.fileExtensions);
    for (const auto& f : files) {
        if (!has_ext(f, cfg.fileExtensions)) continue;
        auto issues = checkFile(f, cfg);
        allIssues.insert(allIssues.end(), issues.begin(), issues.end());
    }
    esh::Logger::instance().log(esh::Logger::Level::Info, "Norm check completed on " + std::to_string(files.size()) + " files", __FILE__, __LINE__, __func__);
    return allIssues;
}

void Checker::reportConsole(const std::vector<Issue>& issues) {
    std::size_t nInfo = 0, nWarn = 0, nErr = 0;
    for (const auto& is : issues) {
        const char* sev = is.severity == Severity::Error ? "ERROR" : (is.severity == Severity::Warning ? "WARN " : "INFO ");
        std::cout << sev << " " << is.file;
        if (is.line) std::cout << ":" << is.line;
        std::cout << " [" << is.rule << "] " << is.message << "\n";
        if (is.severity == Severity::Error) ++nErr;
        else if (is.severity == Severity::Warning) ++nWarn;
        else ++nInfo;
    }
    std::cout << "Summary: " << nErr << " error(s), " << nWarn << " warning(s), " << nInfo << " info.\n";
}

} // namespace norm
