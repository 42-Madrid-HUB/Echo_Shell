#include "log.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

namespace esh {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger()
    : _level(Level::Info) {
    _pattern = R"([{time}] {level} {tid} {file}:{line} {func} | {msg})";
    _color = isatty(STDERR_FILENO);
    _worker = std::thread(&Logger::workerLoop, this);
}

Logger::~Logger() {
    _stop = true;
    _qCv.notify_all();
    if (_worker.joinable()) {
        _worker.join();
    }
    flush();
}

void Logger::setLevel(Level lvl) {
    _level.store(lvl, std::memory_order_relaxed);
}

Logger::Level Logger::level() const noexcept {
    return _level.load(std::memory_order_relaxed);
}

bool Logger::shouldLog(Level lvl) const noexcept {
    Level current = _level.load(std::memory_order_relaxed);
    return current != Level::Off && lvl >= current;
}

void Logger::enableConsole(bool on) {
    _console.store(on, std::memory_order_relaxed);
}

void Logger::setConsoleColored(bool on) {
    _color.store(on, std::memory_order_relaxed);
}

void Logger::setUseUTC(bool on) {
    _utc.store(on, std::memory_order_relaxed);
}

bool Logger::setFile(const std::string& path, bool truncate) {
    std::lock_guard<std::mutex> lock(_fileMx);
    try {
        _filePath = path;
        _file.reset(new std::ofstream());
        std::ios_base::openmode mode = std::ios::out | std::ios::app;
        if (truncate) {
            mode = std::ios::out | std::ios::trunc;
        }
        _file->open(path.c_str(), mode);
        if (!_file->is_open()) {
            _file.reset();
            return false;
        }
        _file->seekp(0, std::ios::end);
        _fileSize = static_cast<size_t>(_file->tellp());
        return true;
    } catch (...) {
        _file.reset();
        return false;
    }
}

void Logger::clearFile() {
    std::lock_guard<std::mutex> lock(_fileMx);
    _file.reset();
    _filePath.clear();
    _fileSize = 0;
}

void Logger::setRotation(size_t maxBytes, unsigned maxFiles) {
    std::lock_guard<std::mutex> lock(_fileMx);
    _rotateBytes = maxBytes;
    _rotateFiles = std::max(1u, maxFiles);
}

void Logger::setPattern(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(_fileMx);
    _pattern = pattern;
}

void Logger::setAsync(bool on) {
    _async.store(on, std::memory_order_relaxed);
}

void Logger::flush() {
    if (_console.load()) {
        std::cerr.flush();
    }
    std::lock_guard<std::mutex> lock(_fileMx);
    if (_file && _file->is_open()) {
        _file->flush();
    }
}

unsigned long Logger::currentTid() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return static_cast<unsigned long>(
        std::hash<std::string>{}(oss.str()));
}

const char* Logger::levelName(Level lvl) noexcept {
    switch (lvl) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO ";
        case Level::Warn:  return "WARN ";
        case Level::Error: return "ERROR";
        case Level::Fatal: return "FATAL";
        default:           return "OFF  ";
    }
}

const char* Logger::levelColor(Level lvl) noexcept {
    switch (lvl) {
        case Level::Trace: return "\033[90m";
        case Level::Debug: return "\033[36m";
        case Level::Info:  return "\033[32m";
        case Level::Warn:  return "\033[33m";
        case Level::Error: return "\033[31m";
        case Level::Fatal: return "\033[41;97m";
        default:           return "\033[0m";
    }
}

std::string Logger::makeTime(
    std::chrono::system_clock::time_point tp) const {
    std::time_t raw = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#ifdef __linux__
    if (_utc.load()) {
        gmtime_r(&raw, &tm);
    } else {
        localtime_r(&raw, &tm);
    }
#else
    tm = _utc.load() ? *std::gmtime(&raw) : *std::localtime(&raw);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}

