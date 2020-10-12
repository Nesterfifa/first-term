#include "big_integer.h"

#include <cstring>
#include <stdexcept>
#include <climits>

big_integer::big_integer() : sign(false), digits(1) {}

big_integer::big_integer(int a) : sign(a < 0), digits(1) {
    digits[0] = (a == INT_MIN ? static_cast<uint32_t>(INT_MAX) + 1 : abs(a));
}

big_integer::big_integer(uint32_t a) : sign(false), digits(1, a) {}

big_integer::big_integer(std::string const& str) : big_integer() {
    if (str.empty()) {
        throw std::runtime_error("Expected number, found: empty string");
    }
    for (size_t i = (str[0] == '-' || str[0] == '+'); i < str.length(); i += 8) {
        uint32_t part = 0;
        for (size_t j = i; j < std::min(str.length(), i + 8); j++) {
            if (!isdigit(str[j])) {
                throw std::runtime_error("Invalid number");
            }
            part = part * 10 + static_cast<uint32_t>(str[j] - '0');
        }
        uint32_t pwd = 1;
        for (size_t cnt = 0; cnt < std::min(str.length() - i, 8ul); cnt++) {
            pwd *= 10;
        }
        (*this) *= pwd;
        (*this) += part;
    }
    sign = (str[0] == '-' && digits.back() != 0);
}

void big_integer::sum_unsigned(big_integer const &rhs) {
    uint64_t carry = 0;
    for (size_t i = 0; i < size(); i++) {
        uint64_t const res = static_cast<uint64_t>(digits[i]) + rhs.kth_digit(i) + carry;
        digits[i] = res;
        carry = res >> 32u;
    }
    digits.push_back(carry);
    erase_leading_zeros();
}

void big_integer::sub_from_bigger(big_integer const &rhs, bool less) {
    uint64_t res;
    if (less) {
        res = (static_cast<uint64_t>(rhs.kth_digit(1)) << 32u) | rhs.digits[0];
    } else {
        res = (static_cast<uint64_t>(kth_digit(1)) << 32u) | digits[0];
    }
    for (size_t i = 0; i < size(); i++) {
        res -= less ? kth_digit(i) : rhs.kth_digit(i);
        digits[i] = res;
        res >>= 32u;
        if (less) {
            res |= (static_cast<uint64_t>(rhs.kth_digit(i + 2)) - (res > rhs.kth_digit(i + 1))) << 32u;
        } else {
            res |= (static_cast<uint64_t>(kth_digit(i + 2)) - (res > kth_digit(i + 1))) << 32u;
        }
    }
    erase_leading_zeros();
}

big_integer& big_integer::operator+=(big_integer const& rhs) {
    if (sign == rhs.sign) {
        sum_unsigned(rhs);
    } else {
        add_leading_zeros(rhs.size());
        bool less = smaller(*this, rhs, size());
        sub_from_bigger(rhs, less);
        sign ^= less;
    }
    return *this;
}

big_integer& big_integer::operator-=(big_integer const& rhs) {
    if (sign != rhs.sign) {
        sum_unsigned(rhs);
    } else {
        add_leading_zeros(rhs.size());
        bool less = smaller(*this, rhs, size());
        sub_from_bigger(rhs, less);
        sign ^= less;
    }
    return *this;
}

big_integer& big_integer::operator*=(big_integer const& rhs) {
    big_integer ans;
    ans.sign = sign ^ rhs.sign;
    ans.add_leading_zeros(size() + rhs.size());
    for (size_t i = 0; i < size(); i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < rhs.size(); j++) {
            uint64_t const res = static_cast<uint64_t>(digits[i])
                    * rhs.kth_digit(j)
                    + carry
                    + ans.digits[i + j];
            ans.digits[i + j] = res;
            carry = res >> 32u;
        }
        ans.digits[i + rhs.size()] += carry;
    }
    ans.erase_leading_zeros();
    return *this = ans;
}

big_integer big_integer::div_short(big_integer const& a, uint32_t const divider) {
    uint64_t rem = 0;
    big_integer ans;
    ans.add_leading_zeros(a.size());
    for (ptrdiff_t i = ans.size() - 1; i >= 0; i--) {
        uint64_t dividend = static_cast<uint64_t>(a.digits[i]) + (rem << 32u);
        ans.digits[i] = dividend / divider;
        rem = dividend % divider;
    }
    ans.erase_leading_zeros();
    return ans;
}

