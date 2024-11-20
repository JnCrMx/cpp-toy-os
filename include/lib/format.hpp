#pragma once

#include <lib/io.hpp>
#include <source_location>
#include <string_view>

namespace kernel {

void kprint_value(ostream& out, const char*&format, int value);
void kprint_value(ostream& out, const char*&format, unsigned int value);
void kprint_value(ostream& out, const char*&format, unsigned long int value);
void kprint_value(ostream& out, const char*&format, bool value);
void kprint_value(ostream& out, const char*&format, const char* value);
void kprint_value(ostream& out, const char*&format, std::string_view value);
void kprint_value(ostream& out, const char*&format, char value);
void kprint_value(ostream& out, const char*&format, void* value);
void kprint_value(ostream& out, const char*&format, volatile void* value);
void kprint_value(ostream& out, const char*&format, const std::source_location& value);
void kprint_value(ostream& out, const char*&format, const class coroutine_info& value);

inline void kprint(ostream& out, const char* format)
{
    for(; *format; format++) {
        out.put(*format);
    }
}

template<typename Arg, typename... Args>
void kprint(ostream& out, const char* format, Arg a, Args... args)
{
    for(; *format; format++) {
        if(*format == '{') {
            format++;
            kprint_value(out, format, a);
            kprint(out, format+1, args...);
            return;
        } else {
            out.put(*format);
        }
    }
}

template<typename... Args>
inline void kprintln(ostream& out, const char* format, Args... args) {
    kprint(out, format, args...);
    kprint(out, "\r\n");
}

template<typename T>
ostream& operator<<(ostream& out, T&& t)
{
    const char* fmt = "}";
    kprint_value(out, fmt, t);
    return out;
}

namespace detail {
    enum class format_type
    {
        binary,
        decimal,
        octal,
        hex
    };
    struct format_options {
        int width = 0;
        char sign = '\0';
        char pad = ' ';
        bool justifyLeft = false;
        bool printType = false;
        format_type type = format_type::decimal;
    };

    unsigned int read_w(const char*& str);
    void read_options(const char*& format, format_options& options);

    class aligned {
        std::string_view sv;
        int width;
        bool justifyLeft;
        char pad;

        public:
            aligned(std::string_view sv, const format_options& opts) : sv(sv), width(opts.width), justifyLeft(opts.justifyLeft), pad(opts.pad) {}
            aligned(std::string_view sv, int width = 0, bool justifyLeft = false, char pad = ' ') : sv(sv), width(width), justifyLeft(justifyLeft), pad(pad) {}
            void print(ostream& out) const;
    };
}
inline void kprint_value(ostream& out, const char*&, const detail::aligned& value) {
    value.print(out);
}

}
