#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace gopost {

enum class LogLevel : int {
    Error = 0,
    Warn = 1,
    Info = 2,
    Debug = 3,
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void set_level(LogLevel level) { level_ = level; }

    void log(LogLevel level, const char* fmt, ...) {
        if (static_cast<int>(level) > static_cast<int>(level_)) return;

        std::lock_guard<std::mutex> lock(mutex_);

        const char* prefix = "";
        switch (level) {
            case LogLevel::Error: prefix = "[ERROR]"; break;
            case LogLevel::Warn:  prefix = "[WARN] "; break;
            case LogLevel::Info:  prefix = "[INFO] "; break;
            case LogLevel::Debug: prefix = "[DEBUG]"; break;
        }

        fprintf(stderr, "%s ", prefix);

        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);

        fprintf(stderr, "\n");
    }

private:
    Logger() = default;
    LogLevel level_ = LogLevel::Info;
    std::mutex mutex_;
};

}  // namespace gopost
