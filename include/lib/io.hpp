#pragma once

#include <cstddef>
#include <new>
#include <string_view>

namespace kernel {

class ostream {
    public:
        virtual ~ostream() {};

        virtual ostream& put(char ch) = 0;
        virtual ostream& write(const char* s, std::size_t count) {
            for(std::size_t i = 0; i<count; i++) {
                put(s[i]);
            }
            return *this;
        }

        void operator delete([[maybe_unused]] ostream* p, std::destroying_delete_t) {}

        ostream& operator<<(char ch) {
            put(ch);
            return *this;
        }
        ostream& operator<<(const char* str) {
            for(; *str; str++) {
                put(*str);
            }
            return *this;
        }
};

class istream {
    public:
        virtual ~istream() {};

        constexpr static int eof = std::string_view::traits_type::eof();

        virtual int get() = 0;
        virtual int available() const = 0;

        virtual istream& get(char& ch) {
            int c = get();
            if(c != eof) {
                ch = c;
            }
            return *this;
        }
        virtual istream& read(char* data, std::size_t count) {
            for(std::size_t i=0; i<count; i++) {
                get(data[i]);
            }
            return *this;
        }

        void operator delete([[maybe_unused]] istream* p, std::destroying_delete_t) {}

        istream& operator>>(char& ch) {
            get(ch);
            return *this;
        }
};

}
