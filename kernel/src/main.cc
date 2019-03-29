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

extern "C"
void kmain(const boot_info_t &boot_info) {

    Gdt::init();

    for (int y = 0; y < 25; ++y) {
        for (int x = 0; x < 80; ++x) {
            ((volatile u16*)0xb8000)[y*80+x]
                = (x+y)&1 ? 0x8f00 | ' '
                          : 0x7f00 | ' ';
        }
    }

    panic();
}
