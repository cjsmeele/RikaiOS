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
#include "kprint.hh"
#include "../console/textmode.hh"
#include "../console/serial.hh"
#include "interrupt/interrupt.hh"

using namespace ostd;

void kprint_char(char c) {
    // Try to print to whatever console is available.

    enter_critical_section();

    Console::TextMode::print_char(c);
    Console::Serial  ::print_char(c);

    leave_critical_section();
}

void kprint_init() {
    Console::TextMode::init();
    Console::Serial  ::init();
}
