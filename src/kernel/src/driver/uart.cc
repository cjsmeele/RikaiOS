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
#include "filesystem/vfs.hh"
#include "ipc/queue.hh"
#include "process/proc.hh"

#include "../kshell.hh"

DRIVER_NAME("uart");

namespace Driver::Uart {

    using namespace Interrupt;

    KQueue<char, 32> uart_input;

    static void irq_handler(const interrupt_frame_t &) {
        char ch = Io::in_8(0x3f8);

        if (Kshell::want_all_input()) {
            if (!Kshell::input.try_enqueue(ch))
                dprint("kshell input queue full, char dropped\n");
        } else if (!Kshell::enabled() && ch == 0x1b) {
            // ESC pressed.
            Kshell::enable();
        } else {
            // handle_debug_key(ch);
            uart_input.try_enqueue(ch);
        }
    }

    struct uart_dev_t : public DevFs::device_t {

        ssize_t read (u64, void *buffer_, size_t nbytes) override {

            char  *buffer  = (char*)buffer_;
            size_t written = 0;

            while (written < nbytes) {
                char c = uart_input.dequeue();
                // Translate carriage return to newline.
                if (c == '\r')   c = '\n';
                if (c == '\x7f') c = '\b';

                if (written == 0 && c == ascii_ctrl('D')) {
                    // ^D, or EOT, indicates end-of-file.
                    // (if encountered in the middle of a line, the character is passed on as-is)
                    return 0;
                }

                // FIXME: For lack of a TTY driver, we currently echo UART input here.
                kprint("{}", c);

                buffer[written++] = c;
                if (c == '\n')
                    // Automatic line buffering: Return early on newline.
                    return written;
            }

            return written;
        }

        ssize_t write(u64, const void *buffer_, size_t nbytes) override {
            u8 *buffer = (u8*)buffer_;
            size_t written = 0;

            while (written < nbytes) {

                // Try to print at most 256 bytes before yielding to another thread.
                // (ideally, we would use a separate thread and a queue for output)
                if (written && written == 256)
                    Process::yield();

                bool ready = false;
                for (size_t i : range(1000)) {
                    // Read "Transmit Holding Register Empty" register.
                    if (Io::in_8(0x3f8 + 5) & 0x20) {
                        ready = true;
                        break;
                    }
                }
                if (!ready) return ERR_timeout;

                Io::out_8(0x3f8, buffer[written++]);
            }
            return written;
        }
        s64 size() override { return 0; }
    };
    static uart_dev_t uart_dev;

    void init() {

        // The serial port, if it exists, should have already been initialized
        // by Console::Serial.

        if (Console::Serial::exists()) {
            Handler::register_irq_handler(4, irq_handler);
            dprint("uart0: initialised\n");

            DevFs *devfs = Vfs::get_devfs();
            if (devfs) devfs->register_device("uart0", uart_dev, 0600);
        }
    }
}
