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

    if (argc != 3) {
        print(stderr, "usage: cp <src> <dst>\n");
        return 1;
    }

    fd_t src = open(argv[1], "r");
    if (src < 0) {
        print(stderr, "could not open src for reading: {}\n", error_name(src));
        return 1;
    }
    fd_t dst = open(argv[2], "w");
    if (dst < 0) {
        close(src);
        print(stderr, "could not open dst for writing: {}\n", error_name(dst));
        return 1;
    }

    Array<char, 512> buffer;
    while (true) {
        ssize_t n = read(src, buffer.data(), buffer.size());
        if (n <= 0) {
            if (n != 0)
                print(stderr, "read failed: {}\n", error_name(n));
            break;
        }
        n = write(dst, buffer.data(), n);
        if (n < 0) {
            print(stderr, "write failed: {}\n", error_name(n));
            break;
        }
    }
    close(dst);
    close(src);

    return 0;
}
