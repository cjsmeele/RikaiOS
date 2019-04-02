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
#include "common.hh"
#include "boot/bootinfo.hh"
#include "memory/memory.hh"
#include "interrupt/interrupt.hh"

/**
 * \file
 * Contains the kernel's main function.
 */

/// This is the C++ kernel entrypoint (run right after start.asm).
extern "C" void kmain(const boot_info_t &boot_info);
extern "C" void kmain(const boot_info_t &boot_info) {

    // Make sure we can write to the console.
    kprint_init();

    Interrupt::init();
    Memory::init();

    kprint("\neos-os is booting.\n");
    kprint("\nmemory map:\n");

    for (size_t i = 0; i < boot_info.memory_region_count; ++i) {
        auto &region = boot_info.memory_regions[i];
        if (region.start < intmax<u32>::value) {
            kprint("  {} - {}  {6S} free\n"
                  ,(u8*)region.start
                  ,(u8*)region.start + (region.size-1)
                  ,region.size);
        }
    }

    kprint("\n(nothing to do - press ESC to crash and burn)\n\n");

    // XXX temporary - sets PIT frequency to 1 KHz.
    Io::out_8(0x43, 0x34);            Io::wait();
    Io::out_8(0x40, (1*1193) & 0xff); Io::wait();
    Io::out_8(0x40, (1*1193) >> 8);

    Interrupt::enable();
    for (int i = 0;; ++i) {
        if (i % 50 == 0) kprint(".");
        asm_hlt();
    }

    panic("reached end of kmain");
}
