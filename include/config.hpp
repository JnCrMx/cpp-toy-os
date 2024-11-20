#pragma once

#include <cstddef>
#include <cstdint>

namespace kernel {
    enum class log_level {
        trace = 1, debug = 2, info = 3, warn = 4, error = 5
    };
}

namespace kernel::config {

constexpr std::size_t malloc_memory_size = 0x8000;
constexpr std::size_t event_queue_size = 1024;
constexpr uint32_t system_timer_interval = 1000000;

constexpr log_level minimum_log_level = log_level::info;
constexpr bool log_print_function = false;

constexpr std::size_t mode_stack_size = 0x100000;

extern "C" char _end_of_kernel;
const inline uintptr_t end_of_kernel = reinterpret_cast<uintptr_t>(&_end_of_kernel);
const inline struct {
    uintptr_t usr;
    uintptr_t fiq;
    uintptr_t irq;
    uintptr_t svc;
    uintptr_t abt;
    uintptr_t und;
} stackpointers = {
    .usr = end_of_kernel + 2*mode_stack_size,
    .fiq = end_of_kernel + 3*mode_stack_size,
    .irq = end_of_kernel + 4*mode_stack_size,
    .svc = end_of_kernel + 1*mode_stack_size,
    .abt = end_of_kernel + 5*mode_stack_size,
    .und = end_of_kernel + 6*mode_stack_size,
};

constexpr std::size_t thread_count = 32;
constexpr std::size_t thread_stack_size = 0x10000;
constexpr std::size_t idle_thread_stack_size = 0x1000;

}
