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

static void more_file(fd_t fd) {

    int row = 0;
    int col = 0;

    Array<char, 512> buffer;

    // Copy fd to stdout.
    while (true) {
        ssize_t n = read(fd, buffer.data(), buffer.size());
        if (n <= 0) {
            if (n != 0)
                print(stderr, "read failed: {}\n", error_name(n));
            return; // next file.
        }

        for (size_t i : range(n)) {
            if (buffer[i] != '\n')
                ++col;
            if (buffer[i] == '\n' || col == 80)
                col = 0, ++row;

            putchar(buffer[i]);

            if (row == 23) {
                if (buffer[i] != '\n') putchar('\n');
                StringView str = "-- press enter to continue --\n";
                print(str);
                getchar();
                col = row = 0;
            }
        }
    }
}

int main(int argc, const char **argv) {

    if (argc < 2) {
        print(stderr, "usage: more <file...>\n");
        return 0;
    }

    for (int i : range(1, argc)) {
        fd_t fd = open(argv[i], "r");
        if (fd < 0) {
            print(stderr, "could not open <{}> for reading: {}\n", argv[i], error_name(fd));
            continue;
        }
        more_file(fd);
        close(fd);
    }

    return 0;
}
