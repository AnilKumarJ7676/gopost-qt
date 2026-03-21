#include "core/logging/console_log_sink.h"

#include <cstdio>

#ifdef _WIN32
#include <io.h>
#define isatty_check _isatty
#define fileno_check _fileno
#else
#include <unistd.h>
#define isatty_check isatty
#define fileno_check fileno
#endif

namespace gopost::logging {

ConsoleLogSink::ConsoleLogSink()
    : colorsEnabled_(isatty_check(fileno_check(stdout)) != 0)
{
}

void ConsoleLogSink::write(LogLevel level, const QString& formatted) {
    FILE* stream = (level >= LogLevel::Warning) ? stderr : stdout;
    auto utf8 = formatted.toUtf8();

    if (colorsEnabled_) {
        const char* color = "\033[0m";
        switch (level) {
        case LogLevel::Trace:   color = "\033[90m";   break; // gray
        case LogLevel::Debug:   color = "\033[36m";   break; // cyan
        case LogLevel::Info:    color = "\033[32m";   break; // green
        case LogLevel::Warning: color = "\033[33m";   break; // yellow
        case LogLevel::Error:   color = "\033[31m";   break; // red
        case LogLevel::Fatal:   color = "\033[1;31m"; break; // bold red
        }
        std::fprintf(stream, "%s%s\033[0m\n", color, utf8.constData());
    } else {
        std::fprintf(stream, "%s\n", utf8.constData());
    }

    if (level >= LogLevel::Warning)
        std::fflush(stream);
}

} // namespace gopost::logging
