#pragma once

#include <cstdint>
#include <initializer_list>

namespace kernel {

inline bool get_bit(uint32_t value, unsigned int bit) {
    return value & (1<<bit);
}

inline uint32_t extract_bits(uint32_t value, std::initializer_list<unsigned int> bits) {
    uint32_t result{};
    unsigned int pos = 0;
    for(auto i : bits) {
        if(get_bit(value, i)) {
            result |= (1<<pos);
        }
        pos++;
    }
    return result;
}

}
