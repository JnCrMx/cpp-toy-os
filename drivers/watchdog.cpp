#include <drivers/watchdog.hpp>

#include <cstddef>
#include <cstdint>

namespace kernel::driver::watchdog {

// see https://github.com/torvalds/linux/blob/05d3ef8bba77c1b5f98d941d8b2d4aeab8118ef1/arch/arm/boot/dts/broadcom/bcm2835-common.dtsi#L63
constexpr std::uintptr_t WATCHDOG_BASE = (0x7E100000 - 0x3F000000);

// see https://github.com/torvalds/linux/blob/05d3ef8bba77c1b5f98d941d8b2d4aeab8118ef1/drivers/watchdog/bcm2835_wdt.c#L23-L25
struct watchdog {
    uint32_t padding[7];
    uint32_t rstc;
    uint32_t rsts;
    uint32_t wdog;
};
static_assert(offsetof(watchdog, rstc) == 0x1c);
static_assert(offsetof(watchdog, rsts) == 0x20);
static_assert(offsetof(watchdog, wdog) == 0x24);

// see https://github.com/torvalds/linux/blob/05d3ef8bba77c1b5f98d941d8b2d4aeab8118ef1/drivers/watchdog/bcm2835_wdt.c#L27
constexpr static uint32_t PM_PASSWORD = 0x5a000000;
// see https://github.com/torvalds/linux/blob/05d3ef8bba77c1b5f98d941d8b2d4aeab8118ef1/drivers/watchdog/bcm2835_wdt.c#L29-L34
constexpr static uint32_t PM_RSTC_WRCFG_CLR = 0xffffffcf;
constexpr static uint32_t PM_RSTC_WRCFG_FULL_RESET = 0x00000020;
// see https://github.com/torvalds/linux/blob/05d3ef8bba77c1b5f98d941d8b2d4aeab8118ef1/drivers/watchdog/bcm2835_wdt.c#L36-L41
constexpr static uint32_t PM_RSTS_RASPBERRYPI_HALT = 0x555;

static volatile struct watchdog *const watchdog_interface = reinterpret_cast<struct watchdog*>(WATCHDOG_BASE);

volatile static uint32_t no_undefined_behaviour = 0;
[[noreturn]] void restart() {
    // see https://github.com/torvalds/linux/blob/05d3ef8bba77c1b5f98d941d8b2d4aeab8118ef1/drivers/watchdog/bcm2835_wdt.c#L100-L123
    watchdog_interface->wdog = 10 | PM_PASSWORD;
    uint32_t val = watchdog_interface->rstc;
    val &= PM_RSTC_WRCFG_CLR;
    val |= PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET;
    watchdog_interface->rstc = val;

    for(;;) {
        no_undefined_behaviour = no_undefined_behaviour + 1; // we need this, so our loop is not undefined behaviour
    }
}

void poweroff() {
    // see https://github.com/torvalds/linux/blob/05d3ef8bba77c1b5f98d941d8b2d4aeab8118ef1/drivers/watchdog/bcm2835_wdt.c#L147-L168
    uint32_t val = watchdog_interface->rsts;
    val |= PM_PASSWORD | PM_RSTS_RASPBERRYPI_HALT;
    watchdog_interface->rsts = val;

    restart();
}

}
