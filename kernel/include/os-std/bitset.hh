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
#include <os-std/math.hh>
#include <os-std/array.hh>
#include <os-std/string.hh>
#include <os-std/fmt.hh>

namespace ostd {

    /**
     * Bitset datatype.
     *
     * This is an efficient storage mechanism for fixed-size lists of booleans.
     * The interface is somewhat similar to that of std::bitset<N>.
     */
    template<size_t Nbits>
    struct Bitset {

        /// How many 32-bit ints are needed to store this bitset.
        static constexpr size_t num_u32s = div_ceil(Nbits, 32);

        /// The array of u32s that holds our bits.
        Array<u32, num_u32s> data;

        /// Get the amount of elements (bits).
        size_t size() const { return Nbits; }

        /// \name Functions to get/set/clear/toggle individual bits.
        ///@{
        void set       (size_t i
                       ,bool v = true)  { data[i/32] = bit_set   (data[i/32], i%32, v); }
        void clear     (size_t i)       { data[i/32] = bit_clear (data[i/32], i%32);    }
        void toggle    (size_t i)       { data[i/32] = bit_toggle(data[i/32], i%32);    }

        bool get       (size_t i) const { return bit_get(data[i/32], i%32); }
        bool operator[](size_t i) const { return get(i);                    }
        ///@}

        /// \name Bitwise operators.
        /// These are currently only useful for bitsets of size 32 or smaller.
        ///@{
        Bitset  operator | (u32 x) const { Bitset o = *this; o |= x; return o; }
        Bitset  operator & (u32 x) const { Bitset o = *this; o &= x; return o; }
        Bitset  operator ^ (u32 x) const { Bitset o = *this; o ^= x; return o; }
        Bitset &operator |=(u32 x)       { data[0] |= x; return *this;         }
        Bitset &operator &=(u32 x)       { data[0] &= x; return *this;         }
        Bitset &operator ^=(u32 x)       { data[0] ^= x; return *this;         }
        ///@}

        Bitset()      : data{   } { }
        Bitset(u32 x) : data{{x}} { }
    };

    Bitset(u32 x) -> Bitset<32>;
    Bitset(u16 x) -> Bitset<16>;
    Bitset(u8  x) -> Bitset< 8>;
}

namespace ostd::Format {
    /// Formatter for bitsets.
    template<typename F, size_t N>
    constexpr int format(F &print, Flags f, const Bitset<N> &bitset) {
        String<N+2> buffer;
        if (f.prefix_radix)
            buffer += "0b";
        for (size_t i = 0; i < N; ++i)
            buffer += bitset[N-i-1] ? '1' : '0';

        // Handle padding.
        return format(print, f, buffer);
    }
}
