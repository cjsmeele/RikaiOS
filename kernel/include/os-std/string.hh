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

#include <os-std/array.hh>

/**
 * \file
 * String types.
 *
 * These types have a minimal std::string-like interface.
 */

namespace ostd {

    /**
     * Fixed-size string class.
     *
     * This provides storage for NUL-terminated strings. It is in essence a
     * wrapper around an Array<char> of the right size.
     *
     * Strings may be modified and resized in place, but the capacity cannot
     * change.
     *
     * `Nchars` is the capacity of the string's contents; the NUL byte is not counted.
     *
     * Note that length() != size()!
     * - size() always returns the *capacity* of the string.
     * - length() always returns the amount of characters currently in the string
     */
    template<size_t Nchars>
    struct String {
        Array<char, Nchars+1> data_; // Have extra space for the NUL-byte.

        size_t length_ = 0;

        /// Get the capacity.
        constexpr size_t size()   const { return Nchars; }

        /// Get the amount of characters currently in the string.
        constexpr size_t length() const { return length_; }

        /// Get a pointer to the underlying data.
        constexpr const char *data() const { return data_.data(); }
        constexpr       char *data()       { return data_.data(); }

        /// \name Accessors.
        ///@{
        constexpr const char &operator[](size_t i) const { return data_[i]; }
        constexpr       char &operator[](size_t i)       { return data_[i]; }
        ///@}

        /** \name Iterator-ish functions.
         *
         * These enable the use of range-based for-loops (`for (x : y)`).
         */
        ///@{
        constexpr const char *begin() const { return data_.data();   }
        constexpr       char *begin()       { return data_.data();   }
        constexpr const char *end()   const { return data_.data()+length_; }
        constexpr       char *end()         { return data_.data()+length_; }
        ///@}

        constexpr String &operator=(const char *o) {
            for (length_ = 0
                ;o[length_] && length_ < Nchars
                ;++length_) {

                data_[length_] = o[length_];
            }
            // Add NUL terminator.
            data_[length_] = 0;

            return *this;
        }

        constexpr String() { length_ = 0; }

        template<size_t N>
        constexpr String(const char (&data)[N])
            : data_{data}
            , length_(N-1) {

            static_assert(N-1 <= Nchars);
        }

        constexpr String(const char *data) { *this = data; }
    };

    // Deduction guide for string literals.
    template<size_t N>
    String(const char (&data)[N]) -> String<N-1>;

    namespace {
        template<typename C>
        struct StringViewBase {
            C *data_;
            size_t length_;

            constexpr size_t length() const { return length_; }

            constexpr StringViewBase &operator=(C *o) {
                if (o) {
                    data_ = o;
                    for (length_ = 0; o[length_]; ++length_);
                } else {
                    data_   = nullptr;
                    length_ = 0;
                }
                return *this;
            }

            constexpr StringViewBase(C *s)
                : data_(s)
                , length_(strlen(s)) {
            }

            template<size_t N>
            constexpr StringViewBase(C (&s)[N])
                : data_(s)
                , length_(N-1) { }
        };
    }

    template<typename C>
    struct StringView;

    /**
     * Provides a read-only view into a string.
     *
     * This is meant to be a cheap-to-copy method of access to both string
     * literals (const char arrays) and String types.
     */
    template<typename C>
    struct StringView<const C> : public StringViewBase<const C> {
        using StringViewBase<const C>::data_;
        using StringViewBase<const C>::length_;

        /// Get a pointer to the underlying data.
        constexpr const char *data() const { return data_; }

        /// \name Accessors.
        ///@{
        constexpr const C &operator[](size_t i) const { return data_[i]; }
        constexpr       C  operator[](size_t i)       { return data_[i]; }
        ///@}

        /** \name Iterator-ish functions.
         *
         * These enable the use of range-based for-loops (`for (x : y)`).
         */
        ///@{
        constexpr const char *begin() const { return data_;   }
        constexpr const char *end()   const { return data_+length_; }
        ///@}

        constexpr StringView(const C *s) : StringViewBase<const C>(s) { }

        template<size_t N>
        constexpr StringView(const C (&s)[N]) : StringViewBase<const C>(s) { }
    };

    template<typename C, size_t N>
    StringView(const C (&s)[N]) -> StringView<const C>;
}
