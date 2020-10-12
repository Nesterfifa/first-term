#pragma once

#include "optimized_vector.h"
#include <cstddef>
#include <vector>
#include <iosfwd>
#include <algorithm>
#include <functional>

struct big_integer {
    big_integer();
    big_integer(big_integer const& other) = default;
    big_integer(int a);
    big_integer(uint32_t);
    explicit big_integer(std::string const& str);
    ~big_integer() = default;

    big_integer& operator=(big_integer const& other) = default;

    big_integer& operator+=(big_integer const& rhs);
    big_integer& operator-=(big_integer const& rhs);
    big_integer& operator*=(big_integer const& rhs);
    big_integer& operator/=(big_integer const& rhs);
    big_integer& operator%=(big_integer const& rhs);

    big_integer& operator&=(big_integer const& rhs);
    big_integer& operator|=(big_integer const& rhs);
    big_integer& operator^=(big_integer const& rhs);

    big_integer& operator<<=(int rhs);
    big_integer& operator>>=(int rhs);

    big_integer operator+() const;
    big_integer operator-() const;
    big_integer operator~() const;

    big_integer& operator++();
    big_integer operator++(int);

    big_integer& operator--();
    big_integer operator--(int);

    friend bool operator==(big_integer const& a, big_integer const& b);
    friend bool operator!=(big_integer const& a, big_integer const& b);
    friend bool operator<(big_integer const& a, big_integer const& b);
    friend bool operator>(big_integer const& a, big_integer const& b);
    friend bool operator<=(big_integer const& a, big_integer const& b);
    friend bool operator>=(big_integer const& a, big_integer const& b);

    friend std::string to_string(big_integer const& a);

private:
    using uint128_t = unsigned __int128;

    bool sign;
    optimized_vector digits;

    size_t size() const;
    void add_leading_zeros(size_t);
    void erase_leading_zeros();
    uint32_t kth_digit(size_t const) const;

    void to_add2(size_t);
    big_integer bit_operation(big_integer const& rhs, const std::function<uint32_t(uint32_t, uint32_t)>&);

    static big_integer div_short(big_integer const&, uint32_t const);
    uint32_t trial(big_integer const&, big_integer const&);
    bool smaller(big_integer const&, big_integer const&, size_t);
    void difference(big_integer&, big_integer const&, size_t);
    void sum_unsigned(big_integer const &rhs);
    void sub_from_bigger(big_integer const &rhs, bool less);
};


big_integer operator+(big_integer a, big_integer const& b);
big_integer operator-(big_integer a, big_integer const& b);
big_integer operator*(big_integer a, big_integer const& b);
big_integer operator/(big_integer a, big_integer const& b);
big_integer operator%(big_integer a, big_integer const& b);

big_integer operator&(big_integer a, big_integer const& b);
big_integer operator|(big_integer a, big_integer const& b);
big_integer operator^(big_integer a, big_integer const& b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);

bool operator==(big_integer const& a, big_integer const& b);
bool operator!=(big_integer const& a, big_integer const& b);
bool operator<(big_integer const& a, big_integer const& b);
bool operator>(big_integer const& a, big_integer const& b);
bool operator<=(big_integer const& a, big_integer const& b);
bool operator>=(big_integer const& a, big_integer const& b);

std::string to_string(big_integer const& a);
std::ostream& operator<<(std::ostream& s, big_integer const& a);
