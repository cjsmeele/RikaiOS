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

    /// \name Functions for classifying and manipulating characters.
    ///@{
    template<typename T>
    constexpr bool is_oneof(const T)           { return false; }
    template<typename T, typename... Us>
    constexpr bool is_oneof(const T v
                           ,const Us&... rest) { return ((v == rest) || ...); }

    template<typename N>
    constexpr bool in_range(N mi, N ma, N x) { return x >= mi && x <= ma; }

    constexpr char to_lower (char c) { return in_range('A', 'Z', c) ? c - 'A' + 'a' : c;      }
    constexpr char to_upper (char c) { return in_range('a', 'z', c) ? c - 'a' + 'A' : c;      }
    constexpr bool is_space (char c) { return is_oneof(c, ' ', '\t', '\n', '\r');             }
    constexpr bool is_lower (char c) { return in_range('a', 'z', c);                          }
    constexpr bool is_upper (char c) { return in_range('A', 'Z', c);                          }
    constexpr bool is_alpha (char c) { return is_lower(c) || is_upper(c);                     }
    constexpr bool is_num   (char c) { return in_range('0', '9', c);                          }
    constexpr bool is_hexnum(char c) { return is_num(c)   || in_range('a', 'f', to_lower(c)); }
    constexpr bool is_alnum (char c) { return is_alpha(c) || is_num(c);                       }
    constexpr bool is_print (char c) { return in_range(' ', '~', c);                          }
    ///@}

    struct StringView;

    /**
     * Fixed-size string class.
     *
     * This provides storage for NUL-terminated strings. It is in essence a
     * wrapper around an Array<char> of the right size.
     *
     * Strings may be modified and resized in place, but the capacity cannot
     * change.
     *
     * `Nchars` is the capacity of the string's contents; the NUL byte is not
     * counted (so String<3> can safely hold "foo").
     *
     * Note that length() != size()!
     * - size() always returns the *capacity* of the string, that is, the
     *   amount of characters it can hold excluding the NUL byte.
     * - length() always returns the amount of characters *currently* in the
     *   string
     */
    template<size_t Nchars>
    struct String {
        Array<char, Nchars+1> data_; // Have extra space for the NUL-byte.

        size_t length_ = 0;

        /// Get the capacity.
        constexpr size_t size()   const { return Nchars; }

        /// Get the amount of characters currently in the string.
        constexpr size_t length() const { return length_;      }
        constexpr bool   empty()  const { return length_ == 0; }

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

        // Empty the string.
        constexpr void clear() {
            *this = "";
        }

        // Trim whitespace at the end of the string.
        constexpr void rtrim() {
            for (ssize_t i = Nchars-1; i >= 0; --i) {
                if (!data_[i] || data_[i] == ' ') {
                    if ((size_t)i < length_)
                        --length_;
                    data_[i] = 0;
                } else {
                    return;
                }
            }
        }

        constexpr bool operator==(StringView o) const;
        constexpr bool operator!=(StringView o) const;

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

        constexpr String &operator=(StringView o);

        constexpr String &operator+=(const char *o) {
            for (size_t i = 0
                ;o[i] && length_ < Nchars
                ;++length_, ++i) {

                data_[length_] = o[i];
            }
            // Add NUL terminator.
            data_[length_] = 0;

            return *this;
        }

        constexpr String &operator+=(char c) {
            if (length_ < Nchars) {
                data_[length_++] = c;
                data_[length_]   = 0;
            }

            return *this;
        }

        constexpr operator StringView();
        constexpr operator const char*() { return data_.data(); }

        constexpr String() { length_ = 0; }

        template<size_t N>
        constexpr String(const char (&data)[N])
            : data_{data}
            , length_(N-1) {

            static_assert(N-1 <= Nchars);
        }

        template<typename... Ts>
        constexpr String(Ts... ts)
            : data_{ts...}
            , length_(sizeof...(Ts)) {

            static_assert(sizeof...(Ts) <= Nchars);
        }

        constexpr String(const char *data) { *this = data; }
    };

    // Deduction guide for string literals.
    template<size_t N>
    String(const char (&data)[N]) -> String<N-1>;

    template<typename... Ts>
    String(Ts... ts) -> String<sizeof...(Ts)>;

    /**
     * Provides a read-only view into a string.
     *
     * This is meant to be a cheap-to-copy method of access to both string
     * literals (const char arrays) and String types.
     */
    struct StringView {

        const char *data_;
        size_t length_;

        constexpr size_t length() const { return length_;      }
        constexpr bool   empty()  const { return length_ == 0; }

        constexpr StringView &operator=(const char *o) {
            data_ = o;
            if (o) for (length_ = 0; o[length_]; ++length_);
            else   length_ = 0;

            return *this;
        }

        /// Get a pointer to the underlying data.
        constexpr const char *data() const { return data_; }

        /// Accessor.
        constexpr char operator[](size_t i)       { return data_[i]; }

        /** \name Iterator-ish functions.
         *
         * These enable the use of range-based for-loops (`for (x : y)`).
         */
        ///@{
        constexpr const char *begin() const { return data_;   }
        constexpr const char *end()   const { return data_+length_; }
        ///@}

        constexpr operator const char*() const { return data_; }

        constexpr bool operator==(const char *o) const {
            return *this == StringView(o);
        }
        constexpr bool operator==(StringView o) const {
            if (o.length() != length_)
                return false;

            for (auto [x,y] : zip(*this, o))
                if (x != y) return false;
            return true;
        }

        constexpr bool operator!=(const char *o) const { return !(*this == o); }
        constexpr bool operator!=(StringView o)  const { return !(*this == o); }

        // constexpr operator bool() const { return data_; }

        constexpr StringView(const char *s)
            : data_(s)
            , length_(str_length(s)) { }
        constexpr StringView(const char *s, size_t length)
            :   data_(s)
            , length_(length) { }

        template<size_t N>
        constexpr StringView(const char (&s)[N])
            :   data_(s)
            , length_(N-1) { }

        template<size_t N>
        constexpr StringView(const String<N> &s)
                :   data_(s.data())
                , length_(s.length()) { }
    };

    template<size_t N>
    constexpr String<N> &String<N>::operator=(StringView o) {
        for (length_ = 0
            ;o[length_] && length_ < N
            ;++length_) {

            data_[length_] = o[length_];
        }
        // Add NUL terminator.
        data_[length_] = 0;

        return *this;
    }

    template<size_t N>
    constexpr String<N>::operator StringView() {
        return StringView(data_.data(), length_);
    }

    template<size_t N>
    constexpr String<N> &operator+=(String<N> &x, StringView y) {
        return x += y.data();
    }

    template<size_t N>
    constexpr bool String<N>::operator==(StringView o) const {
        return StringView(*this) == o;
    }
    template<size_t N>
    constexpr bool String<N>::operator!=(StringView o) const {
        return StringView(*this) != o;
    }


    namespace Format {
        template<typename F>
        constexpr int format(F &print, Flags &f, StringView s) {
            if (!s) s = "<null>";

            int count = 0;

            int pad = f.width ? f.width - (int)s.length() : 0;

            if (!f.align_left && pad > 0)
                count += format_padding(print, ' ', pad);

            print(s);

            if (f.align_left && pad > 0)
                count += format_padding(print, ' ', pad);

            return count + s.length();
        }

        template<typename F, size_t N>
        constexpr int format(F &print, Flags &f, const String<N> &s) {
            return format(print, f, StringView(s));
        }
    }

    /// Format a string, append results to a String.
    template<size_t StrLen, typename... As>
    constexpr int fmt(String<StrLen> &dest, const char *s, const As&... args) {
        int count = fmt(dest.data()+dest.length()
                       ,StrLen+1 - dest.length()
                       ,s, args...);

        dest.length_ += count;

        return count;
    }
}
