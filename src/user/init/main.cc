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
#include <io.hh>
#include <proc.hh>
#include <os-std/ostd.hh>

using namespace ostd;

int main(int, const char**) {
    // (arguments are ignored)

    // Open stdin/out/err. These will be inherited by processes we spawn.
    // If a serial port is available, use it for all I/O.
    errno_t err = open(stdin , "/dev/uart0", "r");
    if (err >= 0) {
        // OK.
        err = open(stdout, "/dev/uart0", "w");
        if (err < 0) return 1;

    } else if (err == ERR_not_exists) {
        // TODO: Provide a tty driver. Open a pipe to it here.
        // Serial port not available, use keyboard+monitor instead.
        err = open(stdin , "/dev/keyboard",   "r"); if (err < 0) return 1;
        err = open(stdout, "/dev/video-text", "w"); if (err < 0) return 1;

        // This pretends to be succesful, but we actually can't use video-text
        // as a console for userland currently. We depend on a working UART.

    } else {
        return 1;
    }

    // stderr should appear in the same place as stdout.
    err = duplicate_fd(stderr, stdout);
    if (err < 0) return 1;

    // Note: The current working directory should be the root of the first
    // mounted partition. We look for a shell in its "/bin" directory.

    Array<StringView, 1> args { "shell" };
    err = spawn("bin/shell.elf", args, true);

    if (err < 0) {
        print(stderr, "init: could not load the shell: {}\n", error_name(err));
        return 1;
    }

    return 0;
}
