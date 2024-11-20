#pragma once

#include "kernel/basic.hpp"
#include "kernel/debug.hpp"

#include <expected>
#include <type_traits>
#include <utility>

namespace kernel {

namespace threads {
    void init();
    void start();

    using entry_point = std::add_pointer_t<void(void*)>;

    enum class thread_create_error {
        no_free_thread,
        out_of_argument_memory,
        no_more_thread_ids,
    };
    std::expected<unsigned int, thread_create_error> create(entry_point entry, const void* args, std::size_t args_size);

    namespace detail {
        class prepared_thread {
            public:
                prepared_thread() = default;

                inline char* arg() {
                    return arg_pointer;
                }

                template<typename T>
                void construct_arg(T&& arg) {
                    new(arg_pointer) T(std::forward<T>(arg));
                }
            private:
                friend std::expected<prepared_thread, thread_create_error> prepare_thread(entry_point entry, std::size_t args_size);
                friend std::expected<unsigned int, thread_create_error> start_thread(prepared_thread thread);

                prepared_thread(unsigned int id, void* block, char* arg_pointer) : id(id), block(block), arg_pointer(arg_pointer) {}

                unsigned int id = 0;
                void* block = nullptr;
                char* arg_pointer = nullptr;
        };
        std::expected<prepared_thread, thread_create_error> prepare_thread(entry_point entry, std::size_t args_size);
        std::expected<unsigned int, thread_create_error> start_thread(prepared_thread thread);
    };

    template<typename T>
    std::expected<unsigned int, thread_create_error> create(entry_point entry, T&& arg) {
        auto prepared = detail::prepare_thread(entry, sizeof(T));
        if(!prepared)
            return std::unexpected(prepared.error());

        prepared->construct_arg(std::forward<T>(arg));

        auto res = detail::start_thread(*prepared);
        if(!res)
            return std::unexpected(res.error());
        return *res;
    }

    void kprint_value(ostream& out, const char*&format, thread_create_error value);

    [[noreturn]] void terminate();
    void yield();
}

namespace detail {

    template<typename T>
    struct set_pointer {
        using type = std::remove_cv_t<T>;

        set_pointer(const type&& init, T*& ptr) : value(init), ptr(ptr) {
            ptr = &value;
        }

        // The reference to other.ptr MUST still be valid at this point!
        set_pointer(set_pointer&& other) : value(other.value), ptr(other.ptr) {
            ptr = &value;
        }

        type value;
        T*& ptr;

        type& operator*() {
            return value;
        }
    };
};

class thread {
    public:
        using id = unsigned int;
        using native_handle_type = unsigned int;

        thread() = default;
        thread(const thread&) = delete;
        thread(thread&& other) {
            handle = other.handle;
            other.handle = 0;
        };
        thread& operator=(const thread&) = delete;
        thread& operator=(thread&& other) {
            handle = other.handle;
            other.handle = 0;
            return *this;
        };
        ~thread() {
            if(handle) {
                kernel::panic("Thread not joined!");
            }
        };

        template<typename Func, typename... Args>
        thread(Func func, Args... args) requires std::is_invocable_v<Func, Args...> {
            auto call = [func, args..., detached = detail::set_pointer(false, detached), &done = this->done]() mutable {
                func(args...);
                debug::ktrace("Thread is detached at its end? {} at {}", *detached, &detached.value);
                if(!*detached) {
                    done = true;
                }
            };

            using call_type = decltype(call);
            threads::entry_point entry = [](void* data){
                auto call = reinterpret_cast<call_type*>(data);
                (*call)();
            };

            debug::ktrace("Before place-move deatched = {}", reinterpret_cast<volatile void*>(detached));
            auto ret = threads::create(entry, std::move(call));
            debug::ktrace("After place-move deatched = {}", reinterpret_cast<volatile void*>(detached));
            if(ret.has_value()) {
                handle = ret.value();
            } else {
                debug::kerror("Failed to create thread: {}", ret.error());
                panic("Failed to create thread");
            }
        }

        bool joinable() {
            return get_id() != thread::id{};
        }

        id get_id() {
            return handle;
        }

        native_handle_type native_handle() {
            return handle;
        }

        static unsigned int hardware_concurrency() {
            return 1;
        }

        void join() {
            if(!handle) {
                kernel::panic("Thread not joinable!");
            }

            while(!done) {}

            handle = 0;
        }

        void detach() {
            if(!handle) {
                kernel::panic("Thread not joinable!");
            }
            if(!done) {
                *detached = true;
            }
            handle = 0;
        }

        void swap(thread& other) {
            std::swap(handle, other.handle);
        }
    private:
        unsigned int handle = 0;
        volatile bool done = false;
        volatile bool* detached = nullptr;
};

}
