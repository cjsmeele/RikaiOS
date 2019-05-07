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
#include "uart.hh"
#include "../interrupt/handlers.hh"
#include "../console/serial.hh"
#include "../debug-keys.hh"

DRIVER_NAME("uart");

namespace Driver::Uart {

    using namespace Interrupt;

    Queue<char, 16> queue;

    static void irq_handler(const interrupt_frame_t &) {
        char ch = Io::in_8(0x3f8);

        handle_debug_key(ch);

        if (!queue.enqueue(ch)) {
            dprint("input queue full, char dropped\n");
        }
    }

    void init() {

        // The serial port, if it exists, should have already been initialized
        // by Console::Serial.

        if (Console::Serial::exists()) {
            Handler::register_irq_handler(4, irq_handler);
            dprint("uart0: initialised\n");
        }
    }
}
