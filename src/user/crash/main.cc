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
#include <os-std/ostd.hh>

using namespace ostd;

// Intentionally cause a userland crash to see how well the kernel handles
// misbehaving processes.

static void usage(const char *progname) {
    print(stderr, "usage: {} <pf|gpf|ill|div0>\n", progname);
    print(stderr, "pf  = page fault,          gpf  = general protection fault\n"
                  "ill = illegal instruction, div0 = divide by zero\n");
    print(stderr, "\nthis program should crash without bringing down the whole OS\n");
}

int main(int argc, const char **argv) {

    if (argc != 2) {
        usage(argv[0]);
        return 0;
    }

    if (StringView(argv[1]) == "pf") {
        // Cause page fault by writing to unmapped memory.
        *(volatile u8*)0 = 1;

    } else if (StringView(argv[1]) == "gpf") {
        // Cause general protection fault by using a privileged instruction.
        asm volatile ("sti");

    } else if (StringView(argv[1]) == "ill") {
        // Cause illegal opcode fault by ... you get the gist.
        asm volatile ("ud2");

    } else if (StringView(argv[1]) == "div0") {
        volatile int x = 0;
        volatile int y = 42 / x;
    } else {
        usage(argv[0]);
        return 0;
    }

    print("I shouldn't be alive!\n");

    return 1;
}
