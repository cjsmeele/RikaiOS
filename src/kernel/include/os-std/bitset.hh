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
#include <os-std/container.hh>

namespace ostd {

    /**
     * Bitset datatype.
     *
     * This is an efficient storage mechanism for fixed-size lists of booleans.
     * The interface is somewhat similar to that of std::bitset<N>.
     */
    template<size_t Nbits>
    struct Bitset {

        /// The underlying integer type we use for storing bits.
        using T = u32;

        /// The amount of bits we can store in a T (32 for T=u32).
        static constexpr size_t Tbits = sizeof(T)*8;

        /// How many words are needed to store this bitset.
        static constexpr size_t num_Ts = div_ceil(Nbits, Tbits);

        /// The array of u32s that holds our bits.
        Array<T, num_Ts> data_;

        /// Get the amount of elements (bits).
        constexpr size_t size() const { return Nbits; }

        /// Get the amount of elements (bits).
        constexpr       Array<T,num_Ts> &data()       { return data_; }
        constexpr const Array<T,num_Ts> &data() const { return data_; }

        /// \name Functions to get/set/clear/toggle individual bits.
        ///@{
        constexpr void set       (size_t i
                                 ,bool v = true)  { data_[i/32] = bit_set   (data_[i/32], i%32, v); }
        constexpr void clear     (size_t i)       { data_[i/32] = bit_clear (data_[i/32], i%32);    }
        constexpr void toggle    (size_t i)       { data_[i/32] = bit_toggle(data_[i/32], i%32);    }

        constexpr bool get       (size_t i) const { return bit_get(data_[i/32], i%32); }
        constexpr bool operator[](size_t i) const { return get(i);                    }
        ///@}

        /// Structure representing a position in the data array.
        struct index_t { size_t word, bit; };

        /// Translates a bit index into a word number and bit number within that word.
        constexpr index_t index(size_t i) { return { i / Tbits, i % Tbits }; }

        constexpr void set_all(bool val = true) {
            if (val)
                 for (auto &w : data_) w = ~T(0);
            else for (auto &w : data_) w =  T(0);
        }

        constexpr void clear_all() { set_all(false); }

        /// Efficiently sets multible bits at once.
        constexpr void set_range(size_t i, size_t count, bool val) {
            if (!count)
                return;

            auto [data_i, first_bitoff] = index(i);
            {   // Set bits of first word.
                size_t first_count = min(count, Tbits-first_bitoff);
                data_[data_i] = bit_set_range(data_[data_i], first_bitoff, first_count, val);
                count -= first_count;
                ++data_i;
            }
            // Set the rest of the bits.
            while (count > 0) {
                size_t n = min(count, Tbits);
                data_[data_i] = bit_set_range(data_[data_i], 0, n, val);
                count -= n;
                ++data_i;
            }
        }

        /// \name Bitwise operators.
        /// These are currently only useful for bitsets of size 32 or smaller.
        ///@{
        constexpr Bitset  operator | (u32 x) const { Bitset o = *this; o |= x; return o; }
        constexpr Bitset  operator & (u32 x) const { Bitset o = *this; o &= x; return o; }
        constexpr Bitset  operator ^ (u32 x) const { Bitset o = *this; o ^= x; return o; }
        constexpr Bitset &operator |=(u32 x)       { data_[0] |= x; return *this;         }
        constexpr Bitset &operator &=(u32 x)       { data_[0] &= x; return *this;         }
        constexpr Bitset &operator ^=(u32 x)       { data_[0] ^= x; return *this;         }
        ///@}

        constexpr Bitset()      : data_{   } { }
        constexpr Bitset(u32 x) : data_{{x}} { }
    };

    Bitset(u32 x) -> Bitset<32>;
    Bitset(u16 x) -> Bitset<16>;
    Bitset(u8  x) -> Bitset< 8>;

    template<size_t Nbits>
    [[nodiscard]] constexpr Bitset<Nbits> bit_set_range(Bitset<Nbits> x
                                                       ,u8 i
                                                       ,u8 count
                                                       ,bool val) {
        x.set_range(i, count, val);
        return x;
    }
}

namespace ostd::Format {
    /// Formatter for bitsets.
    template<typename F, size_t N>
    constexpr int format(F &print, Flags f, const Bitset<N> &bitset) {
        if constexpr (N < 32*16) {
            // Is the bitset relatively small? Print it as a binary string.
            String<N+2> buffer;
            if (f.prefix_radix)
                buffer += "0b";
            for (size_t i = 0; i < N; ++i)
                buffer += bitset[N-i-1] ? '1' : '0';

            // Handle padding.
            return format(print, f, buffer);
        } else {
            // Bitset is huge; ignore flags and print in hex.
            (void)f;
            int count = 0;
            for (size_t i = 0; i < Bitset<N>::num_Ts; ++i) {
                count += fmt(print, "{08x}", bitset.data()[Bitset<N>::num_Ts-i-1]);
                if (i % 8 == 7)
                    // Print the index at the end of each line.
                    count += fmt(print, ":{08x}\n"
                                ,(Bitset<N>::num_Ts-(i+1))
                                 *Bitset<N>::Tbits);
                else if (i % 2 == 1)
                    count++, print("'");
            }

            return count;
        }
    }
}
