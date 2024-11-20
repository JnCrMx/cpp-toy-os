#include <drivers/gpio.hpp>

#include <cstdint>
#include <tuple>

namespace kernel::driver::gpio {

constexpr std::uintptr_t GPIO_BASE = (0x7E200000 - 0x3F000000);
constexpr unsigned int GPF_BITS = 3;

namespace pins {
    constexpr unsigned int GREEN_1 = 4;
    constexpr unsigned int YELLOW_LED_1 = 5;
    constexpr unsigned int RED_1 = 6;
    constexpr unsigned int YELLOW_LED_2 = 7;
    constexpr unsigned int GREEN_2 = 8;
}

struct gpio {
    // mabye we can use std::bitset here?
    unsigned int func[6];
    unsigned int unused0;
    unsigned int set[2];
    unsigned int unused1;
    unsigned int clr[2];
};
static volatile struct gpio *const gpio_port = reinterpret_cast<struct gpio*>(GPIO_BASE);


static constexpr std::tuple<unsigned int, unsigned int> index(unsigned int pin, unsigned int bits)
{
    return {0, pin*bits};
}

void Pin::configure(func function) {
    auto [i, o] = index(m_pin, GPF_BITS);
    constexpr uint32_t all_pin_bits = ((1<<GPF_BITS)-1);
    gpio_port->func[i] &= ~(all_pin_bits << o); // clear all bits related to this pin first
    gpio_port->func[i] |= static_cast<unsigned int>(function) << o;
}
void Pin::on() {
    auto [i, o] = index(m_pin, 1);
    gpio_port->set[i] = 1 << o;
}
void Pin::off() {
    auto [i, o] = index(m_pin, 1);
    gpio_port->clr[i] = 1 << o;
}
void Pin::set(bool on_) {
    if(on_) {
        on();
    } else {
        off();
    }
}

Pin GreenLED1(pins::GREEN_1);
Pin GreenLED2(pins::GREEN_2);
Pin YellowLED1(pins::YELLOW_LED_1);
Pin YellowLED2(pins::YELLOW_LED_2);
Pin RedLED(pins::RED_1);

void configureLEDs() {
    for(auto& led : LEDs) {
        led->configure(Pin::func::output);
    }
}

}
