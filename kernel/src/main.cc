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

#include "boot/bootinfo.hh"

using u16 = unsigned short;
using u32 = unsigned int;

// poor-man's printf debugging
#define DEBUG_EAX(x)  asm volatile ("xchg %%bx, %%bx" \
                                  :: "a" (u32(x)), "b" (0xfeedbeef))

extern "C"
void kmain(const boot_info_t &boot_info) {

    for (int y = 0; y < 25; ++y) {
        for (int x = 0; x < 80; ++x) {
            ((volatile u16*)0xb8000)[y*80+x]
                = (x+y)&1 ? 0xf101 : 0x1f02;
        }
    }

    DEBUG_EAX(boot_info.memory_region_count);
    DEBUG_EAX(boot_info.memory_regions[0].size);
}
