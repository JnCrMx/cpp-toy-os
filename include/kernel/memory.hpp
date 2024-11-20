#pragma once

#include <cstddef>

namespace kernel {

struct memory_statistics {
    std::size_t memory_total{};
    std::size_t memory_allocated{};
    inline std::size_t memory_overhead() const {
        return num_blocks * block_overhead;
    }
    std::size_t num_allocations{};
    std::size_t num_blocks{};
    std::size_t block_overhead{};

    inline std::size_t memory_used() const {
        return memory_allocated + memory_overhead();
    }
    inline std::size_t memory_free() const {
        return memory_total - memory_used();
    }
};
const memory_statistics& malloc_stats();

void malloc_init();
void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void free(void* ptr);

}

void* operator new(std::size_t size);
void operator delete(void* ptr) noexcept;
void operator delete(void* ptr, size_t size) noexcept;

extern "C" void* memset(void* ptr, int value, size_t num);
