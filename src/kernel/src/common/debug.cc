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
#include "debug.hh"
#include "panic.hh"
#include "asm.hh"
#include "kprint.hh"
#include <os-std/type-traits.hh>
#include <os-std/string.hh>

using namespace ostd;

void do_assert(bool test, const char *error) {
    if (!test)
        panic("assertion failed:\n  {}\n", error);
}

int rand() {
    // Generate a random number using a Linear Congruential Generator (LCG).

// #define RANDOM_SEED 0
#ifdef RANDOM_SEED
    // Allow choosing a fixed random seed (may be useful for debugging).
    static u32 seed = RANDOM_SEED;
#else
    // Take a seed from the CPU's timestamp counter.
    static u32 seed = ~asm_rdtsc32();
#endif

    // Use the multiplier & incrementor as used in ISO/IEC 9899:201x.
    return (seed = seed * 1103515245 + 12345)
           & ostd::intmax<int>::value;
}

void hex_dump(void *p_, size_t size, bool as32) {
    u8 *p = (u8*)((addr_t)p_ & ~0xf);

    for (size_t i : range(0, size + ((u8*)p_ - p), 16)) {

        String<16> str { };
        for (size_t j : range(16))
            str += p[i+j] >= ' ' && p[i+j] <= '~' ? p[i+j] : '.';

        if (as32) {
            kprint("{08x}: {08x} {08x} {08x} {08x} {}\n"
                  ,p+i
                  ,((u32*)(void*)(p+i))[0]
                  ,((u32*)(void*)(p+i))[1]
                  ,((u32*)(void*)(p+i))[2]
                  ,((u32*)(void*)(p+i))[3]
                  ,str);
        } else {
            kprint("{08x}:"
                   " {02x}{02x}{02x}{02x}"
                   " {02x}{02x}{02x}{02x}"
                   " {02x}{02x}{02x}{02x}"
                   " {02x}{02x}{02x}{02x}"
                   " {}\n"
                  ,p+i
                  ,p[i+ 0],p[i+ 1],p[i+ 2],p[i+ 3]
                  ,p[i+ 4],p[i+ 5],p[i+ 6],p[i+ 7]
                  ,p[i+ 8],p[i+ 9],p[i+10],p[i+11]
                  ,p[i+12],p[i+13],p[i+14],p[i+15]
                  ,str);
        }
    }
}
