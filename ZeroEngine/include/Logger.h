#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <mutex>

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    void Init(const std::string& filename = "engine.log", LogLevel level = LogLevel::DEBUG) {
        std::lock_guard<std::mutex> lock(mutex_);
        logFile_.open(filename, std::ios::out | std::ios::app);
        minLevel_ = level;
        
        if (!logFile_.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
    }

    void SetLevel(LogLevel level) {
        minLevel_ = level;
    }

    void Log(LogLevel level, const std::string& message, const char* file, int line) {
        if (level < minLevel_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") 
           << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
           << "[" << LevelToString(level) << "] "
           << message;

        if (level >= LogLevel::WARN) {
            ss << " (" << file << ":" << line << ")";
        }

        std::string logLine = ss.str();

        // Console output with colors
        std::cout << GetColorCode(level) << logLine << "\033[0m" << std::endl;

        // File output
        if (logFile_.is_open()) {
            logFile_ << logLine << std::endl;
            logFile_.flush();
        }
    }

    void Shutdown() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    ~Logger() {
        Shutdown();
    }

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string LevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO ";
            case LogLevel::WARN:  return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    const char* GetColorCode(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "\033[37m";  // White
            case LogLevel::DEBUG: return "\033[36m";  // Cyan
            case LogLevel::INFO:  return "\033[32m";  // Green
            case LogLevel::WARN:  return "\033[33m";  // Yellow
            case LogLevel::ERROR: return "\033[31m";  // Red
            case LogLevel::FATAL: return "\033[35m";  // Magenta
            default: return "\033[0m";
        }
    }

    std::ofstream logFile_;
    LogLevel minLevel_ = LogLevel::DEBUG;
    std::mutex mutex_;
};

// Convenience macros
#define LOG_TRACE(msg) Logger::Instance().Log(LogLevel::TRACE, msg, __FILE__, __LINE__)
#define LOG_DEBUG(msg) Logger::Instance().Log(LogLevel::DEBUG, msg, __FILE__, __LINE__)
#define LOG_INFO(msg)  Logger::Instance().Log(LogLevel::INFO, msg, __FILE__, __LINE__)
#define LOG_WARN(msg)  Logger::Instance().Log(LogLevel::WARN, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::Instance().Log(LogLevel::ERROR, msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) Logger::Instance().Log(LogLevel::FATAL, msg, __FILE__, __LINE__)
