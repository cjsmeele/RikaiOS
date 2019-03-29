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

    /// Performs an integer divide, rounding up to the next whole number.
    template<typename N, typename M> constexpr N div_ceil(N x, M y) {
        auto rest = x % y;
        if (rest) return x / y + 1;
        else      return x / y;
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
    ///@}

}
