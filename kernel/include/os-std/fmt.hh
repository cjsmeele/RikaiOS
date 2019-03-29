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
#include <os-std/array.hh>
#include <os-std/memory.hh>
#include <os-std/math.hh>

/**
 * \file
 * String formatting library.
 *
 * fmt takes a format string, containing zero or more bracketed blocks (`{}`),
 * and any number of other arguments. It then parser the blocks for formatting
 * options and prints the arguments in order, each with their own applied
 * formatting options.
 *
 * fmt is recursive, but recursion depth is bounded by the number of arguments
 * passed to it. As such, its max recursion depth is known at compile-time.
 *
 * \todo Expand documentation.
 */

namespace ostd {

    template<typename F, typename... As>
    void fmt(F p, const char *s, const As&... args);

    namespace Format {

        /**
         * Formatting flags.
         */
        struct Flags {
            int  width             =  0;
            union {
                u8 radix           = 10;
                u8 scale;
            };
            bool explicit_sign : 1 = false; ///< (only for numbers)
            bool unsign        : 1 = false; ///< Remove sign bit.
            bool prefix_radix  : 1 = false; ///< Prefix number with 0b, 0x, 0o, or 0d.
            bool align_left    : 1 = false;
            bool prefix_zero   : 1 = false; ///< (only for numbers)
            bool repeat        : 1 = false; ///< (only for chars) fill width by repetition
            bool size          : 1 = false; ///< (only for numbers) format as human-readable K/M/G
        };

        template<typename F>
        constexpr void format_padding(F print, char c, int count) {
            for (int i = 0; i < count; ++i)
                print(c);
        }

        template<typename F>
        constexpr void format(F print, Flags &f, const char *s) {
            if (!s) s = "<null>";

            int pad = f.width ? f.width - (int)str_length(s) : 0;

            if (!f.align_left && pad > 0)
                format_padding(print, ' ', pad);

            print(s);

            if (f.align_left && pad > 0)
                format_padding(print, ' ', pad);
        }

        /// Formatter for characters.
        template<typename F> constexpr void format(F print, Flags &f, char c) {
            if (f.repeat) {
                for (int i = 0; i < f.width; ++i)
                    print(c);
            } else {
                int pad = f.width ? f.width - 1 : 0;
                if (!f.align_left) format_padding(print, ' ', pad);
                print(c);
                if (f.align_left)  format_padding(print, ' ', pad);
            }
        }

        /// Formatter for booleans.
        template<typename F> constexpr void format(F print, Flags &f, bool b) {
            format(print, f, b ? "true" : "false");
        }

        /// Formatter for unsigned numbers.
        template<typename F>
        constexpr void format(F print, Flags f, u64 n, bool sign = false) {
            char buf[2 * 64 + 3] = {}; // max length (binary full 64-bit number + sign and radix).
            int  i          = 0;
            int  prefix_len = 0;

            // For printing K/M/G size units.
            u8 scale = f.scale;
            char size_char = 'B';

            if (f.size) { // Reset radix.
                f.radix     = 10;
                prefix_len +=  2; // not actually a prefix, but who cares.
                if (scale == 0)
                    scale = min((int)logn(1024, n), (int)6);
                if (scale == 0) size_char = 'B';
                if (scale == 1) size_char = 'K';
                if (scale == 2) size_char = 'M';
                if (scale == 3) size_char = 'G';
                if (scale == 4) size_char = 'T';
                if (scale == 5) size_char = 'P';
                if (scale == 6) size_char = 'E';
                n /= pow((u32)1024, (u32)scale);
            }

            if (sign || f.explicit_sign) prefix_len += 1;
            if (f.prefix_radix)          prefix_len += 2;

            auto emit_radix = [&print,f] () {
                print('0');
                print( f.radix == 10 ? 'd'
                  : f.radix == 16 ? 'x'
                  : f.radix ==  8 ? 'o'
                  : f.radix ==  2 ? 'b'
                  : '?');
            };

            auto fmtdigit = [] (int n) {
                if (n < 10) return n + '0';
                else        return n - 10 + 'a';
            };

            if (n) {
                while (n) {
                    u64 x = n % f.radix;
                    u64 y = n / f.radix;
                    buf[i++] = fmtdigit(x);
                    n = y;
                }
            } else {
                buf[i++] = '0';
            }

            int pad = f.width ? f.width - (prefix_len + (int)i) : 0;

            if (pad > 0 && !f.align_left && !f.prefix_zero)
                format_padding(print, ' ', pad);

            if (sign || f.explicit_sign) print(sign ? '-' : '+');
            if (f.prefix_radix)          emit_radix();

            if (pad > 0 && !f.align_left && f.prefix_zero)
                format_padding(print, '0', pad);

            for (--i; i >= 0; --i)
                print(buf[i]);

            if (f.size) {
                print(' ');
                print(size_char);
            }

            if (pad > 0 && f.align_left)
                format_padding(print, ' ', pad);
        }

