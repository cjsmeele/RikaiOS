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

/// Generates a "magic" breakpoint in the Bochs emulator.
#define DEBUG_BREAK() asm volatile ("xchg %%bx, %%bx" :: "a" (0xdeba9000));

/**
 * \name Poor-man's printf debugging.
 *
 * This loads magic values in eax..edx and pauses Bochs,
 * useful for quickly inspecting variables.
 *
 * This has no effect whatsoever on QEMU and real hardware.
 */
///@{
#define DEBUG1(x)     asm volatile ("xchg %%bx, %%bx" :: "a" (0xdeba9001) \
                                                       , "b" (u32(x)))
#define DEBUG2(x,y)   asm volatile ("xchg %%bx, %%bx" :: "a" (0xdeba9002) \
                                                       , "b" (u32(x))     \
                                                       , "c" (u32(y)))
#define DEBUG3(x,y,z) asm volatile ("xchg %%bx, %%bx" :: "a" (0xdeba9003) \
                                                       , "b" (u32(x))     \
                                                       , "c" (u32(y))     \
                                                       , "d" (u32(z)))
///@}

/**
 * \name Assertion macro
 *
 * Causes a kernel panic when a given condition is not fulfilled.
 *
 * Usage:
 *
 *     assert(1 > 0, "math is broken!");
 *
 * (the C macros below are needed in order to insert the source file name and
 * line number into the error message)
 */
///@{

void do_assert(bool test, const char *error);

#define ASSERT_STRINGIFY2(x) #x
#define ASSERT_STRINGIFY(x) ASSERT_STRINGIFY2(x)
#define assert(expr, err) \
    do_assert((expr), __FILE__ ":" ASSERT_STRINGIFY(__LINE__) ": { " #expr " } - " err)

///@}

/// A PRNG, useful for testing functions with semi-random arguments.
int rand();

/**
 * Dump a region of memory.
 *
 * p_ is rounded down to the nearest 16-byte boundary.
 */
void hex_dump(void *p_, size_t size, bool as32 = false);
