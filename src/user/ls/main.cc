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

/// Creates a "drwx------"-like mode string.
static String<10> format_mode(const dir_entry_t &e) {
    String<10> s = "";

    ftype_t t = e.type;
    perm_t  p = e.perm;

    // File type letter.
    s += t == t_regular ? '-'
       : t == t_dir     ? 'd'
       : t == t_dev     ? 'b' : '?';

    // File permissions (user, group, other; read, write, execute).
    s += p & perm_ur ? 'r':'-'; s += p & perm_uw ? 'w':'-'; s += p & perm_ux ? 'x':'-';
    s += p & perm_gr ? 'r':'-'; s += p & perm_gw ? 'w':'-'; s += p & perm_gx ? 'x':'-';
    s += p & perm_or ? 'r':'-'; s += p & perm_ow ? 'w':'-'; s += p & perm_ox ? 'x':'-';

    return s;
}

/// Prints a listing for a directory.
static void list_dir(StringView path) {

    fd_t dir = open(path, "d");

    if (dir < 0) {
        print(stderr, "could not open <{}>: {}\n"
             ,path, error_name(dir));
        return;
    }

    print("  {-6} {-10} {-6} {}\n", "INODE", "MODE", "SIZE", "NAME");

    dir_entry_t entry;

    while (true) {
        ssize_t ret = read_dir(dir, entry);
        if (ret < 0) {
            print(stderr, "could not read dir <{}>: {}\n", path, error_name(ret));
            break;
        }
        if (ret == 0)
            // End of dir.
            break;

        if (entry.type == t_dir) {
            // Print directories with a trailing slash.
            print("  {6} {} {6 } {}/\n"
                  ,entry.inode_i
                  ,format_mode(entry)
                  ,""
                  ,entry.name);
        } else {
            print("  {6} {} {4S} {}\n"
                  ,entry.inode_i
                  ,format_mode(entry)
                  ,entry.size
                  ,entry.name);
        }
    }

    close(dir);
}

int main(int argc, const char **argv) {

    if (argc < 2) {
        list_dir(".");
    } else {
        for (int i : range(1, argc)) {
            if (argc > 2) print("{}:\n", argv[i]);
            list_dir(argv[i]);
            if (argc > 2 && i < argc-1) print("\n");
        }
    }

    return 0;
}
