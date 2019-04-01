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
#include "memory/gdt.hh"

#include <os-std/fmt.hh>

static void print(StringView s) {

    static int x = 0;
    static int y = 0;

    for (char c : s) {
        if (c == '\n' || x >= 80) {
            x = 0, y++;
            if (y >= 25)
                y = 0;
        } else {
            ((volatile u16*)0xb8000)[y*80+(x++)] = 0x0700 | c;
        }
    }
}


static void cls() {
    for (int y = 0; y < 25; ++y) {
        for (int x = 0; x < 80; ++x) {
            ((volatile u16*)0xb8000)[y*80+x] = 0x0720;
        }
    }
}

extern "C" void kmain(const boot_info_t &boot_info);
extern "C" void kmain(const boot_info_t &boot_info) {

    Gdt::init();

    cls();

    fmt(print, "formatted text:\n\n");

    fmt(print, "{8   } {8   } {8 } {8 }\n" , "hex", "oct", "dec", "bin");
    fmt(print, "{8~  } {8~  } {8~} {8~}\n" , '-', '-', '-', '-');
    fmt(print, "{08#x} {08#o} {8 } {8 }\n" , 42, 42, 42, Bitset<8>(42));

    auto ar = Array{1,22,333,4444,999999};

    fmt(print, "\nThis is an array:       {   }\n", ar);
    fmt(print,   "This is the same array: {#7x}\n", ar);
    fmt(print,   "This is the same array: {07 }\n", ar);
    fmt(print,   "This is the same array: { 7S}\n", ar);

    fmt(print, "\nMemory map:\n");

    for (size_t i = 0; i < boot_info.memory_region_count; ++i) {
        auto &region = boot_info.memory_regions[i];
        if (region.start < intmax<u32>::value) {
            fmt(print, "{} - {} {6S} free\n"
               ,(u8*)region.start
               ,(u8*)region.start + (region.size-1)
               ,region.size);
        }
    }

    panic();
}
