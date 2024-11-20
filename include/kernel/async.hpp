#pragma once

#include <kernel/coroutine.hpp>
#include <kernel/events.hpp>

#include <new>
#include <cstddef>

namespace kernel {

class async_istream {
    public:
        virtual ~async_istream() {};

        /**
         * Returns an awaiter that waits for the next input and returns the received character.
         */
        [[nodiscard("The awaiter must be awaited.")]] virtual events::awaiter co_get();
        /**
         * Asynchronously reads up to `size-1` characters from the stream and
         * places them in `buffer` until `delim` is encountered.
         * The delimeter will NOT be included in the buffer.
         * After reading a `\0` will ALWAYS be placed in the buffer.
         *
         * Returns `true` if the line was terminated due to encountering the delimeter or
         * `false` is the buffer got filled before that.
         */
        [[nodiscard("The coroutine must be awaited.")]] virtual coroutine<bool> co_getline(char* buffer, std::size_t size, char delim = '\r') {
            if(size == 0) {
                co_return false;
            }
            size_t i;
            bool okay = false;
            for(i = 0; i < size-1; i++) {
                buffer[i] = co_await co_get();
                if(buffer[i] == delim) {
                    okay = true;
                    break;
                }
            }
            buffer[i] = '\0';
            co_return okay;
        }
        /**
         * Asynchronously reads characters from the stream and
         * places them in `buffer` until `delim` is encountered
         * or the buffer is full (leaving space for a terminating `\0`).

         * The delimeter will NOT be included in the buffer.
         * After reading a `\0` will ALWAYS be placed in the buffer.
         *
         * Returns `true` if the line was terminated due to encountering the delimeter or
         * `false` is the buffer got filled before that.
         */
        template<typename Buffer>
        [[nodiscard("The coroutine must be awaited.")]] coroutine<bool> co_getline(Buffer& buffer, char delim = '\r') {
            if(buffer.size() == 0) {
                co_return false;
            }
            size_t i;
            bool okay = false;
            for(i = 0; i < buffer.size()-1; i++) {
                buffer[i] = co_await co_get();
                if(buffer[i] == delim) {
                    okay = true;
                    break;
                }
            }
            buffer[i] = '\0';
            co_return okay;
        }

        void operator delete([[maybe_unused]] async_istream* p, std::destroying_delete_t) {}
};

}
