#pragma once

#include <cstddef>
#include <string_view>
#include <type_traits>
#include <expected>

namespace kernel {

extern "C" std::size_t strlen(const char* string);
extern "C" int memcmp(const void * ptr1, const void * ptr2, size_t num);
extern "C" void* memset(void* ptr, int value, size_t num);
extern "C" const void* memchr(const void* ptr, int value, size_t num);
extern "C" void* memcpy(void* dest, const void* src, std::size_t count);

bool iscntrl(int ch);
bool isprint(int ch);
bool isspace(int ch);
bool isblank(int ch);
bool isgraph(int ch);
bool ispunct(int ch);
bool isalnum(int ch);
bool isalpha(int ch);
bool isupper(int ch);
bool islower(int ch);
bool isdigit(int ch);
bool isxdigit(int ch);

enum class parse_error {
    illegal_base,
    invalid_character,
    empty,
};

template<typename T>
std::expected<T, parse_error> string_to_integral(std::string_view sv, int base = -1) {
    static_assert(std::is_integral_v<T>, "type is not an integral type");
    if(base < -1 || base > 36) {
        return std::unexpected(parse_error::illegal_base);
    }

    sv.remove_prefix(sv.find_first_not_of(" "));
    if(sv.empty()) return std::unexpected(parse_error::empty);

    bool negative = false;
    if constexpr (std::is_signed_v<T>) {
        if(sv[0] == '-') {
            negative = true;
            sv.remove_prefix(1);
            if(sv.empty()) return std::unexpected(parse_error::empty);
        }
    }
    if(base == -1) {
        if(sv.starts_with("0x") || sv.starts_with("0X")) {
            base = 16;
            sv.remove_prefix(2);
        }
        else if(sv.starts_with("0b")) {
            base = 2;
            sv.remove_prefix(2);
        }
        else {
            base = 10;
        }
    }
    if(sv.empty()) return std::unexpected(parse_error::empty);

    T value{};
    for(char c : sv) {
        bool is_digit = (c >= '0' && c <= ('0'+std::min(10, base)-1));
        bool is_xdigit = (c >= 'a' && c <= ('a'+std::min(26, base-10)-1));
        bool is_big_xdigit = (c >= 'A' && c <= ('A'+std::min(26, base-10)-1));
        if(!is_digit && !is_xdigit && !is_big_xdigit) {
            return std::unexpected(parse_error::invalid_character);
        }
        if(is_digit) {
            value = value*base + (c - '0');
        } else if(is_xdigit) {
            value = value*base + (10 + (c - 'a'));
        } else if(is_big_xdigit) {
            value = value*base + (10 + (c - 'A'));
        }
    }

    return negative ? -value : value;
}

}
