#pragma once

#include <cstddef>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include "buffer.h"

struct optimized_vector
{
    typedef uint32_t* iterator;
    typedef uint32_t const* const_iterator;

    optimized_vector() : size_(0) {};
    optimized_vector(size_t sz, uint32_t value = 0) : size_(0) {
        while (size_ < sz) {
            push_back(value);
        }
    }

    optimized_vector(optimized_vector const& other) : size_(other.size_) {
        if (other.small()) {
            std::copy_n(other.static_data, size_, static_data);
        } else {
            other.dynamic_data->ref_counter++;
            dynamic_data = other.dynamic_data;
        }
    }

    optimized_vector& operator=(optimized_vector const& other) {
        if (this != &other) {
            optimized_vector safe(other);
            swap(safe);
        }
        return *this;
    }

    ~optimized_vector() {
        if (!small()) {
            dynamic_data->ref_counter--;
            if (dynamic_data->ref_counter == 0) {
                operator delete(dynamic_data);
            }
        }
    }

    bool operator==(optimized_vector const& other) const {
        if (size() != other.size()) {
            return false;
        }
        for (size_t i = 0; i < size(); i++) {
            if ((small() ? static_data[i] : dynamic_data->data[i])
            != (other.small() ? other.static_data[i] : other.dynamic_data->data[i])) {
                return false;
            }
        }
        return true;
    }

    uint32_t& operator[](size_t i) {
        unshare();
        return small() ? static_data[i] : dynamic_data->data[i];
    }

    uint32_t const& operator[](size_t i) const {
        return small() ? static_data[i] : dynamic_data->data[i];
    }

    size_t size() const { return size_ & (SIZE_MAX - FLAG); }

    uint32_t& back() {
        unshare();
        return small() ? static_data[size_ - 1] : dynamic_data->data[size() - 1];
    }

    uint32_t const& back() const {
        return small() ? static_data[size_ - 1] : dynamic_data->data[size() - 1];
    }

    void push_back(uint32_t const& val) {
        unshare();
        if (size_ < MAX_SMALL) {
            static_data[size_++] = val;
        } else {
            to_big();
            if (size() == capacity()) {
                buffer* expanded = buffer::allocate_buffer(2 * capacity());
                std::copy_n(dynamic_data->data, size(), expanded->data);
                dynamic_data = expanded;
            }
            dynamic_data->data[size()] = val;
            size_++;
        }
    }

    void pop_back() {
        unshare();
        size_--;
    }

    bool empty() const {
        return size() == 0;
    }

    size_t capacity() const {
        return small() ? MAX_SMALL : dynamic_data->capacity;
    }

    void swap(optimized_vector& other) {
        if (small()) {
            size_t sz = size_;
            uint32_t this_data[MAX_SMALL];
            std::copy_n(static_data, size_, this_data);
            if (other.small()) {
                std::copy_n(other.static_data, other.size(), static_data);
            } else {
                dynamic_data = other.dynamic_data;
            }
            std::copy_n(this_data, sz, other.static_data);
        } else {
            if (other.small()) {
                buffer* buf = dynamic_data;
                std::copy_n(other.static_data, other.size(), static_data);
                other.dynamic_data = buf;
            } else {
                std::swap(dynamic_data, other.dynamic_data);
            }
        }
        std::swap(size_, other.size_);
    }

    iterator begin() {
        unshare();
        return small() ? static_data : dynamic_data->data;
    }
    iterator end() {
        unshare();
        return small() ? static_data + size() : dynamic_data->data + size();
    }

    const_iterator begin() const {
        return small() ? static_data : dynamic_data->data;
    }
    const_iterator end() const {
        return small() ? static_data + size() : dynamic_data->data + size();
    }

    iterator insert(const_iterator it, uint32_t const& elem) {
        ptrdiff_t pos = it - begin();
        push_back(elem);
        if (small()) {
            for (ptrdiff_t i = size() - 1; i > pos; i--) {
                std::swap(static_data[i - 1], static_data[i]);
            }
        } else {
            for (ptrdiff_t i = size() - 1; i > pos; i--) {
                std::swap(dynamic_data->data[i - 1], dynamic_data->data[i]);
            }
        }
        return begin() + pos;
    }

    iterator insert(const_iterator first, size_t cnt, uint32_t const& elem) {
        for (size_t i = 0; i < cnt; i++) {
            insert(first, elem);
        }
        ptrdiff_t start_to_return = first - begin();
        return begin() + start_to_return;
    }

    iterator erase(const_iterator pos) {
        return erase(pos, pos + 1);
    }

    iterator erase(const_iterator first, const_iterator last) {
        ptrdiff_t len = last - first;
        ptrdiff_t left = first - begin();
        if (len > 0) {
            if (small()) {
                for (ptrdiff_t i = left; i + len < size(); i++) {
                    std::swap(static_data[i], static_data[i + len]);
                }
            } else {
                for (ptrdiff_t i = left; i + len < size(); i++) {
                    std::swap(dynamic_data->data[i], dynamic_data->data[i + len]);
                }
            }
            for (ptrdiff_t i = 0; i < len; i++) {
                pop_back();
            }
        }
        return begin() + left;
    }

private:
    static constexpr size_t MAX_SMALL = 2;
    static constexpr size_t FLAG = static_cast<size_t>(1) << 31u;
    size_t size_;

    union {
        uint32_t static_data[MAX_SMALL];
        buffer* dynamic_data;
    };

    void unshare() {
        if (!small() && dynamic_data->ref_counter > 1) {
            dynamic_data->ref_counter--;
            buffer* unshared_data = buffer::allocate_buffer(capacity());
            std::copy_n(dynamic_data->data, size(), unshared_data->data);
            dynamic_data = unshared_data;
        }
    }

    void to_big() {
        if (small()) {
            size_ |= FLAG;
            buffer* copy = buffer::allocate_buffer(size());
            std::copy_n(static_data, size(), copy->data);
            dynamic_data = copy;
        }
    }

    bool small() const {
        return (size_ & FLAG) == 0;
    }
};