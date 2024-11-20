#pragma once

#include <cstdint>

namespace kernel::driver::interrupts {
    enum class interrupt_source : uint32_t {
        sys_timer0 = 0,
        sys_timer1 = 1,
        sys_timer2 = 2,
        sys_timer3 = 3,
        aux = 29,
        i2c_spi_slv = 43,
        pwa0 = 45,
        pwa1 = 46,
        smi = 48,
        gpio0 = 49,
        gpio1 = 50,
        gpio2 = 51,
        gpio3 = 52,
        i2c = 53,
        spi = 54,
        pcm = 55,
        uart = 57,
        emmc = 62,
    };
    void enable_source(interrupt_source);
    void disable_source(interrupt_source);
    bool check_interrupt(interrupt_source);
}
