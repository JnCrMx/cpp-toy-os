#include <lib/format.hpp>
#include <lib/io.hpp>
#include <lib/string.hpp>
#include <kernel/coroutine.hpp>
#include <arch/arm/interrupts.hpp>

#include <climits>
#include <cstdint>
#include <type_traits>

namespace kernel {

namespace detail {
    unsigned int read_w(const char*& str) {
        const char* work = str;
        char c;
        int i = 0;
        while((c = *work) && (c >= '0' && c <= '9')) {
            work++;
            if(i >= (INT_MAX/10)) {
                return 0;
            }
            i = i*10 + (c - '0');
        }
        str = work;
        return i;
    }

    void read_options(const char*& format, format_options& options)
    {
        if (*format == ':') {
            format++;
            [&]{ // lambda, so we can use return
                for(; *format && *format != '}'; format++) {
                    char c = *format;
                    switch(c) {
                        case '+':
                            options.sign = '+';
                            break;
                        case '-':
                            options.sign = '\0';
                            break;
                        case ' ':
                            options.sign = ' ';
                            break;
                        case '0':
                            options.pad = '0';
                            break;
                        case '<':
                            options.justifyLeft = true;
                            break;
                        case '>':
                            options.justifyLeft = false;
                            break;
                        case '#':
                            options.printType = true;
                            break;
                        default:
                            return;
                    }
                }
            }();
            options.width = read_w(format);
            char t = *format;
            if(t != '}') {
                format++;
            }
            switch(t) {
                case 'b':
                    options.type = format_type::binary;
                    break;
                case 'd':
                    options.type = format_type::decimal;
                    break;
                case 'o':
                    options.type = format_type::octal;
                    break;
                case 'x':
                    options.type = format_type::hex;
                    break;
            }
        }
    }

    void aligned::print(ostream& out) const {
        int len = sv.length();

        if(!justifyLeft) {
            for(int i=len; i<width; i++) {
                out.put(pad);
            }
        }
        out.write(sv.data(), sv.size());
        if(justifyLeft) {
            for(int i=len; i<width; i++) {
                out.put(pad);
            }
        }
    }
}
using detail::format_options;
using detail::format_type;
using detail::read_options;

static unsigned int put_type(ostream& out, bool isZero, format_type type, bool print = true) {
    switch(type) {
        case format_type::binary:
            if(print) out << "0b";
            return 2;
        case format_type::decimal:
            return 0;
        case format_type::octal:
            if(isZero) {
                return 0;
            }
            if(print) out << "0";
            return 1;
        case format_type::hex:
            if(print) out << "0x";
            return 2;
    }
    return 0;
}

static unsigned int get_radix(format_type t) {
    switch(t) {
        case format_type::binary: return 2;
        case format_type::decimal: return 10;
        case format_type::octal: return 8;
        case format_type::hex: return 16;
    }
    return 10;
}

static unsigned int put_number(unsigned int value, int radix, char* target)
{
    if(value == 0) {
        target[0] = '0';
        return 1;
    }
    else {
        int pos = 0;
        for(; value > 0; pos++) {
            int digit = (value % radix);
            char c = '0' + digit;
            if(digit >= 10) {
                c = 'a' + (digit - 10);
            }
            target[pos] = c;
            value = value / radix;
        }
        return pos;
    }
}

template<typename T>
static void kprint_integral(ostream& out, format_options& options, T value)
{
    static_assert(std::is_integral_v<T>, "not an integral");
    auto [width, sign, pad, justifyLeft, printType, type] = options;

    if(value < 0) {
        sign = '-';
        value = -value;
    }
    if(sign) {
        if(pad != ' ') {
            out.put(sign);
        }
        width--;
    }
    if(printType) {
        width -= put_type(out, !value, type, pad != ' ');
    }

    char data[8*sizeof(T)];
    int radix = get_radix(type);
    int pos = put_number(value, radix, data);

    if(!justifyLeft) {
        for(int i=pos; i<width; i++) {
            out.put(pad);
        }
    }

    if(pad == ' ') {
        if(sign) out.put(sign);
        if(printType) put_type(out, !value, type);
    }

    for(int i=pos; i > 0; i--) {
        out.put(data[i-1]);
    }

    if(justifyLeft) {
        for(int i=pos; i<width; i++) {
            out.put(' ');
        }
    }
}

void kprint_value(ostream& out, const char*&format, int value)
{
    format_options options{};
    read_options(format, options);
    kprint_integral(out, options, value);
}
void kprint_value(ostream& out, const char*&format, unsigned int value)
{
    format_options options{};
    read_options(format, options);
    kprint_integral(out, options, value);
}
void kprint_value(ostream& out, const char*&format, unsigned long int value)
{
    format_options options{};
    read_options(format, options);
    kprint_integral(out, options, value);
}

void kprint_value(ostream &out, const char *&format, bool value)
{
    format_options options{};
    read_options(format, options);
    out << detail::aligned(value ? "true" : "false", options);
}

void kprint_value(ostream &out, const char *&format, const char* value)
{
    format_options options{};
    read_options(format, options);
    out << detail::aligned(value, options);
}
void kprint_value(ostream &out, const char *&format, std::string_view value)
{
    format_options options{};
    read_options(format, options);
    out << detail::aligned(value, options);
}

void kprint_value(ostream &out, const char *&format, char value)
{
    format_options options{};
    read_options(format, options);

    int len = 1;

    if(!options.justifyLeft) {
        for(int i=len; i<options.width; i++) {
            out.put(options.pad);
        }
    }
    out << value;
    if(options.justifyLeft) {
        for(int i=len; i<options.width; i++) {
            out.put(options.pad);
        }
    }
}

void kprint_value(ostream &out, const char *&format, void* value)
{
    format_options options{};
    options.type = format_type::hex;
    options.printType = true;
    read_options(format, options);

    kprint_integral(out, options, reinterpret_cast<std::uintptr_t>(value));
}
void kprint_value(ostream &out, const char *&format, volatile void* value)
{
    format_options options{};
    options.type = format_type::hex;
    options.printType = true;
    read_options(format, options);

    kprint_integral(out, options, reinterpret_cast<std::uintptr_t>(value));
}

void kprint_value(ostream& out, const char*&, const std::source_location& value) {
    if constexpr(config::log_print_function) {
        kprint(out, "{}:{} in \"{}\"", value.file_name(), value.line(), value.function_name());
    } else {
        kprint(out, "{}:{}", value.file_name(), value.line());
    }
}

void kprint_value(ostream& out, const char*&, const coroutine_info& value) {
    kprint(out, "[\"{}\"{} at {} from {}]", value.name(), value.critical()?" (critical)":"", value.address(), value.location());
}

}
