#include <kernel/basic.hpp>

namespace std {
    void terminate() noexcept {
        kernel::panic("std::terminate() called");
    }
}

extern "C" {

[[noreturn]] void abort() {
    kernel::panic("abort() called");
}

void*  __dso_handle = static_cast<void*>(&__dso_handle);
int __aeabi_atexit([[maybe_unused]] void *arg, [[maybe_unused]] void (*func) (void *), [[maybe_unused]] void *d) {
    return 0;
}
int __cxa_atexit(void (*func) (void *), void * arg, void * dso_handle) {
    return 0;
}

void __aeabi_memclr(void *s, size_t n) {
    char* p = static_cast<char*>(s);
    for(unsigned int i=0; i<n; i++) {
        p[i] = 0;
    }
}
void __aeabi_memclr4(void *s, size_t n) {
    return __aeabi_memclr(s, n);
}
void __aeabi_memclr8(void *s, size_t n) {
    return __aeabi_memclr(s, n);
}

void* __aeabi_memcpy(void* dst, const void* src, size_t n) {
    char* d = static_cast<char*>(dst);
    const char* s = static_cast<const char*>(src);
    for(unsigned int i=0; i<n; i++) {
        d[i] = s[i];
    }
    return dst;
}
void* __aeabi_memcpy4(void* dst, const void* src, size_t n) {
    return __aeabi_memcpy(dst, src, n);
}

}
