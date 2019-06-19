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
#pragma once

#include "common.hh"
#include "ipc/queue.hh"

/**
 * Kernel built-in debug shell.
 *
 * This provides a minimal command-line shell for debugging purposes.
 *
 * To use the shell, attach a serial cable (or use your host terminal when
 * running with QEMU) and press ESC in your serial terminal.
 *
 * Use the `help` command to get a list of available shell commands.
 */
namespace Kshell {

    /// Input characters. Read by kshell, written by the serial/uart driver.
    extern KQueue<char, 64> input;

    /// Pauses all userspace threads and enables the kernel shell on the serial port.
    /// (there is no disable - the shell can only be disabled using shell commands)
    void enable();

    /// Ask if the shell is running.
    bool enabled();

    /// Ask if UART and keyboard input should be redirected to the kshell input buffer.
    bool want_all_input();

    /// kshell thread function.
    void kshell();
}
