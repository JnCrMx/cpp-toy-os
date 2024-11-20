#pragma once
#include <array>

namespace kernel::driver::gpio {

class Pin {
    public:
        /**
            * An enum for all available GPIO functions
            * @see Pin::configure(func)
            */
        enum class func {
            input = 0x0, output = 0x1
        };

        /**
            * Constructs a "new" GPIO pin from a pin number.
            * The newly constructed pin shares its state with all other pins of the same number
            */
        constexpr Pin(int pin) : m_pin(pin) {}

        void configure(func function);
        void on();
        void off();
        void set(bool on_);

        int pin() {
            return m_pin;
        }
    private:
        int m_pin;
};

extern Pin YellowLED1, YellowLED2, GreenLED1, GreenLED2, RedLED;

/**
    * An array containing all LEDs, in order of their pin numbers.
    */
inline constexpr std::array LEDs = {
    &GreenLED1, &YellowLED1, &RedLED, &YellowLED2, &GreenLED2
};

/**
    * Configures all LED GPIOs as outputs.
    */
void configureLEDs();

}
