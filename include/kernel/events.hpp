#pragma once

#include <coroutine>
#include <cstdint>
#include <type_traits>

#include <arch/arm/interrupts.hpp>
#include <kernel/debug.hpp>
#include <kernel/basic.hpp>
#include <kernel/coroutine.hpp>
#include <lib/queue.hpp>

namespace kernel::events {

enum class type {
    tick,
    serial_rx,
    system_timer,
    EVENT_TYPE_COUNT
};

struct event {
    enum type type;
    uint32_t data;
};

class awaiter;
class yield;
namespace detail {
    cpu::interrupts::interrupt_result process_interrupt(cpu::interrupts::interrupt_context&, void*);
}

void configure();
void run_main_event_loop();

class event_loop {
    public:
        constexpr event_loop() = default;

        [[noreturn]] void operator()() {
            run();
        }
        [[noreturn]] void run();
        void step();

        void fire_event(event&& event);

        /**
        * Submits a coroutine to the event loop.
        * The coroutine will automatically be destroyed once it exits.
        * This function may resume the coroutine up until the first co_await.
        */
        template<typename T>
        bool submit_coroutine(coroutine<T>&& coro) {
            if(!coro) {
                debug::kerror("Tried to submit an empty coroutine to the event loop: {}", coro.address());
                return false;
            }

            current_coroutine = coro;
            set_event_loop(coro, this);

            coro.resume();
            if(coro.done()) {
                debug::kwarn("Coroutine {} done after first resume", get_coroutine_info(coro));
            }
            return true;
        }
        std::coroutine_handle<> current_coroutine = nullptr;
    private:
        unsigned int counter = 0;

        awaiter* event_awaiters[static_cast<uint32_t>(type::EVENT_TYPE_COUNT)]{};
        queue<yield> yield_queue{};

        event event_queue[config::event_queue_size]{};
        std::size_t read_pos = 0;
        std::size_t write_pos = 0;

        void process_events();

        [[nodiscard("Use the return value to build a linked list")]] class awaiter* register_event_handler(type type, awaiter* awaiter);
        void yield_coroutine(yield* awaiter);

        friend class awaiter;
        friend class yield;
};
inline constinit event_loop main_event_loop{};

class awaiter {
    type type_;
    uint32_t result{};
    std::coroutine_handle<> handle = nullptr;
    awaiter* next = nullptr;

public:
    /**
     * Constructs an awaiter to wait for an event of a specified type.
     */
    awaiter(type type_) : type_(type_) {
    }

    [[nodiscard]] bool await_ready() const noexcept {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle) noexcept {
        auto loop = get_event_loop(handle);
        debug::ktrace("Coroutine {} is waiting for event {} on event loop {}, with next = {}",
            get_coroutine_info(handle), static_cast<std::underlying_type_t<type>>(type_), loop, static_cast<void*>(next));
        if(!loop) {
            panic("Event loop is null");
        }

        this->handle = handle;
        this->next = loop->register_event_handler(type_, this);
        loop->current_coroutine = nullptr;
        return true;
    }

    uint32_t await_resume() const noexcept {
        return result;
    }
private:
    void complete(uint32_t result) {
        if(this->next) {
            debug::ktrace("Forwarding event to next = {} before completing this = {}",
                static_cast<void*>(next), static_cast<void*>(this));
            this->next->complete(result);
        }
        this->result = result;
        debug::ktrace("Resuming coroutine {} after event {} with result {}",
            get_coroutine_info(this->handle), static_cast<std::underlying_type_t<type>>(type_), result);

        auto loop = get_event_loop(this->handle);
        if(!loop) {
            panic("Event loop is null");
        }
        loop->current_coroutine = this->handle;
        this->handle.resume();
    }
    friend class event_loop;
};

class yield : public queue_mixin<yield> {
    std::coroutine_handle<> handle = nullptr;
    event_loop* target = nullptr;
public:
    yield() = default;
    yield(event_loop* target) : target(target) {
    }

    [[nodiscard]] bool await_ready() const noexcept {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle) noexcept {
        auto origin = get_event_loop(handle);
        auto target = this->target ? this->target : origin;
        debug::ktrace("Coroutine {} yields on event loop {} to event loop {}", get_coroutine_info(handle), origin, target);
        if(!target) {
            panic("Target event loop is null");
        }

        this->handle = handle;
        target->yield_coroutine(this);
        if(origin) {
            origin->current_coroutine = nullptr;
        }
        if(origin != target) {
            set_event_loop(handle, target);
        }
        return true;
    }

    void await_resume() const noexcept {
        return;
    }
private:
    void complete() {
        auto loop = get_event_loop(this->handle);
        debug::ktrace("Resuming coroutine {} after yield on event loop {}", get_coroutine_info(this->handle), loop);
        if(!loop) {
            panic("Event loop is null");
        }
        loop->current_coroutine = this->handle;
        this->handle.resume();
    }
    friend struct yield_queue;
    friend class event_loop;
};
using yield_to = yield;
using move_to_event_loop = yield;

}

namespace kernel {
    using events::yield;
}
