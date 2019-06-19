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

int main(int argc, const char **argv) {

    if (argc < 2) {
        print(stderr, "usage: cat <file>...\n");
        return 1;
    }

    Array<char, 512> buffer;

    for (int i : range(1, argc)) {

        fd_t fd = open(argv[i], "r");
        if (fd < 0) {
            print(stderr, "could not open <{}> for reading: {}\n", argv[i], error_name(fd));
            return 1;
        }

        // Copy fd to stdout.
        while (true) {
            ssize_t n = read(fd, buffer.data(), buffer.size());
            if (n <= 0) {
                if (n != 0) // not EOF?
                    print(stderr, "read failed: {}\n", error_name(n));
                close(fd);
                break; // next file.
            }

            n = write(stdout, buffer.data(), n);
            if (n < 0) {
                print(stderr, "write failed: {}\n", error_name(n));
                return 1;
            }
        }
        close(fd);
    }

    return 0;
}