        /// Formatter for signed numbers.
        template<typename F>
        constexpr void format(F print, Flags &f, s64 n) {
            if (n >= 0)
                 format(print, f, (u64) n);
            else format(print, f, (u64)-n, !f.unsign);
        }

        /// \name Formatters for all numeric types.
        ///@{
        template<typename F> constexpr void format(F print, Flags &f,  s8 n) { format(print, f, (s64)n); }
        template<typename F> constexpr void format(F print, Flags &f, s16 n) { format(print, f, (s64)n); }
        template<typename F> constexpr void format(F print, Flags &f, s32 n) { format(print, f, (s64)n); }
        template<typename F> constexpr void format(F print, Flags &f,  u8 n) { format(print, f, (u64)n); }
        template<typename F> constexpr void format(F print, Flags &f, u16 n) { format(print, f, (u64)n); }
        template<typename F> constexpr void format(F print, Flags &f, u32 n) { format(print, f, (u64)n); }
        ///@}

        /// Formatter for pointers.
        template<typename F>
        constexpr void format(F print, Flags &f, void *a) {
            f.radix        = 16;
            f.prefix_zero  = true;
            f.unsign       = true;

            if constexpr (sizeof(void*) >= 8)
                 f.width = 16;
            else f.width =  8;

            if (f.prefix_radix) f.width += 2;

            format(print, f, (u64)a);
        }

        /// Parses flags and runs a formatting function for each argument.
        template<typename F, typename A, typename... As>
        constexpr void run_format(F print, const char *s, const A &a, const As&... as) {
            bool in_brace = false;
            Flags flags;

            auto reset = [&] () { flags    = Flags{};
                                  in_brace = false; };

            for (; *s; ++s) {
                auto c = *s;
                if (in_brace) {
                    if (c == '}') {
                        format(print, flags, a);
                        fmt(print, s+1, as...);
                        return;
                    } else if (c == '{') { print(c); reset();
                    } else if (c == '0' && !flags.width)
                                         { flags.prefix_zero   = true;
                    } else if (c == 'r' && flags.width > 1)
                                         { flags.radix = flags.width; flags.width = 0;
                    } else if (c >= '0' && c <= '9')
                                         { flags.width         = flags.width * 10 + (c - '0');
                    } else if (c == 'u') { flags.unsign        = true;
                    } else if (c == 'x') { flags.unsign        = true; flags.radix = 16;
                    } else if (c == 'o') { flags.unsign        = true; flags.radix =  8;
                    } else if (c == 'b') { flags.unsign        = true; flags.radix =  2;
                    } else if (c == 'S') { flags.size          = true; flags.scale =  0;
                    } else if (c == 'K') { flags.size          = true; flags.scale =  1;
                    } else if (c == 'M') { flags.size          = true; flags.scale =  2;
                    } else if (c == 'G') { flags.size          = true; flags.scale =  3;
                    } else if (c == 'T') { flags.size          = true; flags.scale =  4;
                    } else if (c == '-') { flags.align_left    = true;
                    } else if (c == '+') { flags.explicit_sign = true;
                    } else if (c == '#') { flags.prefix_radix  = true;
                    } else if (c == '~') { flags.repeat        = true;
                    }
                } else {
                    if (c == '{') in_brace = true;
                    else          print(c);
                }
            }
        }

        /// Wrapper for formatting callbacks.
        template<typename F>
        struct Callback {
            F print;
            void operator()(char c) {
                if constexpr (is_callable<F,char>::value) {
                    print(c);
                } else {
                    char buf[2] = {c, 0};
                    print(buf);
                }
            }
            void operator()(const char *s) {
                if constexpr (is_callable<F,const char*>::value)
                     print(s);
                else while (*s) print(*s++);
            }

            Callback(F print) : print(print) { }
        };
    }

    /// Format a string, pass results to a callback.
    template<typename F, typename... As>
    void fmt(F p, const char *s, const As&... args) {
        if constexpr (sizeof...(As) > 0) {
            Format::run_format(Format::Callback{p}, s, args...);
        } else {
            // No args other than the format string.
            Format::Callback{p}(s);
        }
    }

    /// Format a string, write results into a string.
    template<typename... As>
    void fmt(char *dest, size_t dest_size, const char *s, const As&... args) {
        if constexpr (sizeof...(As) > 0) {
            size_t i = 0;
            fmt([&] (char c) { dest[i++] = c; }, s, args...);
        } else {
            // No args other than the format string.
            str_copy(dest, s, dest_size);
        }
    }
}