uint32_t big_integer::trial(big_integer const &a, big_integer const &b) {
    uint128_t a_last_3 = (static_cast<uint128_t>(a.digits[a.digits.size() - 1]) << 64u) |
                         (static_cast<uint128_t>(a.digits[a.digits.size() - 2]) << 32u) |
                         (static_cast<uint128_t>(a.digits[a.digits.size() - 3]));
    uint128_t b_last_2 = ((static_cast<uint128_t>(b.digits[b.digits.size() - 1]) << 32u) | static_cast<uint128_t>(b.digits[b.digits.size() - 2]));
    return static_cast<uint32_t>(std::min(static_cast<uint128_t>(UINT32_MAX), a_last_3 / b_last_2));
}

bool big_integer::smaller(const big_integer &a, const big_integer &b, size_t idx) {
    for (size_t i = 1; i <= a.size(); i++) {
        if (a.digits[a.size() - i] != b.kth_digit(idx - i)) {
            return a.digits[a.size() - i] < b.kth_digit(idx - i);
        }
    }
    return false;
}

void big_integer::difference(big_integer &a, const big_integer &b, size_t idx) {
    size_t start = a.size() - idx;
    uint64_t res = (static_cast<uint64_t>(a.kth_digit(start + 1)) << 32u) | a.kth_digit(start);
    for (size_t i = 0; i < idx; i++) {
        res -= b.kth_digit(i);
        a.digits[start + i] = res;
        res >>= 32u;
        res |= static_cast<uint64_t>(a.kth_digit(start + i + 2) - (res > a.kth_digit(start + i + 1))) << 32u;
    }
}

big_integer& big_integer::operator/=(big_integer const& rhs) {
    big_integer ans;
    if (size() < rhs.size()) {
        return *this = ans;
    }
    if (rhs.size() == 1) {
        ans = div_short(*this, rhs.digits[0]);
    } else {
        uint32_t normalizer = (static_cast<uint64_t>(1) << 32u) / (static_cast<uint64_t>(rhs.digits.back()) + 1);
        big_integer dividend = *this * normalizer;
        big_integer divisor = rhs * normalizer;

        dividend.digits.push_back(0);
        size_t const n = dividend.size();
        size_t const m = divisor.size() + 1;
        ans.add_leading_zeros(n - m + 1);
        for (size_t i = 0; i <= n - m; i++) {
            uint32_t res = trial(dividend, divisor);
            big_integer multiple = divisor * res;
            while (smaller(dividend, multiple, m)) {
                res--;
                multiple -= divisor;
            }
            ans.digits[n - m - i] = res;
            difference(dividend, multiple, m);
            if (dividend.digits.back() == 0) {
                dividend.digits.pop_back();
            }
        }
    }
    ans.sign = sign ^ rhs.sign;
    ans.erase_leading_zeros();
    return *this = ans;
}

big_integer& big_integer::operator%=(big_integer const& rhs) {
    return *this = *this - *this / rhs * rhs;
}

void big_integer::to_add2(size_t length) {
    add_leading_zeros(length);
    if (sign) {
        sign = false;
        for (uint32_t &digit : digits) {
            digit = ~digit;
        }
        (*this)++;
    }
}

big_integer big_integer::bit_operation(big_integer const& rhs, const std::function<uint32_t(uint32_t, uint32_t)>& operation) {
    big_integer this_copy(*this),
    rhs_copy(rhs);
    size_t len = std::max(this_copy.size(), rhs_copy.size());
    this_copy.to_add2(len);
    rhs_copy.to_add2(len);
    this_copy.sign = operation(sign, rhs.sign);
    for (size_t i = 0; i < len; i++) {
        this_copy.digits[i] = operation(this_copy.digits[i], rhs_copy.digits[i]);
    }
    if (this_copy.sign) {
        this_copy.to_add2(len);
        this_copy.sign = true;
    }
    return this_copy;
}

big_integer& big_integer::operator&=(big_integer const& rhs) {
    return *this = bit_operation(rhs, [](uint32_t a, uint32_t b) { return a & b; });
}

big_integer& big_integer::operator|=(big_integer const& rhs) {
    return *this = bit_operation(rhs, [](uint32_t a, uint32_t b) { return a | b; });
}

big_integer& big_integer::operator^=(big_integer const& rhs) {
    return *this = bit_operation(rhs, [](uint32_t a, uint32_t b) { return a ^ b; });
}

