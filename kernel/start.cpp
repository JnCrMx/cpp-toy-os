#include <config.hpp>
#include <arch/arm/cpu.hpp>
#include <arch/arm/interrupts.hpp>
#include <drivers/gpio.hpp>
#include <drivers/serial.hpp>
#include <drivers/timer.hpp>
#include <drivers/interrupt_controller.hpp>
#include <cstdint>
#include <kernel/basic.hpp>
#include <kernel/images.hpp>
#include <kernel/debug.hpp>
#include <kernel/memory.hpp>
#include <kernel/coroutine.hpp>
#include <kernel/events.hpp>
#include <kernel/supervisor.hpp>
#include <lib/format.hpp>
#include <lib/string.hpp>
#include <lib/bitfield.hpp>

#include <kernel/threads.hpp>

// not freestanding yet (coping for C++26), but they work thanks to the courtesy of libstdc++
#include <span>
#include <string_view>

namespace kernel {

volatile unsigned int counter = 0;

void increment_counter() {
    counter = counter + 1;
}

coroutine<bool> terminal(std::span<char> buffer, const char* prompt = "$ ") {
    using driver::serial::Serial;

    Serial << prompt;

    if(buffer.size() == 0) {
        co_return false;
    }
    size_t i;
    bool okay = false;
    for(i = 0; i < buffer.size()-1;) {
        char c = buffer[i] = co_await Serial.co_get();
        switch(c) {
            case '\r': // enter
            case '\n': // enter (when feeding QEMU from stdin)
                okay = true;
                Serial << "\r\n";
                break;
            case 0x7f: // backspace
            case 0x08: // backspace (in QEMU gui)
                buffer[i] = '\0';
                if(i > 0) {
                    Serial.put('\b');
                    Serial.put(' ');
                    Serial.put('\b');
                    i--;
                }
                break;
            default:
                if(isprint(c)) {
                    Serial.put(c);
                    i++;
                } else {
                    buffer[i] = '\0';
                }
                break;
        }
        if(okay) {
            break;
        }
    }
    buffer[i] = '\0';
    co_return okay;
}

void start_kernel() {
    using namespace driver::gpio;
    using namespace driver::serial;
    using debug::kprintln;

    Serial.begin();
    debug::kinfo("Kernel starting...");

    debug::kdebug("Configured stack pointers:");
    debug::kdebug("  sp_usr = {}", reinterpret_cast<void*>(config::stackpointers.usr));
    debug::kdebug("  sp_fiq = {}", reinterpret_cast<void*>(config::stackpointers.fiq));
    debug::kdebug("  sp_irq = {}", reinterpret_cast<void*>(config::stackpointers.irq));
    debug::kdebug("  sp_svc = {}", reinterpret_cast<void*>(config::stackpointers.svc));
    debug::kdebug("  sp_abt = {}", reinterpret_cast<void*>(config::stackpointers.abt));
    debug::kdebug("  sp_und = {}", reinterpret_cast<void*>(config::stackpointers.und));

    using cpu::interrupts::interrupt_type;
    cpu::interrupts::set_handler(
        {interrupt_type::data_abort, interrupt_type::prefetch_abort, interrupt_type::undefined_instruction},
        &handle_exception, nullptr);
    cpu::interrupts::set_handler(interrupt_type::software_interrupt, &handle_svc, nullptr);

    debug::kdebug("Interrupt vector table is at {}.", &cpu::interrupts::_ivt);
    cpu::interrupts::setup_vector_table();
    cpu::interrupts::enable();

    events::configure();

    debug::kdebug("Preparing {} threads.", config::thread_count);
    threads::init();

    malloc_init();
    debug::kdebug("Initialized malloc, we have {} bytes of dynamically allocatable memory.", config::malloc_memory_size);

    configureLEDs();
    debug::kdebug("Configured {} LEDs.", LEDs.size());

    YellowLED2.on();
    debug::kdebug("Turned on the 2nd yellow LED (at pin {}).", YellowLED2.pin());

    debug::kdebug("Testing the serial console:");
    kprintln("Hallo {} |{:#016b}|{:<5}|!", "Welt", 0xff, false);
    kprintln("Hallo Welt |{:#016b}|{:<5}|!", 0x333, true);
    Serial << "Test: " << 1234 << "\r\n\r\n";

    kprintln("Some addresses and sizes:");
    kprintln("- &{:<15} = {:08} ({:3})", "YellowLED2", &YellowLED2, sizeof(YellowLED2));
    kprintln("- &{:<15} = {:08} ({:3})", "Serial", &Serial, sizeof(Serial));
    Serial << "\r\n";

    kprintln("Some sizes:");
    kprintln("- sizeof(promise_base_base) = {}", sizeof(detail::promise_base<void>));
    kprintln("- sizeof(promise_base)      = {}", sizeof(detail::promise_base<void>));
    kprintln("- sizeof(promise<void>)     = {}", sizeof(promise<void>));
    kprintln("- sizeof(promise<int>)      = {}", sizeof(promise<int>));

    events::event_loop test{};
    test.submit_coroutine([](events::event_loop* test, coroutine_name = "test")->coroutine<void> {
        co_await events::yield();

        auto handle = co_await get_coroutine_handle();
        kprintln("Hello from a coroutine {} on event loop {}!", get_coroutine_info(handle), get_event_loop(handle));

        co_await events::move_to_event_loop(&events::main_event_loop);

        kprintln("Hello from a coroutine {} on event loop {}!", get_coroutine_info(handle), get_event_loop(handle));
        co_await events::awaiter(events::type::system_timer);
        kprintln("Hello from a coroutine {} on event loop {}!", get_coroutine_info(handle), get_event_loop(handle));

        co_await events::move_to_event_loop(test);

        kprintln("Hello from a coroutine {} on event loop {}!", get_coroutine_info(handle), get_event_loop(handle));
        co_return;
    }(&test));
    {
        int d = 8;
        thread([c = 7, d, &test](int a, int b){
            debug::kinfo("Hello from a thread! I captured {} {} and got as arguments {} {}.", a, b, c, d);
            test.run();
        }, 5, 6).detach();
    }

    debug::kinfo("Kernel started. Entering event loop.");

    bool debug_mode = false;
    events::main_event_loop.submit_coroutine([&debug_mode](coroutine_name = "input debug")->coroutine<void> {
        for(;;) {
            int c = co_await Serial.co_get();
            if(!debug_mode) {
                continue;
            }
            if(isprint(c)) {
                kprintln("Es wurde folgender Charakter eingegeben:  '{}', In Hexadezimal: {:02x}, In Dezimal: {:08}, In Binär: {:08b}, In Oktal: {:04o}",
                    static_cast<char>(c), c, c, c, c);
            } else {
                kprintln("Es wurde folgender Charakter eingegeben: {:#04x}, In Hexadezimal: {:02x}, In Dezimal: {:08}, In Binär: {:08b}, In Oktal: {:04o}",
                    c, c, c, c, c);
            }
        }
    }());
    events::main_event_loop.submit_coroutine([&debug_mode](events::event_loop* test, coroutine_name = "terminal")->coroutine<void> {
        for(;;) {
            std::array<char, 256> line;
            bool okay = co_await terminal(line, "kernel@localhost:/# ");
            if(!okay) {
                debug::kwarn("Line too long, please keep it to {} characters.", line.size());
            }
            std::string_view sv(line.data());
            if(sv == "hello") {
                kprintln("world");
            }
            else if(sv == "keqing") {
                char c;
                for(const char* ptr = images::keqingheart; (c=*ptr); ptr++) {
                    if(c == '\n') {
                        Serial << "\r\n";
                    } else {
                        Serial << c;
                    }
                }
            }
            else if(sv == "debug") {
                debug_mode = !debug_mode;
                kprintln("Debug mode is {}.", debug_mode?"on":"off");
            }
            else if(sv == "stats") {
                kprintln("Kernel statistics:");
                kprintln("  Memory statistics:");
                const auto& stats = malloc_stats();
                kprintln("    memory_total     = {}", stats.memory_total);
                kprintln("    memory_allocated = {}", stats.memory_allocated);
                kprintln("    memory_overhead  = {}", stats.memory_overhead());
                kprintln("    memory_used      = {}", stats.memory_used());
                kprintln("    memory_free      = {}", stats.memory_free());
                kprintln("    num_allocations  = {}", stats.num_allocations);
                kprintln("    num_blocks       = {}", stats.num_blocks);
                kprintln("    block_overhead   = {}", stats.block_overhead);
            }
            else if(sv.starts_with("malloc ")) {
                sv.remove_prefix(std::char_traits<char>::length("malloc "));
                std::size_t size = string_to_integral<std::size_t>(sv).value_or(100);
                kprintln("Allocated {} bytes of memory: {}", size, malloc(size));
            }
            else if(sv.starts_with("free ")) {
                sv.remove_prefix(std::char_traits<char>::length("free "));
                uintptr_t mem = string_to_integral<uintptr_t>(sv).value_or(reinterpret_cast<uintptr_t>(nullptr));
                void* ptr = reinterpret_cast<void*>(mem);
                kprintln("Freeing allocated memory: {}", ptr);
                free(ptr);
            }
            else if(sv.starts_with("led ")) {
                sv.remove_prefix(std::char_traits<char>::length("led "));
                bool on;
                if(sv.ends_with(" on")) {
                    on = true;
                    sv.remove_suffix(std::char_traits<char>::length(" on"));
                }
                else if(sv.ends_with(" off")) {
                    on = false;
                    sv.remove_suffix(std::char_traits<char>::length(" off"));
                }
                else {
                    kprintln("Usage: led n on/off");
                    continue;
                }
                auto ledRes = string_to_integral<int>(sv);
                if(!ledRes) {
                    kprintln("Cannot parse LED number: \"{}\"", sv);
                    continue;
                }
                int led = ledRes.value();
                if(led < 1 || led > static_cast<int>(LEDs.size())) {
                    kprintln("LED number {} is not between {} and {}.", led, 1, LEDs.size());
                    continue;
                }
                LEDs[led-1]->set(on);
                kprintln("Turned LED {} {}.", led, on?"on":"off");
            }
            else if(sv == "whoami") {
                kprintln("I am {}!", get_coroutine_info(co_await get_coroutine_handle()));
            }
            else if(sv == "trap") {
                kprintln("Before trap");
                // __builtin_trap is [[noreturn]] so we cannot use it here because we want to test continuation
                __asm__ __volatile__("udf #0");
                kprintln("After trap");
            }
            else if(sv == "breakpoint") {
                kprintln("Before breakpoint");
                __asm__ __volatile__("bkpt");
                kprintln("After breakpoint");
            }
            else if(sv == "syscall") {
                kprintln("Before syscall");
                __asm__ __volatile__("svc #0");
                kprintln("After syscall");
            }
            else if(sv == "unaligned") {
                kprintln("Before unaligned memory access");
                uint32_t address = 0x1;
                __asm__ __volatile__("ldr r0, [%0]" : : "r"(address) : "r0");
                kprintln("After unaligned memory access");
            }
            else if(sv == "reboot" || sv == "restart") {
                reboot();
            }
            else if(sv == "shutdown" || sv == "poweroff") {
                shutdown();
            }
            else if(sv == "panic") {
                panic("\"panic\" command used");
            }
            else if(sv == "move") {
                co_await events::move_to_event_loop(test);
                kprintln("Moved to test event loop {}", get_event_loop(co_await get_coroutine_handle()));
                for(int i = 0; i < 10; i++) {
                    debug::kprint(".");
                    for(int j=0; j<2500; j++) {
                        co_await events::awaiter(events::type::tick);
                    }
                }
                kprintln("");
                co_await events::move_to_event_loop(&events::main_event_loop);
                kprintln("Moved back to main event loop {}", get_event_loop(co_await get_coroutine_handle()));
            }
            else if(sv == "help") {
                kprintln("Available commands:");
                kprintln("hello          - print \"world\"");
                kprintln("keqing         - show a picture of Keqing");
                kprintln("debug          - toggle debug mode");
                kprintln("stats          - show (memory) stats");
                kprintln("malloc <n>     - allocate n bytes of dynamic memory");
                kprintln("free <p>       - free the memory at pointer p");
                kprintln("led <n> on|off - turn LED n on or off");
                kprintln("whoami         - print the name of the current coroutine");
                kprintln("trap           - trigger an undefined instruction exception");
                kprintln("breakpoint     - trigger a prefetch abort exception");
                kprintln("syscall        - trigger a software interrupt exception");
                kprintln("unaligned      - trigger a data abort exception");
                kprintln("reboot         - reboot the system");
                kprintln("restart        - reboot the system");
                kprintln("shutdown       - shut the system down");
                kprintln("poweroff       - shut the system down");
                kprintln("panic          - cause a kernel panic");
                kprintln("move           - move around event loops");
                kprintln("help           - display this help message");
            }
            else if(sv.empty()) {

            }
            else {
                kprintln("Unknown command: {}", sv);
            }
        }
    }(&test));
    events::main_event_loop.submit_coroutine([&debug_mode](coroutine_name = "timer")->coroutine<void> {
        while(true) {
            co_await events::awaiter(events::type::system_timer);
            if(debug_mode) {
                debug::kprint("!");
            }
        }
        co_return;
    }());
    events::main_event_loop.run();
}

}

extern "C" void start_kernel(){
    kernel::start_kernel();
}
