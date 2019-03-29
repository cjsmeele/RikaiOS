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

extern "C"
void kmain(const boot_info_t &boot_info) {

    Gdt::init();

    for (int y = 0; y < 25; ++y) {
        for (int x = 0; x < 80; ++x) {
            ((volatile u16*)0xb8000)[y*80+x]
                = 0x7020;
                // = x&1 ? 0x8700 | 0xdf
                //       : 0x7800 | 0xdf;
        }
    }

    String<80*25> str = "formatted text:\n\n";

    {
        String<80> tmp;

        fmt(tmp, "{8   } {8   } {8 } {8 }\n" , "hex", "oct", "dec", "bin"); str += tmp;
        fmt(tmp, "{8~  } {8~  } {8~} {8~}\n" , '-', '-', '-', '-');         str += tmp;
        fmt(tmp, "{08#x} {08#o} {8 } {8 }\n" , 42, 42, 42, Bitset<8>(42));  str += tmp;

        auto ar = Array{1,22,333,4444,999999};

        fmt(tmp, "\nThis is an array:       {   }\n", ar); str += tmp;
        fmt(tmp,   "This is the same array: {#7x}\n", ar); str += tmp;
        fmt(tmp,   "This is the same array: {07 }\n", ar); str += tmp;
        fmt(tmp,   "This is the same array: { 7S}\n", ar); str += tmp;
    }

    int x = 0, y = 0;
    for (char c : str) {
        if (c == '\n') {
            x = 0, y++;
        } else {
            ((volatile u16*)0xb8000)[y*80+(x++)] = 0x0f00 | c;
        }
    }

    panic();
}
