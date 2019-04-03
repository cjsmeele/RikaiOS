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
#include "panic.hh"
#include "debug.hh"
#include <os-std/string.hh>
#include "kprint.hh"
#include "asm.hh"

using namespace ostd;

void panic(StringView reason) {
    DEBUG1(0xdeaddead);

    kprint("\n*** kernel panic ***\n");
    if (reason.length()) {
        kprint(reason);
        kprint("\n*** kernel panic ***\n");
    }

    // Print "PANIC" in the upper right corner of the screen.
    // (note: this assumes the vga is in default 80x25 textmode.
    constexpr StringView str = " PANIC! ";

    for (size_t i = 0; i < str.length(); ++i)
        ((volatile u16*)0xb8000)[80-str.length()+i]
            = str[i] | 0x4f00;

    // Hang the machine.
    asm_hang();
}
