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

#include "types.hh"
#include "../include/boot/bootinfo.hh"

// C++ functions to wrap BIOS calls.

struct boot_info_t;

// Misc. functions.
void asm_hang();
void asm_boot(boot_info_t &boot_info);

// Console functions.
void asm_console_print_char(char c);
void asm_console_clear();

// Serial I/O functions.
bool asm_serial_is_available();
void asm_serial_init();
void asm_serial_print_char(char c);

// Memory functions.
bool asm_get_memory_map(u64  &start
                       ,u64  &size
                       ,bool &available
                       ,u32  &cont);
void asm_mem_clear_bss();
void asm_mem_enable_high_memory();

// Disk I/O functions.

// NB: dest_addr must be between 0 and 0x10000 (the first 64K of memory).
//     any static / stack buffer should be fine as long as you don't make it too big.
//     (note also that the amount of blocks to read per call is limited
//      to 127 - the amount of blocks that fits in one 64K segment)
bool asm_disk_read(void *dest
                  ,u8    disk_no
                  ,u32   block
                  ,s8    block_count);

// Debug functions.

// This triggers a "magic" breakpoint in the Bochs emulator.
#define DEBUG_BREAK() asm volatile ("xchg %bx, %bx")

// Same, but breaks with a given value in register EAX.
// (poor-man's printf debugging)
#define DEBUG_EAX(x)  asm volatile ("xchg %%bx, %%bx" \
                                  :: "a" (u32(x)), "b" (0xfeedbeef))
