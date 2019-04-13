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

namespace ostd {

    constexpr inline size_t page_size = 4096; // 4K

    template<typename T>
    constexpr T *mem_set(T *s, const T &c, size_t n) {
        for (size_t i = 0; i < n; ++i)
            s[i] = c;
        return s;
    }

    template<typename T>
    constexpr T *mem_copy(T *dst, const T *src, size_t n = 1) {
        for (size_t i = 0; i < n; ++i)
            dst[i] = src[i];
        return dst;
    }

    template<typename T>
    constexpr T *mem_move(T *dst_, const T *src_, size_t n = 1) {
        u8 *dst = (u8*)dst_;
        u8 *src = (u8*)src_;
        n = n * sizeof(T);

        if (dst < src) {
            for (size_t i = 0; i < n; ++i)
                dst[i] = src[i];
        } else {
            for (size_t i = 0; i < n; ++i)
                dst[n - i - 1] = src[n - i - 1];
        }
        return dst_;
    }

    template<typename T>
    constexpr bool mem_eq(const T *x, const T *y, size_t n = 1) {
        for (size_t i = 0; i < n; i++) {
            if (x[i] != y[i])
                return false;
        }
        return true;
    }

    constexpr size_t str_length(const char *s) {
        size_t i = 0;
        for (; s[i]; ++i);
        return i;
    }

    /**
     * Copies at most dst_size bytes or until a NUL terminator was found.
     *
     * A terminating NUL byte is always written as long as dst_size > 0.
     * dst_size must include space for a NUL byte.
     *
     * \return the amount of characters copied (excluding the NUL byte)
     */
    constexpr size_t str_copy(      char *__restrict dst
                             ,const char *__restrict src
                             ,size_t dst_size) {
        if (dst_size == 0)
            return 0;

        size_t i = 0;
        for (; i < dst_size-1 && src[i]; ++i)
            dst[i] = src[i];

        dst[i] = 0;

        return i;
    }
}

extern "C" void *memcpy(void *__restrict dst, const void *__restrict src, size_t sz);
extern "C" void *memset(void *dst, char c, size_t sz);
