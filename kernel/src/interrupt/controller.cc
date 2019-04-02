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
#include "controller.hh"

/// \file
/// \todo magic numbers, documentation.

namespace Interrupt::Controller {

    constexpr u16 master_port = 0x20;
    constexpr u16  slave_port = 0xa0;

    constexpr u8 cmd_eoi  = 0x20; ///< End Of Interrupt.
    constexpr u8 cmd_init = 0x10;

    void acknowledge_interrupt(u8 int_no) {
        if (int_no >= 0x28 && int_no < 0x30)
            // Acknowledge interrupt to slave controller.
            Io::out_8(slave_port, cmd_eoi);

        Io::out_8(master_port, cmd_eoi);
    }

    void init() {
        // Initialise interrupt controller.
        Io::out_8(master_port, cmd_init | 0x01); // init + ICW4
        Io::out_8(slave_port,  cmd_init | 0x01);

        // Remap interrupts for protected mode (unexciting x86 legacy stuff).
        Io::out_8(master_port + 1, 0x20); // master offset.
        Io::out_8(slave_port  + 1, 0x30); // slave offset.

        Io::out_8(master_port + 1, 0x04);
        Io::out_8(slave_port  + 1, 0x02);

        Io::out_8(master_port + 1, 0x01);
        Io::out_8(slave_port  + 1, 0x01);

        Io::out_8(master_port + 1, 0); // Clear IRQ mask.
        Io::out_8(slave_port  + 1, 0);
    }
}
