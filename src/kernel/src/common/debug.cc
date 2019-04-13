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
#include <os-std/type-traits.hh>

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
