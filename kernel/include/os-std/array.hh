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

#include <os-std/memory.hh>
#include <os-std/fmt.hh>

namespace ostd {

    /**
     * Array datatype.
     *
     * This is a simple wrapper around a C-style array, made for the purpose of
     * avoiding having to deal with C-specific array/pointer shenanigans.
     *
     * For arrays of characters, check the String datatype instead.
     */
    template<typename T, size_t N>
    struct Array {
        T data_[N];

        /// Get the amount of elements.
        constexpr size_t size() const { return N; }

        /// Get a pointer to the underlying data.
        constexpr const T *data() const { return data_; }
        constexpr       T *data()       { return data_; }

        /// \name Accessors.
        ///@{
        constexpr const T &operator[](size_t i) const { return data_[i]; }
        constexpr       T &operator[](size_t i)       { return data_[i]; }
        ///@}

        /** \name Iterator-ish functions.
         *
         * These enable the use of range-based for-loops (`for (x : y)`).
         */
        ///@{
        constexpr const T *begin() const { return data_;   }
        constexpr       T *begin()       { return data_;   }
        constexpr const T *end()   const { return data_+N; }
        constexpr       T *end()         { return data_+N; }
        ///@}

        constexpr Array() : data_{} { }

        template<size_t N2>
        constexpr Array(const T (&data)[N2])
            : data_{} {

            static_assert(N2 <= N, "tried to initialize an array with too many elements");
            mem_copy(data_, data, N2);
        }

        template<typename... Ts>
        constexpr Array(const Ts&... args)
            : data_{args...} { }
    };

    template<typename T, typename... Ts>
    Array(const T &a, const Ts&... args) -> Array<T,1+sizeof...(args)>;
}

namespace ostd::Format {
    /// Formatter for arrays.
    template<typename F, typename T, size_t N>
    void format(F print, Flags f, const Array<T,N> &ar) {
        print("[ ");
        bool first = true;
        for (const auto &el : ar) {
            if (first) first = false;
            else       print(", ");

            format(print, f, el);
        }
        print(" ]");
    }
}
