#pragma once
#include <string>
#include <cstddef>

namespace esh {

class DebugTools {
public:
    // Print a hexdump to logs (INFO)
    static void hexdump(const void* data, std::size_t size, std::size_t width = 16, const char* label = "hexdump");

    // Log environment variables (DEBUG)
    static void logEnvironment();

    // Log current process memory usage (Linux /proc/self/status)
    static void logMemoryUsage();

    // Log a call stack if available (Linux/glibc)
    static void logCallstack(std::size_t maxFrames = 32);

    // Bridge to set global log level quickly
    static void setLogLevel(int levelEnum); // 0=Trace ... 5=Fatal

    // Assertion with logging
    static void assertTrue(bool cond, const std::string& message, const char* file, int line, const char* func);

    // RAII: measure scope time
    class ScopeTimer {
    public:
        explicit ScopeTimer(const std::string& name);
        ~ScopeTimer();
    private:
        std::string _name;
        unsigned long long _startNs;
    };

    // RAII: trace enter/leave
    class ScopeTrace {
    public:
        explicit ScopeTrace(const std::string& name);
        ~ScopeTrace();
    private:
        std::string _name;
    };
};

// Helper macro for assertions with file/line
#define ESH_DEBUG_ASSERT(cond, msg) ::esh::DebugTools::assertTrue((cond), (msg), __FILE__, __LINE__, __func__)

} // namespace esh
