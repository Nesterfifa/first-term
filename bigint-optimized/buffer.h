#pragma once

struct buffer {
    size_t ref_counter;
    size_t capacity;
    uint32_t data[];

    static buffer* allocate_buffer(size_t capacity_) {
        auto buf = static_cast<buffer*>(operator new(sizeof(buffer) + capacity_ * sizeof(uint32_t)));
        buf->capacity = capacity_;
        buf->ref_counter = 1;
        return buf;
    }
};

