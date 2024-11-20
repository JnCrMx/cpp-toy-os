#pragma once

#include <kernel/debug.hpp>
#include <kernel/memory.hpp>

#include <coroutine>
#include <type_traits>

namespace kernel {

namespace events {
    class event_loop;
}

template<typename Return>
struct promise;

template<typename Return>
struct coroutine : std::coroutine_handle<promise<Return>>
{
    using promise_type = promise<Return>;

    ~coroutine() {
        if(this->done() && this->promise().parent) { // top level coroutines have been cleaned up already by their final awaiter
            this->destroy();
        }
    }

    template <typename T>
    [[nodiscard]] std::coroutine_handle<> await_suspend(std::coroutine_handle<T> caller) noexcept {
        debug::ktrace("Coroutine {} called by coroutine {}", get_coroutine_info(*this), get_coroutine_info(caller));
        this->promise().parent = caller;

        // if possible, move new coroutine to the event loop of the caller
        auto loop = this->promise().event_loop = get_event_loop(caller);
        if(loop) {
            loop->current_coroutine = *this;
        }
        return *this;
    }
    [[nodiscard]] bool await_ready() const {
        debug::ktrace("Seeing if coroutine {} is ready -> {}", get_coroutine_info(*this), this->done());
        return false;
    }

    auto await_resume() {
        debug::ktrace("Resuming await from coroutine {}", get_coroutine_info(*this));
        if constexpr (std::is_same_v<Return, void>) {
            return;
        }
        return this->promise().result;
    }
};

namespace detail {
    struct promise_base_base;
}
class coroutine_info {
    public:
        constexpr coroutine_info(const char*&& name, bool&& critical = false, std::source_location&& loc = std::source_location::current()) :
            m_name(name), m_critical(critical), m_location(loc) {}
        constexpr explicit coroutine_info(const char* name, bool critical, std::source_location loc, void* address) :
            m_name(name), m_critical(critical), m_location(loc), m_address(address) {}
        const char* name() const { return m_name; }
        bool critical() const { return m_critical; }
        const std::source_location& location() const { return m_location; }
        void* address() const { return m_address; }
    private:
        const char* m_name;
        bool m_critical;
        std::source_location m_location;
        void* m_address = nullptr;

        std::source_location&& move_location() { return std::move(m_location); }
        friend struct detail::promise_base_base;
};
using coroutine_name = coroutine_info&&;

inline const coroutine_info& get_coroutine_info(std::coroutine_handle<> handle);
inline events::event_loop* get_event_loop(std::coroutine_handle<> handle);
inline void set_event_loop(std::coroutine_handle<> handle, events::event_loop* loop);

namespace detail {
    template <typename R>
    struct final_awaiter {
        [[nodiscard]] bool await_ready() const noexcept {
            return false;
        }

        [[nodiscard]] std::coroutine_handle<> await_suspend(std::coroutine_handle<promise<R>> handle) const noexcept {
            auto parent = handle.promise().parent;
            if(parent)
                debug::ktrace("Final await for coroutine {} going to coroutine {}.", get_coroutine_info(handle), get_coroutine_info(parent));
            else
                debug::ktrace("Final await for coroutine {} going into nothingness.", get_coroutine_info(handle));
            if(!parent) { // this will destroy top-level coroutines
                /*
                    Yes, this kind of goes against the idea of some object having ownership of the coroutine,
                    as well as RAII.
                    However, if we want to avoid this, we would probably move the coroutine handle around,
                    so it always has one owner responsible for destroying it.
                */
                handle.destroy();
            }
            handle.promise().event_loop->current_coroutine = parent;
            return parent ? parent : std::noop_coroutine();
        }

        auto await_resume() const noexcept {}
    };

    template<typename... Args>
    concept ContainsName = (std::is_same_v<std::remove_cvref_t<Args>, coroutine_info> || ...);
    template<typename... Args>
    concept ContainsNameOnce = (std::is_same_v<std::remove_cvref_t<Args>, coroutine_info> + ...) == 1;

    template<typename Last>
    constexpr coroutine_info&& find_coroutine_name(Last&& last) {
        return std::move(last);
    }
    template<typename First, typename... Rest>
    constexpr coroutine_info&& find_coroutine_name(First&& first, Rest&&... rest) {
        if constexpr (std::is_same_v<std::remove_cvref_t<First>, coroutine_info>) {
            return std::move(first);
        } else {
            return find_coroutine_name(rest...);
        }
    }

