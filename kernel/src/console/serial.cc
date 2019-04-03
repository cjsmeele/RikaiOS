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
#include "serial.hh"

/// \file
/// \todo magic numbers.

namespace Console::Serial {

    /// IO port number of the first serial port.
    static u16 port_number = 0;

    static bool can_write() {
        if (!port_number)
            return false;

        // Read "Transmit Holding Register Empty" register.
        return Io::in_8(port_number + 5) & 0x20;
    }

    void print_char(char c) {
        if (c == '\n')
            print_char('\r');

        if (port_number) {
            // Busy wait till Transmit Holding Register Empty.
            while (!can_write())
                Io::wait();
            Io::out_8(port_number, static_cast<u8>(c));
        }
    }

    void init() {

        // Check the BIOS Data Area for the I/O port number of the first serial port.
        port_number = *((volatile u16*)0x0400);

        if (port_number) {
            // Port number found, try to initialize the hardware.

            Io::out_8(port_number + 1, 0x01); // Enable Received Data Available interrupt
            Io::out_8(port_number + 3, 0x80); // Enable DLAB to set baud rate
            Io::out_8(port_number + 0, 0x01); // Divisor LSB (115200 baud)
            Io::out_8(port_number + 1, 0x00); // Divisor MSB
            Io::out_8(port_number + 3, 0x03); // 8 bits, no parity, one stop bit
            // Io::out_8(port_number + 2, 0x07); // enable & clear FIFOs, with 1-byte threshold
            Io::out_8(port_number + 2, 0x06); // Disable & clear FIFOs
            Io::out_8(port_number + 4, 0x08); // Enable aux2 pin for interrupts
        }
    }
}
