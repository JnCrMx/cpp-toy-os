#pragma once

#include "kernel/basic.hpp"

namespace kernel {

template<typename T>
class queue {
    T* head = nullptr;
    T* tail = nullptr;
public:
    constexpr queue() {}

    T* peek() {
        return head;
    }

    T* remove() {
        if(head) {
            auto* y = head;
            head = y->next;
            if(head == nullptr) {
                tail = nullptr;
            }

            // This is important, so we can re-add the element to a new queue later on
            y->next = nullptr;
            return y;
        }
        return nullptr;
    }
    void add(T* y) {
        if(y->next) {
            panic("Tried to add a queue element that already has a next element");
        }

        if(tail) {
            tail->next = y;
            tail = y;
        }
        else {
            head = y;
            tail = y;
        }
    }
};

template<typename T>
struct queue_mixin {
    private:
        T* next = nullptr;
        friend queue<T>;
};

}
