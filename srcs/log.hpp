#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>
#include <sstream>

namespace esh {

class Logger {
public:
    enum class Level {
        Trace = 0,
        Debug,
        Info,
        Warn,
        Error,
        Fatal,
        Off
    };

    static Logger& instance();

    // Configuration (thread-safe)
    void setLevel(Level lvl);
    Level level() const noexcept;
    bool shouldLog(Level lvl) const noexcept;

    void enableConsole(bool on);
    void setConsoleColored(bool on);
    void setUseUTC(bool on);

    // File sink; set truncate=true to start fresh
    bool setFile(const std::string& path, bool truncate = false);
    void clearFile();

    // Size-based rotation; maxBytes=0 disables rotation. maxFiles>=1.
    void setRotation(size_t maxBytes, unsigned maxFiles);

    // Pattern tokens: {time} {level} {tid} {file} {line} {func} {msg}
    // Example: "[{time}] {level} {file}:{line} {msg}"
    void setPattern(const std::string& pattern);

    // Async logging with background worker thread
    void setAsync(bool on);

    // Flush sinks
    void flush();

    // Core logging API
    void log(Level lvl,
             const std::string& msg,
             const char* file,
             int line,
             const char* func);

    // Stream-style builder for ergonomic logging
    class Line {
    public:
        Line(Level lvl, const char* file, int line, const char* func)
        : _lvl(lvl), _file(file), _line(line), _func(func),
          _enabled(Logger::instance().shouldLog(lvl)) {}

        ~Line() {
            if (_enabled) {
                Logger::instance().log(_lvl, _ss.str(), _file, _line, _func);
            }
        }

        template <typename T>
        Line& operator<<(const T& v) {
            if (_enabled) _ss << v;
            return *this;
        }

        // Support manipulators like std::endl
        using Manip = std::ostream& (*)(std::ostream&);
        Line& operator<<(Manip m) {
            if (_enabled) m(_ss);
            return *this;
        }

    private:
        Level _lvl;
        const char* _file;
        int _line;
        const char* _func;
        bool _enabled;
        std::ostringstream _ss;
    };

private:
    Logger(); // use instance()
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    struct Record {
        std::chrono::system_clock::time_point tp;
        Level lvl;
        std::string msg;
        std::string file;
        std::string func;
        int line;
        unsigned long tid;
    };

    // Internal
    void workerLoop();
    void enqueue(Record&& rec);
    void writeRecord(const Record& rec);
    std::string format(const Record& rec, bool forConsole) const;
    static const char* levelName(Level lvl) noexcept;
    static const char* levelColor(Level lvl) noexcept;

    // State
    std::atomic<Level> _level;
    std::atomic<bool> _console{true};
    std::atomic<bool> _color{true};
    std::atomic<bool> _utc{false};

    // File sink
    std::string _filePath;
    size_t _fileSize = 0;
    size_t _rotateBytes = 0;
    unsigned _rotateFiles = 3;
    std::unique_ptr<std::ofstream> _file;
    mutable std::mutex _fileMx;

    // Pattern
    std::string _pattern;

    // Async
    std::atomic<bool> _async{true};
    std::atomic<bool> _stop{false};
    std::thread _worker;
    std::deque<Record> _queue;
    std::mutex _qMx;
    std::condition_variable _qCv;

    // Helpers
    void rotateIfNeededLocked(size_t incomingBytes);
    std::string makeTime(std::chrono::system_clock::time_point tp) const;
    static unsigned long currentTid();
};

// Convenience macros (stream-style)
#define ESH_LOG_TRACE() ::esh::Logger::Line(::esh::Logger::Level::Trace, __FILE__, __LINE__, __func__)
#define ESH_LOG_DEBUG() ::esh::Logger::Line(::esh::Logger::Level::Debug, __FILE__, __LINE__, __func__)
#define ESH_LOG_INFO()  ::esh::Logger::Line(::esh::Logger::Level::Info,  __FILE__, __LINE__, __func__)
#define ESH_LOG_WARN()  ::esh::Logger::Line(::esh::Logger::Level::Warn,  __FILE__, __LINE__, __func__)
#define ESH_LOG_ERROR() ::esh::Logger::Line(::esh::Logger::Level::Error, __FILE__, __LINE__, __func__)
#define ESH_LOG_FATAL() ::esh::Logger::Line(::esh::Logger::Level::Fatal, __FILE__, __LINE__, __func__)

} // namespace esh
