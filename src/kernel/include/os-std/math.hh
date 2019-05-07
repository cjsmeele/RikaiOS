/* Copyright 2019 Chris Smeele
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <os-std/types.hh>
#include <os-std/type-traits.hh>

/**
 * \file
 * Naïve, unoptimized math functions.
 */

namespace ostd {

    /// x to the yth power.
    template<typename N>
    constexpr N pow(N x, N y) {
        // NB: only works for positive y.
        N res = 1;
        for (N i = 0; i < y; ++i)
            res *= x;
        return res;
    }

    /// Absolute value.
    template<typename N> constexpr N   abs(N x) { return x < 0 ? -x : x; }
    /// Sign bit.
    template<typename N> constexpr int sgn(N x) { return x < 0 ? -1 : 1; }

    /// The base-n logarithm of x.
    template<typename N>
    constexpr N logn(u32 n, N x) {
        x = abs(x);
        N y = 0;

        while (x >= n) {
            x /= n;
            y++;
        }
        return y;
    }

    template<typename N> constexpr N log10(N x) { return logn(10, x); }
    template<typename N> constexpr N log2 (N x) { return logn( 2, x); }

    template<typename N> constexpr N max(N x, N y)          { return x >= y ? x : y; }
    template<typename N> constexpr N min(N x, N y)          { return x <= y ? x : y; }
    template<typename N> constexpr N clamp(N mi, N ma, N x) { return min(ma, max(mi, x)); }

    /// Performs a positive integer divide, rounding up to the next whole number.
    template<typename N, typename M> constexpr N div_ceil(N x, M y) {
        auto rest = x % y;
        if (rest) return x / y + 1;
        else      return x / y;
    }

    /// Performs a positive integer divide, rounding to the nearest whole number.
    template<typename N, typename M> constexpr N div_round(N x, M y) {
        auto rest = x % y;
        // eww.
        if (rest < (y&1?y+1:y)/2) return x / y;
        else                      return x / y + 1;
    }

    template<typename N, typename M> constexpr N align_up(N x, M y) {
             if (y == 0) return x;
        else if (x % y)  return x + (y - x % y);
        else             return x;
    }

    template<typename N> constexpr bool is_even(N x) { return (x & 1) == 0; }
    template<typename N> constexpr bool is_odd (N x) { return (x & 1) == 1; }
    template<typename N
            ,typename M> constexpr bool is_divisible(N x, M y) { return x % y == 0; }

    /**
     * \name Bit manipulation functions.
     *
     * These functions all return the new integer.
     */
    ///@{
    template<typename N> constexpr bool bit_get(N x, u8 i) {
        if (i) return !!(x & (1 << i));
        else   return x & 1;
    }
    template<typename N> [[nodiscard]] constexpr N bit_set(N x, u8 i, bool val = true) {
        if (val) return x |=   1 << i;
        else     return x &= ~(1 << i);
    }
    template<typename N> [[nodiscard]] constexpr N bit_clear (N x, u8 i) { return bit_set(x, i, false); }
    template<typename N> [[nodiscard]] constexpr N bit_toggle(N x, u8 i) { return x ^= 1 << i;          }
    template<typename N> [[nodiscard]] constexpr N bit_set_range(N x, u8 i, u8 count, bool val) {
        using U = typename add_unsigned<N>::type;
        U y(x);
        auto max = intmax<U>::value;
        return val
            ? y |   U(max) >> (sizeof(U)*8-count) << i
            : y & ~(U(max) >> (sizeof(U)*8-count) << i);
    }
    template<typename N> constexpr N bit_get_range(N x, u8 i, u8 count) {
        using U = typename add_unsigned<N>::type;
        U y(x);
        auto max = intmax<U>::max();
        int tmp = sizeof(U)*8 - count - i;
        return (x << tmp & max) >> (tmp+i);
    }
    ///@}

    /**
     * \name Bit analysis functions.
     *
     * These functions make use of compiler builtins that translate to
     * specialized x86 instructions such as LZCNT to efficiently count the
     * number of leading / trailing zeroes or ones in a word.
     *
     * These builtins are supported by both GCC and Clang.
     */
    ///@{
#ifdef __GNUC__
    constexpr u8    clz_(u32 x) { return __builtin_clz(x);      }
    constexpr u8    ctz_(u32 x) { return __builtin_ctz(x);      }
    constexpr u8 popcnt_(u32 x) { return __builtin_popcount(x); }
#else
    #error "Need alternative LZCNT / CLZ, CTZ, POPCNT implementations since this is not GCC/Clang"
#endif

    template<typename N>
    constexpr u8 bit_count(N x) {
        constexpr auto nbits = sizeof(N) * 8;
        static_assert(popcnt_(u32{0xffff0001}) == 17
                      , "builtin popcount does not work for 32-bit numbers");

        if constexpr (nbits == 64) {
            return popcnt_(x >> 32)
                 + popcnt_((u32)x);
        } else {
            return popcnt_(x);
        }
    }

    template<typename N>
    constexpr u8 count_leading_0s(N x) {
        constexpr auto nbits = sizeof(N) * 8;
        if (!x) return nbits;
        static_assert(clz_(u32{         1}) == 31, "builtin clz does not work for 32-bit numbers");
        static_assert(clz_(u32{0xffffffff}) ==  0, "builtin clz does not work for 32-bit numbers");
        if constexpr (nbits == 64) {
            if (x >> 32) return clz_((u32)(x >> 32));
            else         return clz_((u32)x) + 32;
        } else if constexpr (nbits < 32) {
            return clz_((u32)x) - (32 - nbits);
        } else {
            return clz_(x);
        }
    }

    template<typename N>
    constexpr u8 count_leading_1s(N x) {
        return count_leading_0s(static_cast<N>(~x));
    }

    template<typename N>
    constexpr u8 count_trailing_0s(N x) {
        constexpr auto nbits = sizeof(N) * 8;
        if (!x) return nbits;
        static_assert(ctz_(u32{         1}) ==  0, "builtin ctz does not work for 32-bit numbers");
        static_assert(ctz_(u32{0x80000000}) == 31, "builtin ctz does not work for 32-bit numbers");
        if constexpr (nbits == 64) {
            if ((u32)x) return ctz_((u32)x);
            else        return ctz_((u32)(x >> 32)) + 32;
        } else {
            return ctz_(x);
        }
    }
    template<typename N>
    constexpr u8 count_trailing_1s(N x) {
        return count_trailing_0s(static_cast<N>(~x));
    }
    ///@}
}
