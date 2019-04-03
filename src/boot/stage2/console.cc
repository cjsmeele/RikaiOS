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
#include "console.hh"
#include "asm.hh"

bool serial_enabled = false;

void init_console() {
    // If a serial line is available, output both to the screen and to the serial line.
    if (asm_serial_is_available()) {
        asm_serial_init();
        serial_enabled = true;
    }
}

void clear_console() {
    asm_console_clear();
}

void putch(char ch) {
    if (serial_enabled)
         asm_serial_print_char(ch);

    asm_console_print_char(ch);
}

void puts(const char *s) {
    while (*s) {
        if (*s == '\n')
            putch('\r');
        putch(*(s++));
    }
}
