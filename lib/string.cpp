#include <lib/string.hpp>

#include <cstddef>

namespace kernel {

extern "C" {
    std::size_t strlen(const char* string) {
        std::size_t len = 0;
        for(; *string; string++, len++);
        return len;
    }

    int memcmp(const void * ptr1, const void * ptr2, size_t num) {
        const unsigned char* a = static_cast<const unsigned char*>(ptr1);
        const unsigned char* b = static_cast<const unsigned char*>(ptr2);
        for(std::size_t i=0; i<num; i++) {
            if(a[i] != b[i]) {
                return a[i] - b[i];
            }
        }
        return 0;
    }

    void* memset(void* ptr, int value, size_t num) {
        char* p = reinterpret_cast<char*>(ptr);
        for(size_t i = 0; i<num; i++) {
            p[i] = value;
        }
        return ptr;
    }

    const void* memchr(const void* ptr, int value, size_t num) {
        const char* p = reinterpret_cast<const char*>(ptr);
        for(size_t i = 0; i<num; i++) {
            if(p[i] == value) {
                return &p[i];
            }
        }
        return nullptr;
    }

    extern "C" void* memcpy(void* dest, const void* src, std::size_t count) {
        char* d = static_cast<char*>(dest);
        const char* s = static_cast<const char*>(src);
        for(std::size_t i = 0; i<count; i++) {
            d[i] = s[i];
        }
        return dest;
    }
}

bool isprint(int ch) {
    return ch >= 32 && ch < 127;
}
bool iscntrl(int ch) {
    return ch < 32 || ch == 127;
}
bool isspace(int ch) {
    return (ch >= 9 && ch <= 13) || ch == 32;
}
bool isblank(int ch) {
    return ch == 9 || ch == 32;
}
bool isgraph(int ch) {
    return ch > 32 && ch < 127;
}
bool ispunct(int ch) {
    return (ch >= 33 && ch <=47) || (ch >= 58 && ch <= 64) || (ch >= 91 && ch <= 96) || (ch >= 123 && ch <= 126);
}
bool isalnum(int ch) {
    return isalpha(ch) || isdigit(ch);
}
bool isalpha(int ch) {
    return isupper(ch) || islower(ch);
}
bool isupper(int ch) {
    return ch >= 'A' && ch <= 'Z';
}
bool islower(int ch) {
    return ch >= 'a' && ch <= 'z';
}
bool isdigit(int ch) {
    return ch >= '0' && ch <= '9';
}
bool isxdigit(int ch) {
    return isdigit(ch) || (ch >= 'A' && ch <= 'F');
}

}
