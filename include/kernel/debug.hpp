#pragma once

#include <lib/io.hpp>
#include <lib/format.hpp>
#include <config.hpp>

#include <source_location>

namespace kernel::debug {
    extern ostream* debug_stream;

    template<typename... Args>
    inline void kprint(const char* format, Args... args) {
        kprint(*debug_stream, format, args...);
    }
    template<typename... Args>
    inline void kprintln(const char* format, Args... args) {
        kprintln(*debug_stream, format, args...);
    }

    // taken from https://stackoverflow.com/a/66402319
    struct FormatWithLocation {
        const char* value;
        std::source_location loc;

        constexpr FormatWithLocation(const char* s,
                        const std::source_location& l = std::source_location::current())
            : value(s), loc(l) {}
    };

    constexpr const char* log_level_name(log_level level) {
        switch(level) {
            case log_level::trace: return "trace";
            case log_level::debug: return "debug";
            case log_level::info:  return "info";
            case log_level::warn:  return "warn";
            case log_level::error: return "error";
        }
        return "error";
    }
    // see https://en.wikipedia.org/wiki/ANSI_escape_code#Colors for the colors
    constexpr const char* log_level_color(log_level level) {
        switch(level) {
            case log_level::trace: return "\033[0;90m";
            case log_level::debug: return "\033[0;96m";
            case log_level::info:  return "\033[1;36m";
            case log_level::warn:  return "\033[1;33m";
            case log_level::error: return "\033[1;31m";
        }
        return "\033[0;31m";
    }

    template<typename... Args>
    inline void klog(log_level level, const FormatWithLocation& format, Args... args) {
        if(level < config::minimum_log_level) {
            return;
        }
        if constexpr (config::log_print_function) {
            kprint("[{}{:<5}\033[0m] (\033[0;90m{}:{:<3} in \"{}\"\033[0m): ",
                log_level_color(level), log_level_name(level),
                format.loc.file_name(), static_cast<int>(format.loc.line()), format.loc.function_name());
        }
        else {
            kprint("[{}{:<5}\033[0m] (\033[0;90m{}:{:<3}\033[0m): ",
                log_level_color(level), log_level_name(level),
                format.loc.file_name(), static_cast<int>(format.loc.line()));
        }
        kprintln(format.value, args...);
    }

    template<typename... Args>
    inline void kinfo(const FormatWithLocation& format, Args... args) {
        klog(log_level::info, format, args...);
    }
    template<typename... Args>
    inline void kwarn(const FormatWithLocation& format, Args... args) {
        klog(log_level::warn, format, args...);
    }
    template<typename... Args>
    inline void kerror(const FormatWithLocation& format, Args... args) {
        klog(log_level::error, format, args...);
    }
    template<typename... Args>
    inline void kdebug(const FormatWithLocation& format, Args... args) {
        klog(log_level::debug, format, args...);
    }
    template<typename... Args>
    inline void ktrace(const FormatWithLocation& format, Args... args) {
        klog(log_level::trace, format, args...);
    }
}
