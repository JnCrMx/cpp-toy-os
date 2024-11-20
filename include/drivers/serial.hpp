#pragma once

#include <cstdint>

#include <lib/io.hpp>
#include <kernel/async.hpp>
#include <kernel/events.hpp>

namespace kernel::driver::serial {

class PL011 : public ostream, public istream, public async_istream {
    public:
        constexpr PL011() = default;
        ~PL011() = default;

        void begin(uint32_t baudrate = 0);

        ostream& put(char ch) override;
        int get() override;
        using istream::get;

        int available() const override;
        events::awaiter co_get() override {
            return events::awaiter{events::type::serial_rx};
        }
        void handle_interrupt();

        void operator delete([[maybe_unused]] PL011* p, std::destroying_delete_t) {}
};

extern PL011 Serial;

}
