#pragma once

#include <cstddef>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <climits>
#include "buffer.h"

struct optimized_vector
{
    using iterator = uint32_t*;
    using const_iterator = uint32_t const*;

    optimized_vector() : size_(0) {};
    optimized_vector(size_t sz, uint32_t value = 0) : size_(sz) {
        if (sz <= MAX_SMALL) {
            std::fill(static_data, static_data + sz, value);
        } else {
            dynamic_data = buffer::allocate_buffer(sz);
            std::fill(dynamic_data->data, dynamic_data->data + sz, value);
            size_ |= FLAG;
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
            if ((*this)[i] != other[i]) {
                return false;
            }
        }
        return true;
    }

    uint32_t& operator[](size_t i) {
        unshare(capacity());
        return small() ? static_data[i] : dynamic_data->data[i];
    }

    uint32_t const& operator[](size_t i) const {
        return small() ? static_data[i] : dynamic_data->data[i];
    }

    size_t size() const { return size_ & (SIZE_MAX - FLAG); }

    uint32_t& back() {
        unshare(capacity());
        return small() ? static_data[size_ - 1] : dynamic_data->data[size() - 1];
    }

    uint32_t const& back() const {
        return small() ? static_data[size_ - 1] : dynamic_data->data[size() - 1];
    }

    void push_back(uint32_t const& val) {
        if (small() && size_ < MAX_SMALL) {
            static_data[size_++] = val;
        } else {
            to_big(capacity() * (1 + (size() == capacity())));
            unshare(capacity() * (1 + (size() == capacity())));
            if (size() == capacity()) {
                buffer* expanded = buffer::allocate_buffer(2 * capacity());
                std::copy_n(dynamic_data->data, size(), expanded->data);
                operator delete(dynamic_data);
                dynamic_data = expanded;
            }
            dynamic_data->data[size()] = val;
            size_++;
        }
    }

    void pop_back() {
        unshare(capacity());
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
            if (other.small()) {
                std::swap(static_data, other.static_data);
                std::swap(size_, other.size_);
            } else {
                swap_small_big(*this, other);
            }
        } else {
            if (other.small()) {
                swap_small_big(other, *this);
            } else {
                std::swap(dynamic_data, other.dynamic_data);
                std::swap(size_, other.size_);
            }
        }
    }

    iterator begin() {
        unshare(capacity());
        return small() ? static_data : dynamic_data->data;
    }
    iterator end() {
        unshare(capacity());
        return small() ? static_data + size() : dynamic_data->data + size();
    }

    const_iterator begin() const {
        return small() ? static_data : dynamic_data->data;
    }
    const_iterator end() const {
        return small() ? static_data + size() : dynamic_data->data + size();
    }

    iterator insert(const_iterator it, uint32_t const& elem) {
        return insert(it, 1, elem);
    }

    iterator insert(const_iterator first, size_t cnt, uint32_t const& elem) {
        unshare(capacity());
        size_t old_size = size();
        ptrdiff_t start = first - begin();
        for (size_t i = 0; i < cnt; i++) {
            push_back(elem);
        }
        for (ptrdiff_t i = old_size + cnt - 1; i >= static_cast<ptrdiff_t>(start + cnt); i--) {
            std::swap((*this)[i], (*this)[i - cnt]);
        }
        return begin() + start;
    }

    iterator erase(const_iterator pos) {
        return erase(pos, pos + 1);
    }

    iterator erase(const_iterator first, const_iterator last) {
        ptrdiff_t len = last - first;
        ptrdiff_t left = first - begin();
        if (len > 0) {
            unshare(capacity());
            for (ptrdiff_t i = left; i + len < size(); i++) {
                std::swap((*this)[i], (*this)[i + len]);
            }
            size_ -= len;
        }
        return begin() + left;
    }

private:
    static constexpr size_t MAX_SMALL = 2;
    static constexpr size_t FLAG = static_cast<size_t>(1) << (CHAR_BIT * sizeof(size_t) - 1);
    size_t size_;

    union {
        uint32_t static_data[MAX_SMALL];
        buffer* dynamic_data;
    };

    void unshare(size_t new_capacity) {
        assert(new_capacity >= size());
        if (!small() && dynamic_data->ref_counter > 1) {
            dynamic_data->ref_counter--;
            buffer* unshared_data = buffer::allocate_buffer(new_capacity);
            std::copy_n(dynamic_data->data, size(), unshared_data->data);
            dynamic_data = unshared_data;
        }
    }

    void to_big(size_t new_capacity) {
        assert(new_capacity >= size());
        if (small()) {
            buffer* copy = buffer::allocate_buffer(new_capacity);
            std::copy_n(static_data, size(), copy->data);
            dynamic_data = copy;
            size_ |= FLAG;
        }
    }

    static void swap_small_big(optimized_vector& small_vector, optimized_vector& big_vector) {
        buffer* big_data = big_vector.dynamic_data;
        std::copy_n(small_vector.static_data, small_vector.size_, big_vector.static_data);
        small_vector.dynamic_data = big_data;
        std::swap(small_vector.size_, big_vector.size_);
    }

    bool small() const {
        return (size_ & FLAG) == 0;
    }
};