    struct promise_base_base {
        std::coroutine_handle<> parent{nullptr};

        coroutine_info info;
        events::event_loop* event_loop{nullptr};

        promise_base_base(std::source_location&& location, const char*&& name = "unnamed coroutine") :
            info(std::move(name), false, std::move(location)) {
            info.m_address = std::coroutine_handle<promise_base_base>::from_promise(*this).address();
            debug::ktrace("Created coroutine promise for coroutine {}.", info);
        }
        promise_base_base(coroutine_info&& name) : promise_base_base(std::move(name.move_location()), std::move(name.name())) {}

    };

    template<typename Return>
    struct promise_base : promise_base_base {

        using promise_base_base::promise_base_base;

        std::suspend_always initial_suspend() noexcept { return {}; }
        detail::final_awaiter<Return> final_suspend() noexcept { return {}; }
        void unhandled_exception() noexcept {}

        void* operator new(std::size_t n) noexcept {
            debug::ktrace("Allocating coroutine promise of size {}.", n);
            if(void* mem = kernel::malloc(n))
                return mem;
            debug::kerror("Failed ot allocate memory for promise type of size {}.", n);
            return nullptr;
        }
    };

    inline static const coroutine_info invalid_coroutine{"INVALID COROUTINE", false, std::source_location{}, nullptr};
}

inline const coroutine_info& get_coroutine_info(std::coroutine_handle<> handle) {
    if(!handle) {
        return detail::invalid_coroutine;
    }
    auto h = std::coroutine_handle<detail::promise_base_base>::from_address(handle.address());
    return h.promise().info;
}
events::event_loop* get_event_loop(std::coroutine_handle<> handle) {
    if(!handle) {
        return nullptr;
    }
    auto h = std::coroutine_handle<detail::promise_base_base>::from_address(handle.address());
    return h.promise().event_loop;
}
void set_event_loop(std::coroutine_handle<> handle, events::event_loop* loop) {
    if(!handle) {
        return;
    }
    auto h = std::coroutine_handle<detail::promise_base_base>::from_address(handle.address());
    h.promise().event_loop = loop;
}

template<typename Return>
struct promise : detail::promise_base<Return>
{
    Return result{};

    template<typename... Args>
    promise(Args&&...args) requires(detail::ContainsName<Args...>) :
        detail::promise_base<Return>(std::move(detail::find_coroutine_name(args...))) {
        static_assert(detail::ContainsNameOnce<Args...>, "Coroutine has multiple names");
    }

    promise(std::source_location&& loc = std::source_location::current())
        : detail::promise_base<Return>(std::move(loc)) {}

    void return_value(Return&& ret) {
        result = std::move(ret);
        debug::ktrace("Coroutine {} returned a value.", this->info);
    }

    [[nodiscard]] coroutine<Return> get_return_object() {
        return coroutine<Return>{std::coroutine_handle<promise<Return>>::from_promise(*this)};
    }
    static coroutine<Return> get_return_object_on_allocation_failure() {
        return coroutine<Return>{nullptr};
    }
};

template<>
struct promise<void> : detail::promise_base<void> {
    template<typename... Args>
    promise(Args&&...args) requires(detail::ContainsName<Args...>) :
        detail::promise_base<void>(std::move(detail::find_coroutine_name(args...))) {
        static_assert(detail::ContainsNameOnce<Args...>, "Coroutine has multiple names");
    }

    promise(std::source_location&& loc = std::source_location::current())
        : detail::promise_base<void>(std::move(loc)) {}

    void return_void() {
        debug::ktrace("Coroutine {} returned void.", this->info);
    }

    [[nodiscard]] coroutine<void> get_return_object() {
        return coroutine<void>{std::coroutine_handle<promise<void>>::from_promise(*this)};
    }
    static coroutine<void> get_return_object_on_allocation_failure() {
        return coroutine<void>{nullptr};
    }
};

class get_coroutine_handle
{
    std::coroutine_handle<> m_handle{nullptr};

public:
    bool await_ready() const noexcept
    {
        return m_handle != nullptr;
    }
    bool await_suspend(std::coroutine_handle<> handle) noexcept
    {
        m_handle = handle;
        return false;
    }
    auto await_resume() noexcept
    {
        return m_handle;
    }
};

}