big_integer& big_integer::operator<<=(int rhs) {
    *this *= big_integer(static_cast<uint32_t>(1) << (rhs % 32u));
    digits.insert(digits.begin(), rhs / 32, 0);
    return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
    *this /= big_integer(static_cast<uint32_t>(1) << (rhs % 32u));
    digits.erase(digits.begin(), digits.begin() + std::min(rhs / 32ul, size()));
    if (digits.empty()) {
        digits = {0};
        sign = false;
    }
    return sign? --*this : *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    big_integer neg(*this);
    if (neg != 0) {
        neg.sign ^= true;
    }
    return neg;
}

big_integer big_integer::operator~() const {
    big_integer rev(*this);
    if (rev.digits.back() == 0) {
        rev.digits.back() = 1;
        rev.sign = true;
    } else {
        rev.sign ^= true;
        size_t i = 0;
        while (rev.digits[i] == (rev.sign ? UINT32_MAX : 0)) {
            rev.digits[i] = (rev.sign ? 0 : UINT32_MAX);
        }
        rev.digits[i] += (rev.sign ? 1 : -1);
    }
    return rev;
}

big_integer& big_integer::operator++() {
    return *this += 1;
}

big_integer big_integer::operator++(int) {
    big_integer r = *this;
    ++*this;
    return r;
}

big_integer& big_integer::operator--() {
    return *this -= 1;
}

big_integer big_integer::operator--(int)
{
    big_integer r = *this;
    --*this;
    return r;
}

big_integer operator+(big_integer a, big_integer const& b) {
    return a += b;
}

big_integer operator-(big_integer a, big_integer const& b)
{
    return a -= b;
}

big_integer operator*(big_integer a, big_integer const& b)
{
    return a *= b;
}

big_integer operator/(big_integer a, big_integer const& b)
{
    return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b)
{
    return a %= b;
}

big_integer operator&(big_integer a, big_integer const& b)
{
    return a &= b;
}

big_integer operator|(big_integer a, big_integer const& b)
{
    return a |= b;
}

big_integer operator^(big_integer a, big_integer const& b)
{
    return a ^= b;
}

big_integer operator<<(big_integer a, int b)
{
    return a <<= b;
}

big_integer operator>>(big_integer a, int b)
{
    return a >>= b;
}

bool operator==(big_integer const& a, big_integer const& b)
{
    return (a.sign == b.sign && a.digits == b.digits);
}

bool operator!=(big_integer const& a, big_integer const& b)
{
    return !(a == b);
}

bool operator<(big_integer const& a, big_integer const& b)
{
    if (a.sign != b.sign) {
        return a.sign;
    } else if (a.size() != b.size()) {
        return ((a.sign && a.size() > b.size()) || (!a.sign && a.size() < b.size()));
    } else {
        for (ptrdiff_t i = a.size() - 1; i >= 0; --i) {
            if (a.digits[i] != b.digits[i]) {
                return (a.digits[i] < b.digits[i]) ^ a.sign;
            }
        }
    }
    return false;
}

bool operator>(big_integer const& a, big_integer const& b)
{
    return b < a;
}

bool operator<=(big_integer const& a, big_integer const& b)
{
    return !(a > b);
}

bool operator>=(big_integer const& a, big_integer const& b)
{
    return !(a < b);
}

size_t big_integer::size() const {
    return digits.size();
}

void big_integer::add_leading_zeros(size_t length) {
    while (size() < length) {
        digits.push_back(0);
    }
}

void big_integer::erase_leading_zeros() {
    while (size() > 1 && digits.back() == 0) {
        digits.pop_back();
    }
    sign &= digits.back() != 0;
}


uint32_t big_integer::kth_digit(size_t idx) const {
    return (idx < size() ? digits[idx] : 0);
}

std::string to_string(big_integer const& a) {
    if (a == 0) {
        return "0";
    }
    big_integer copy(a);
    std::string res;
    while (copy != 0) {
        big_integer rem = copy % big_integer(10);
        res.push_back('0' + rem.digits.back());
        copy = big_integer::div_short(copy, 10);
    }
    if (a.sign) {
        res.push_back('-');
    }
    std::reverse(res.begin(), res.end());
    return res;
}

std::ostream& operator<<(std::ostream& s, big_integer const& a)
{
    return s << to_string(a);
}
