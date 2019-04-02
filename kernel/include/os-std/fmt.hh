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
#include <os-std/memory.hh>
#include <os-std/math.hh>

/**
 * \file
 * String formatting library.
 *
 * fmt takes a format string, containing zero or more bracketed blocks (`{}`),
 * and any number of other arguments. It then parses the blocks for formatting
 * options and prints the arguments in order, each with their own applied
 * formatting options.
 *
 * fmt is recursive, but recursion depth is bounded by the number of arguments
 * passed to it. As such, its max recursion depth is known at compile-time.
 *
 * \todo Expand documentation (examples).
 */

namespace ostd {

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
            bool explicit_sign : 1; ///< (only for numbers)
            bool unsign        : 1; ///< Remove sign bit.
            bool prefix_radix  : 1; ///< Prefix number with 0b, 0x, 0o, or 0d.
            bool align_left    : 1;
            bool prefix_zero   : 1; ///< (only for numbers)
            bool repeat        : 1; ///< (only for chars) fill width by repetition
            bool size          : 1; ///< (only for numbers) format as human-readable K/M/G

            constexpr Flags()
                : explicit_sign (false)
                , unsign        (false)
                , prefix_radix  (false)
                , align_left    (false)
                , prefix_zero   (false)
                , repeat        (false)
                , size          (false)
            { }
        };

        template<typename F>
        constexpr int format_padding(F &print, char c, int count) {
            for (int i = 0; i < count; ++i)
                print(c);
            return count;
        }

        template<typename F>
        constexpr int format(F &print, Flags &f, const char *s) {
            if (!s) s = "<null>";

            int count = 0;

            int pad = f.width ? f.width - (int)str_length(s) : 0;

            if (!f.align_left && pad > 0)
                count += format_padding(print, ' ', pad);

            count += str_length(s);
            print(s);

            if (f.align_left && pad > 0)
                count += format_padding(print, ' ', pad);

            return count;
        }

        /// Formatter for characters.
        template<typename F> constexpr int format(F &print, Flags &f, char c) {
            if (f.repeat) {
                for (int i = 0; i < f.width; ++i)
                    print(c);
                return f.width;
            } else {
                int count = 1;
                int pad = f.width ? f.width - 1 : 0;
                if (!f.align_left) count += format_padding(print, ' ', pad);
                print(c);
                if (f.align_left)  count += format_padding(print, ' ', pad);
                return count;
            }
        }

        /// Formatter for booleans.
        template<typename F> constexpr int format(F &print, Flags &f, bool b) {
            return format(print, f, b ? "true" : "false");
        }

        /// Formatter for unsigned numbers.
        template<typename F>
        constexpr int format(F &print, Flags f, u64 n, bool sign = false) {
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

            auto fmtdigit = [] (int m) {
                if (m < 10) return m + '0';
                else        return m - 10 + 'a';
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

            int count = int(i) + prefix_len;
            int pad = f.width ? f.width - count : 0;

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

            return count + pad;
        }

        /// Formatter for signed numbers.
        template<typename F>
        constexpr int format(F &print, Flags &f, s64 n) {
            if (n >= 0)
                 return format(print, f, (u64) n);
            else return format(print, f, (u64)-n, !f.unsign);
        }

        /// \name Formatters for all numeric types.
        ///@{
        template<typename F> constexpr int format(F &print, Flags &f,  s8 n) { return format(print, f, (s64)n); }
        template<typename F> constexpr int format(F &print, Flags &f, s16 n) { return format(print, f, (s64)n); }
        template<typename F> constexpr int format(F &print, Flags &f, s32 n) { return format(print, f, (s64)n); }
        template<typename F> constexpr int format(F &print, Flags &f,  u8 n) { return format(print, f, (u64)n); }
        template<typename F> constexpr int format(F &print, Flags &f, u16 n) { return format(print, f, (u64)n); }
        template<typename F> constexpr int format(F &print, Flags &f, u32 n) { return format(print, f, (u64)n); }
        ///@}

        /// Formatter for pointers.
        template<typename F, typename T>
        constexpr int format(F &print, Flags &f, const T *a) {
            f.radix        = 16;
            f.prefix_zero  = true;
            f.unsign       = true;

            if constexpr (sizeof(void*) >= 8)
                 f.width = 16;
            else f.width =  8;

            if (f.prefix_radix) f.width += 2;

            return format(print, f, (u64)a);
        }

        struct ParseResult {
            int write;      ///< How many chars to write from the format string.
            int skip;       ///< How many chars to skip after that.
            bool write_arg; ///< Whether to write an argument after that.
            Flags flags;    ///< The flags to use for printing the next argument.
        };

        /// Parses flags for at most one argument.
        constexpr ParseResult parse_one(const char *s) {
            ParseResult res { };

            bool in_brace = false;
            auto &flags = res.flags;

            for (; *s; ++s) {
                auto c = *s;
                if (in_brace) {
                    res.skip++;
                           if (c == '}') { res.write_arg = true;      return res;
                    } else if (c == '{') { res.write++; res.skip = 1; return res;
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
                    if (c == '{') in_brace = true, res.skip++;
                    else          res.write++;
                }
            }
            return res;
        }

        struct FormatOneResult { int read, written; };

        /// Parses flags and runs formatting for one argument.
        template<typename F, typename A>
        constexpr FormatOneResult format_one(F &print, const char *s, const A &a) {

            int read    = 0; ///< The amount of characters read from s.
            int written = 0; ///< The amount of characters printed.

            while (*s) {
                auto res = parse_one(s);
                for (int i = 0; i < res.write; ++i, ++s)
                    print(*s);
                s += res.skip;

                read    += res.write + res.skip;
                written += res.write;

                if (res.write_arg) {
                    written += format(print, res.flags, a);
                    break;
                }
            }
            return { read, written };
        }

        /// Parses and formats everything.
        template<typename F, typename A, typename... As>
        constexpr int run_format(F &print, const char *s, const A &a, const As&... as) {

            auto [read, written] = format_one(print, s, a);

            if constexpr (sizeof...(as) > 0) {
                if (!s) return written;
                else    return written + run_format(print, s + read, as...);
            } else {
                print(s + read);
                return written + str_length(s + read);
            }
        }

        /// Callback for writing to strings.
        struct StringPrinter {
            char  *dest;
            size_t dest_size;

            constexpr void operator()(char c) {
                if (dest_size >= 2) {
                    // Add a char and insert a NUL terminator right after (to
                    // be overwritten on the next print).
                    *dest++ = c;
                    *dest   = 0;
                    dest_size--;
                }
            }

            constexpr void operator()(const char *s) {
                int written = str_copy(dest, s, dest_size);
                dest       += written;
                dest_size  -= written;
            }
        };

        /// Wrapper for callbacks that take only chars or only strings.
        template<typename P>
        struct CallbackWrapper {
            P &print;

            constexpr void operator()(char c) {
                if constexpr (is_callable<P,char>::value) {
                    print(c);
                } else {
                    char buf[2] = {c, 0};
                    print(buf);
                }
            }

            constexpr void operator()(const char *s) {
                // Pick the right printer at compile-time.

                if constexpr (is_callable<P,const char*>::value)
                     print(s);
                else while (*s) print(*s++);
            }

            CallbackWrapper(P &print_)
                : print(print_) { }
        };
    }

    /**
     * Format a string, pass results to a callback.
     *
     * The callback print is a function that will be called repeatedly with
     * parts of the formatted string.
     *
     * Callback print can be either a function that takes one of `char` or
     * `const char*`, or it may be a callable object that has overloads for one
     * or both of those types.
     *
     * \return the amount of characters written
     */
    template<typename F, typename... As>
    constexpr int fmt(F &print, const char *s, const As&... args) {
        // Is this callback sophisticated enough?
        if constexpr (is_callable<F,char>::value
                   && is_callable<F,const char*>::value) {

            if constexpr (sizeof...(As) > 0) {
                return Format::run_format(print, s, args...);
            } else {
                // No args other than the format string, bypass the parser.
                print(s);
                return str_length(s);
            }
        } else {
            // Callback does not handle both chars and strings, wrap it.
            Format::CallbackWrapper w {print};
            return fmt(w, s, args...);
        }
    }

    /**
     * Format a string, pass results to a callback.
     *
     * The callback print is a function that will be called repeatedly with
     * parts of the formatted string.
     *
     * Callback print can be either a function that takes one of `char` or
     * `const char*`, or it may be a callable object that has overloads for one
     * or both of those types.
     *
     * \return the amount of characters written
     */
    template<typename F, typename... As>
    constexpr int fmt(F &&print, const char *s, const As&... args) {
        F p2(print); return fmt(p2, s, args...);
    }

    /**
     * Format a string, write results into a string.
     *
     * This writes up to dest_size-1 characters into dest, and always writes a
     * NUL byte at the end.
     *
     * \return the amount of characters written
     */
    template<typename... As>
    constexpr int fmt(char *dest, size_t dest_size, const char *s, const As&... args) {
        if constexpr (sizeof...(As) > 0) {
            auto sp = Format::StringPrinter{dest, dest_size};
            return fmt(sp, s, args...);
        } else {
            // No args other than the format string,
            // simply copying the string is faster.
            return str_copy(dest, s, dest_size);
        }
    }
}
