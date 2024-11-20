#include <kernel/basic.hpp>

#include <drivers/watchdog.hpp>
#include <kernel/debug.hpp>

namespace kernel {

[[noreturn]] void reboot() {
    debug::kinfo("Rebooting...");
    driver::watchdog::restart();
}
[[noreturn]] void shutdown() {
    debug::kinfo("Shutting down...");
    driver::watchdog::poweroff();
}
[[noreturn]] void panic(const char* message, std::source_location loc) {
    if(message) {
        debug::kerror(debug::FormatWithLocation("KERNEL PANIC: {}", loc), message);
    }
    else {
        debug::kerror(debug::FormatWithLocation("KERNEL PANIC", loc));
    }
    __asm__ __volatile__("bkpt");
    for(;;);
}

}