std::string Logger::format(const Record& rec, bool forConsole) const {
    std::string out = _pattern;
    auto replaceAll = [](std::string& s,
                         const std::string& a,
                         const std::string& b) {
        size_t pos = 0;
        while ((pos = s.find(a, pos)) != std::string::npos) {
            s.replace(pos, a.size(), b);
            pos += b.size();
        }
    };

    replaceAll(out, "{time}", makeTime(rec.tp));
    replaceAll(out, "{level}", levelName(rec.lvl));
    replaceAll(out, "{tid}", std::to_string(rec.tid));
    replaceAll(out, "{file}", rec.file);
    replaceAll(out, "{line}", std::to_string(rec.line));
    replaceAll(out, "{func}", rec.func);
    replaceAll(out, "{msg}", rec.msg);

    if (forConsole && _color.load()) {
        std::ostringstream colored;
        colored << levelColor(rec.lvl) << out << "\033[0m";
        return colored.str();
    }
    return out;
}

void Logger::rotateIfNeededLocked(size_t incomingBytes)
{
    if (!_file || !_file->is_open() || _rotateBytes == 0)
        return;
    if (_fileSize + incomingBytes <= _rotateBytes)
        return;
    _file->close();
    auto	rotated = [&](unsigned idx)
	{
        return _filePath + "." + std::to_string(idx);
	};
    std::remove(rotated(_rotateFiles - 1).c_str());
    for (int i = static_cast<int>(_rotateFiles) - 1; i >= 2; --i)
	{
        std::remove(rotated(i).c_str());
        std::rename(rotated(i - 1).c_str(), rotated(i).c_str());
    }
    std::remove(rotated(1).c_str());
    std::rename(_filePath.c_str(), rotated(1).c_str());

    _file.reset(new std::ofstream());
    _file->open(_filePath.c_str(), std::ios::out | std::ios::trunc);
    _fileSize = 0;
}

void Logger::writeRecord(const Record& rec) {
    const std::string lineConsole = format(rec, true);
    const std::string lineFile = format(rec, false);

    if (_console.load()) {
        std::cerr << lineConsole << '\n';
    }

    std::lock_guard<std::mutex> lock(_fileMx);
    if (_file && _file->is_open()) {
        rotateIfNeededLocked(lineFile.size() + 1);
        (*_file) << lineFile << '\n';
        _fileSize += lineFile.size() + 1;
    }
}

void Logger::enqueue(Record&& rec) {
    if (_async.load()) {
        {
            std::lock_guard<std::mutex> lock(_qMx);
            _queue.emplace_back(std::move(rec));
        }
        _qCv.notify_one();
    } else {
        writeRecord(rec);
    }
}

void Logger::workerLoop() {
    while (!_stop.load())
	{
        std::unique_lock<std::mutex> lock(_qMx);
        _qCv.wait(lock, [&] { return (_stop.load() || !_queue.empty());});
        if (_stop.load() && _queue.empty())
            break;
        Record rec = std::move(_queue.front());
        _queue.pop_front();
        lock.unlock();
        writeRecord(rec);
    }

    for (;;) {
        std::unique_lock<std::mutex> lock(_qMx);
        if (_queue.empty()) {
            break;
        }
        Record rec = std::move(_queue.front());
        _queue.pop_front();
        lock.unlock();
        writeRecord(rec);
    }
}

void Logger::log(Level lvl,
                const std::string& msg,
                const char* file,
                int line,
                const char* func) {
    if (!shouldLog(lvl)) {
        return;
    }

    Record rec;
    rec.tp = std::chrono::system_clock::now();
    rec.lvl = lvl;
    rec.msg = msg;
    rec.file = file ? file : "";
    rec.func = func ? func : "";
    rec.line = line;
    rec.tid = currentTid();

    enqueue(std::move(rec));

    if (lvl == Level::Fatal) {
        flush();
    }
}

} // namespace esh
