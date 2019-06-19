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
    errno_t err;
    err = open(stdin , "/dev/uart0", "r"); if (err < 0) return 1;
    err = open(stdout, "/dev/uart0", "w"); if (err < 0) return 1;
    err = duplicate_fd(stderr, stdout);    if (err < 0) return 1;

    print("init\n");

    Array<StringView, 1> args { "shell" };
    int n = spawn("shell.elf", args, true);

    return 0;
}
