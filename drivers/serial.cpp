#include <drivers/serial.hpp>
#include <kernel/events.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace kernel::driver::serial {

constexpr std::uintptr_t SERIAL_BASE = (0x7E201000 - 0x3F000000);

enum class cr_flags : uint32_t {
    /**
        * UART enable:
        * 0 = UART is disabled. If the UART is
        * disabled in the middle of transmission or
        * reception, it completes the current
        * character before stopping.
        * 1 = the UART is enabled.
        */
    UARTEN = (1<<0),
    LBE    = (1<<7),
    TXE    = (1<<8),
    RXE    = (1<<9),
    RTS    = (1<<11),
    RTSEN  = (1<<14),
    /**
        * CTS hardware flow control enable. If this bit
        * is set to 1, CTS hardware flow control is
        * enabled. Data is only transmitted when the
        * nUARTCTS signal is asserted.
        */
    CTSEN  = (1<<15)
};

enum class fr_flags : uint32_t {
    /**
     * Clear to send. This bit is the complement of
     * the UART clear to send, nUARTCTS,
     * modem status input. That is, the bit is 1
     * when nUARTCTS is LOW.
     */
    CTS = (1<<0),
    /**
     * UART busy. If this bit is set to 1, the UART
     * is busy transmitting data. This bit remains
     * set until the complete byte, including all the
     * stop bits, has been sent from the shift
     * register.
     * This bit is set as soon as the transmit FIFO
     * becomes non-empty, regardless of whether
     * the UART is
     * enabled or not.
     */
    BUSY = (1<<3),
    /**
     * Receive FIFO empty. The meaning of this
     * bit depends on the state of the FEN bit in
     * the UARTLCR_H
     * Register.
     * If the FIFO is disabled, this bit is set when
     * the receive holding register is empty.
     * If the FIFO is enabled, the RXFE bit is set
     * when the receive FIFO is empty.
     */
    RXFE = (1<<4),
    /**
     * Transmit FIFO full. The meaning of this bit
     * depends on the state of the FEN bit in the
     * UARTLCR_ LCRH Register.
     * If the FIFO is disabled, this bit is set when
     * the transmit holding register is full.
     * If the FIFO is enabled, the TXFF bit is set
     * when the transmit FIFO is full.
     */
    TXFF = (1<<5),
};
enum class lcrh_flags : uint32_t {
    FEN = (1<<4),
};
enum class interrupt_flags : uint32_t {
    OE = (1<<10),
    BE = (1<<9),
    PE = (1<<8),
    FE = (1<<7),
    RT = (1<<6),
    TX = (1<<5),
    RX = (1<<4),
    CTSM = (1<<1),
};

struct pl011 {
    // Data Register
    uint32_t dr;
    uint32_t rsrecr;

    uint32_t unused1[4];

    // Flag register
    uint32_t fr;

    uint32_t unused2[1];

    // not in use
    uint32_t ilpr;
    // Integer Baud rate divisor
    uint32_t ibrd;
    // Fractional Baud rate divisor
    uint32_t fbrd;
    // Line Control register
    uint32_t lcrh;
    // Control register
    uint32_t cr;
    // Interupt FIFO Level Select Register
    uint32_t ifls;
    // Interupt Mask Set Clear Register
    uint32_t imsc;
    // Raw Interupt Status Register
    uint32_t ris;
    // Masked Interupt Status Register
    uint32_t mis;
    // Interupt Clear Register
    uint32_t icr;
    // DMA Control Register
    uint32_t dmacr;

    uint32_t unused3[13];

    // Test Control register
    uint32_t itcr;
    // Integration test input reg
    uint32_t itip;
    // Integration test output reg
    uint32_t itop;
    // Test Data reg
    uint32_t tdr;
};
static_assert(offsetof(pl011, dr) == 0x0);
static_assert(offsetof(pl011, rsrecr) == 0x4);
static_assert(offsetof(pl011, fr) == 0x18);
static_assert(offsetof(pl011, ilpr) == 0x20);
static_assert(offsetof(pl011, ibrd) == 0x24);
static_assert(offsetof(pl011, fbrd) == 0x28);
static_assert(offsetof(pl011, lcrh) == 0x2c);
static_assert(offsetof(pl011, cr) == 0x30);
static_assert(offsetof(pl011, ifls) == 0x34);
static_assert(offsetof(pl011, imsc) == 0x38);
static_assert(offsetof(pl011, ris) == 0x3c);
static_assert(offsetof(pl011, mis) == 0x40);
static_assert(offsetof(pl011, icr) == 0x44);
static_assert(offsetof(pl011, dmacr) == 0x48);
static_assert(offsetof(pl011, itcr) == 0x80);
static_assert(offsetof(pl011, itip) == 0x84);
static_assert(offsetof(pl011, itop) == 0x88);
static_assert(offsetof(pl011, tdr) == 0x8c);

static volatile struct pl011 *const uart_controller = reinterpret_cast<struct pl011*>(SERIAL_BASE);

PL011 Serial{};

void PL011::begin([[maybe_unused]] uint32_t baudrate) {
    uart_controller->lcrh &= ~std::to_underlying(lcrh_flags::FEN); // disable FIFO
    uart_controller->imsc |= std::to_underlying(interrupt_flags::RX); // enable RX interrupt
}

ostream& PL011::put(char ch) {
    while(uart_controller->fr & static_cast<uint32_t>(fr_flags::TXFF));

    uart_controller->dr = ch;
    return *this;
}
int PL011::get() {
    while(uart_controller->fr & static_cast<uint32_t>(fr_flags::RXFE));

    uint32_t read = uart_controller->dr;
    return read & 0xff;
}
int PL011::available() const {
    if(uart_controller->fr & static_cast<uint32_t>(fr_flags::RXFE)) {
        return 0;
    }
    return 1;
}

void PL011::handle_interrupt() {
    uint32_t mis = uart_controller->mis;
    uart_controller->icr = uart_controller->mis;
    if(mis & std::to_underlying(interrupt_flags::RX)) {
        while(!(uart_controller->fr & static_cast<uint32_t>(fr_flags::RXFE))) {
            events::main_event_loop.fire_event(events::event{events::type::serial_rx, static_cast<uint32_t>(get())});
        }
    }
}

}

namespace kernel::debug {
    ostream* debug_stream = &driver::serial::Serial;
}
