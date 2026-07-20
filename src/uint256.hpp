#pragma once

#include <algorithm>
#include <cstdint>
#include <iosfwd>
#include <ostream>
#include <string>

inline void print_int128(std::ostream& os, unsigned __int128 n) {
    if (n == 0) {
        os << "0";
        return;
    }
    std::string s;
    while (n > 0) {
        s += static_cast<char>('0' + (n % 10));
        n /= 10;
    }
    std::reverse(s.begin(), s.end());
    os << s;
}

// Lightweight 256-bit unsigned integer for bounded exact walk-DP sweeps.
struct uint256_t {
    unsigned __int128 high = 0;
    unsigned __int128 low = 0;

    uint256_t() = default;
    explicit uint256_t(unsigned __int128 v) : low(v) {}

    uint256_t operator+(const uint256_t& o) const {
        uint256_t r;
        r.low = low + o.low;
        r.high = high + o.high + (r.low < low ? 1 : 0);
        return r;
    }

    uint256_t operator*(uint64_t v) const {
        uint256_t r;
        unsigned __int128 p_low = (low & 0xFFFFFFFFFFFFFFFFULL) * v;
        unsigned __int128 p_mid = (low >> 64) * v + (p_low >> 64);
        r.low = (p_low & 0xFFFFFFFFFFFFFFFFULL) | (p_mid << 64);
        r.high = high * v + (p_mid >> 64);
        return r;
    }

    bool operator<(const uint256_t& o) const {
        return high != o.high ? high < o.high : low < o.low;
    }

    friend std::ostream& operator<<(std::ostream& os, const uint256_t& val) {
        if (val.high == 0) {
            print_int128(os, val.low);
            return os;
        }

        uint256_t n = val;
        std::string s;
        while (n.high != 0 || n.low != 0) {
            uint256_t quotient;
            unsigned remainder = 0;
            for (int bit = 255; bit >= 0; --bit) {
                remainder <<= 1;
                bool set = bit >= 128 ? ((n.high >> (bit - 128)) & 1)
                                      : ((n.low >> bit) & 1);
                if (set) remainder |= 1;
                if (remainder >= 10) {
                    remainder -= 10;
                    if (bit >= 128) {
                        quotient.high |=
                            (static_cast<unsigned __int128>(1) << (bit - 128));
                    } else {
                        quotient.low |=
                            (static_cast<unsigned __int128>(1) << bit);
                    }
                }
            }
            s += static_cast<char>('0' + remainder);
            n = quotient;
        }
        std::reverse(s.begin(), s.end());
        os << s;
        return os;
    }
};
