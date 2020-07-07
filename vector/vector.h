#include <cstddef>
#include <algorithm>
#include <cassert>

template <typename T>
struct vector
{
    typedef T* iterator;
    typedef T const* const_iterator;

    vector() : data_(nullptr), size_(0), capacity_(0) {}
    vector(vector const& other) : vector(other.data_, other.size_, other.size_) {}
    vector& operator=(vector const& other) {
        if (this != &other) {
            vector safe(other);
            swap(safe);
        }
        return *this;
    }

    ~vector() {
        clear();
        operator delete(data_);
    }

    T& operator[](size_t i) {
        assert(i >= 0 && i < size_);
        return data_[i];
    }

    T const& operator[](size_t i) const {
        assert(i >= 0 && i < size_);
        return data_[i];
    }

    T* data() {
        return data_;
    }

    T const* data() const {
        return data_;
    }

    size_t size() const {
        return size_;
    }

    T& front() {
        assert(size_ > 0);
        return data_[0];
    }

    T const& front() const {
        assert(size_ > 0);
        return data_[0];
    }

    T& back() {
        assert(size_ > 0);
        return data_[size_ - 1];
    }

    T const& back() const {
        assert(size_ > 0);
        return data_[size_ - 1];
    }

    void push_back(T const& elem) {
        if (size_ == capacity_) {
            T safe = elem;
            set_capacity(capacity_ ? 2 * capacity_ : 1);
            new(data_ + size_) T(safe);
        } else {
            new(data_ + size_) T(elem);
        }
        size_++;
    }

    void pop_back() {
        assert(size_ > 0);
        data_[--size_].~T();
    }

    bool empty() const {
        return size_ == 0;
    }

    size_t capacity() const {
        return capacity_;
    }

    void reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            set_capacity(new_capacity);
        }
    }

    void shrink_to_fit() {
        if (capacity_ > size_) {
            vector safe(data_, size_, size_);
            swap(safe);
        }
    }

    void clear() {
        while (size_) {
            pop_back();
        }
    }

    void swap(vector& other) {
        std::swap(data_, other.data_);
        std::swap(capacity_, other.capacity_);
        std::swap(size_, other.size_);
    }

    iterator begin() {
        return data_;
    }

    iterator end() {
        return data_ + size_;
    }

    const_iterator begin() const {
        return data_;
    }

    const_iterator end() const {
        return data_ + size_;
    }

    iterator insert(const_iterator it, T const& elem) {
        ptrdiff_t pos = it - begin();
        push_back(elem);
        for (ptrdiff_t i = size_ - 1; i > pos; i--) {
            std::swap(data_[i - 1], data_[i]);
        }
        return begin() + pos;
    }

    iterator erase(const_iterator pos) {
        return erase(pos, pos + 1);
    }

    iterator erase(const_iterator first, const_iterator last) {
        ptrdiff_t len = last - first;
        ptrdiff_t left = first - begin();
        if (len > 0) {
            for (ptrdiff_t i = left; i + len < size_; i++) {
                std::swap(data_[i], data_[i + len]);
            }
            for (ptrdiff_t i = 0; i < len; i++) {
                pop_back();
            }
        }
        return begin() + left;
    }

private:
    T* data_;
    size_t size_;
    size_t capacity_;

    vector(T const* data, size_t size, size_t capacity) {
        size_ = 0;
        capacity_ = capacity;
        data_ = capacity == 0 ? nullptr : static_cast<T*>(operator new(capacity * sizeof(T)));
        try {
            while (size_ < size) {
                new(data_ + size_) T(data[size_]);
                size_++;
            }
        } catch (...) {
            clear();
            operator delete(data_);
            throw;
        }
    }

    void set_capacity(size_t new_capacity) {
        if (new_capacity >= size_) {
            vector safe(data_, size_, new_capacity);
            swap(safe);
        }
    }
};
