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
#include <os-std/types.hh>

/// Handler for pure virtual function calls.
extern "C" void __cxa_pure_virtual();
extern "C" void __cxa_pure_virtual() {
    // Simply hang the machine, this shouldn't happen anyway.
    asm volatile ("cli \n hlt");
}

/// This isn't necessary since the kernel never exits.
extern "C" void __cxa_atexit(void (*)(void*), void*, void*);
extern "C" void __cxa_atexit(void (*)(void*), void*, void*) { }

/// \name These are required by (clang) builtins
///@{
extern "C" void *memcpy(void *__restrict dst, const void *__restrict src, size_t n);
extern "C" void *memcpy(void *__restrict dst, const void *__restrict src, size_t n) {
    for (size_t i = 0; i < n; ++i)
        ((u8*)dst)[i] = ((u8*)src)[i];
    return dst;
}

extern "C" void *memset(void *s, char c, size_t n);
extern "C" void *memset(void *s, char c, size_t n) {
    for (size_t i = 0; i < n; ++i)
        ((char*)s)[i] = c;
    return s;
}
///@}
