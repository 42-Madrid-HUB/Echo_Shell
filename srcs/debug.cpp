#include "debug.hpp"
#include "log.hpp"
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

#if defined(__linux__)
#include <execinfo.h>
#include <fstream>
#endif

extern char **environ;

namespace esh {

static unsigned long long now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<unsigned long long>(ts.tv_sec) * 1000000000ull + static_cast<unsigned long long>(ts.tv_nsec);
}

void DebugTools::hexdump(const void* data, std::size_t size, std::size_t width, const char* label) {
    const unsigned char* p = static_cast<const unsigned char*>(data);
    std::ostringstream out;
    out << label << " (" << size << " bytes)\n";
    for (std::size_t i = 0; i < size; i += width) {
        out << std::setw(6) << std::setfill('0') << std::hex << i << "  ";
        // hex
        for (std::size_t j = 0; j < width; ++j) {
            if (i + j < size) {
                out << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << (int)p[i + j] << ' ';
            } else {
                out << "   ";
            }
        }
        out << " ";
        // ascii
        for (std::size_t j = 0; j < width && i + j < size; ++j) {
            unsigned char c = p[i + j];
            out << (std::isprint(c) ? (char)c : '.');
        }
        out << '\n';
    }
    ESH_LOG_INFO() << out.str();
}

void DebugTools::logEnvironment() {
    ESH_LOG_DEBUG() << "Environment dump begin";
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    for (char **envp = ::environ; envp && *envp; ++envp) {
        ESH_LOG_DEBUG() << *envp;
    }
#else
    ESH_LOG_WARN() << "Environment dump not supported on this platform";
#endif
    ESH_LOG_DEBUG() << "Environment dump end";
}

void DebugTools::logMemoryUsage() {
#if defined(__linux__)
    std::ifstream in("/proc/self/status");
    if (!in.is_open()) {
        ESH_LOG_WARN() << "Cannot open /proc/self/status";
        return;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("VmSize:", 0) == 0 || line.rfind("VmRSS:", 0) == 0) {
            ESH_LOG_INFO() << line;
        }
    }
#else
    ESH_LOG_WARN() << "Memory usage info not implemented on this platform";
#endif
}

void DebugTools::logCallstack(std::size_t maxFrames) {
#if defined(__linux__)
    void* addrs[256];
    int n = backtrace(addrs, static_cast<int>(std::min<std::size_t>(maxFrames, 256)));
    char** syms = backtrace_symbols(addrs, n);
    if (!syms) {
        ESH_LOG_WARN() << "backtrace_symbols failed";
        return;
    }
    ESH_LOG_INFO() << "Call stack (" << n << " frames):";
    for (int i = 0; i < n; ++i) {
        ESH_LOG_INFO() << "  " << syms[i];
    }
    free(syms);
#else
    ESH_LOG_WARN() << "Call stack not available on this platform";
#endif
}

void DebugTools::setLogLevel(int levelEnum) {
    using L = esh::Logger::Level;
    L lvl = L::Info;
    switch (levelEnum) {
        case 0: lvl = L::Trace; break;
        case 1: lvl = L::Debug; break;
        case 2: lvl = L::Info;  break;
        case 3: lvl = L::Warn;  break;
        case 4: lvl = L::Error; break;
        case 5: lvl = L::Fatal; break;
        default: break;
    }
    esh::Logger::instance().setLevel(lvl);
    ESH_LOG_INFO() << "Log level set to " << levelEnum;
}

void DebugTools::assertTrue(bool cond, const std::string& message, const char* file, int line, const char* func) {
    if (!cond) {
        std::ostringstream oss;
        oss << "Assertion failed at " << file << ":" << line << " " << func << " | " << message;
        ESH_LOG_ERROR() << oss.str();
        logCallstack();
    }
}

DebugTools::ScopeTimer::ScopeTimer(const std::string& name)
: _name(name), _startNs(now_ns()) {
    ESH_LOG_DEBUG() << "Start timer: " << _name;
}

DebugTools::ScopeTimer::~ScopeTimer() {
    unsigned long long elapsed = now_ns() - _startNs;
    double ms = static_cast<double>(elapsed) / 1e6;
    ESH_LOG_INFO() << "Timer [" << _name << "] " << ms << " ms";
}

DebugTools::ScopeTrace::ScopeTrace(const std::string& name)
: _name(name) {
    ESH_LOG_DEBUG() << "Enter: " << _name;
}

DebugTools::ScopeTrace::~ScopeTrace() {
    ESH_LOG_DEBUG() << "Leave: " << _name;
}

} // namespace esh
