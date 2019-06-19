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
#include "filesystem.hh"
#include "vfs.hh"

namespace FileSystem {
    errno_t Fs::lookup(inode_t &inode, StringView name, inode_t &dest) {

        // Provide a default lookup() implementation in terms of Fs::read_dir.

        dir_entry_t entry;

        u64 i = 0;
        while (true) {
            errno_t err = read_dir(inode, i, entry);
            if (err > 0) {
                if (entry.name == name) {
                    dest = entry.inode;
                    return ERR_success;
                }
            } else {
                if (err == 0)
                     return ERR_not_exists;
                else return err;
            }
        }
    }
    ssize_t Fs::read  (inode_t&, u64, void*, size_t)               { return ERR_not_supported; }
    ssize_t Fs::write (inode_t&, u64, const void*, size_t)         { return ERR_not_supported; }
    ssize_t Fs::truncate(inode_t &, u64)                           { return ERR_not_supported; }

    errno_t Fs::unlink(inode_t&, StringView)                       { return ERR_not_supported; }
    errno_t Fs::create(inode_t&, const file_name_t&, inode_t&)     { return ERR_not_supported; }

    errno_t Fs::seek(inode_t &inode, u64 &pos, seek_t dir, s64 off) {
        if (dir == seek_set) {
            if (off < 0 || u64(off) >= inode.size)
                return ERR_invalid;
            pos = off;
        } else if (dir == seek_end) {
            if (off < 0 || u64(off) >= inode.size)
                return ERR_invalid;
            pos = inode.size - 1 - off;
        } else if (dir == seek_cur) {
            if ((off < 0 && u64(-off) > pos) || pos + off >= inode.size)
                return ERR_invalid;
            pos = inode.size - 1 - off;
        } else {
            return ERR_invalid;
        }
        return ERR_success;
    }

    errno_t Fs::mkdir (inode_t&, StringView)                       { return ERR_not_supported; }
    errno_t Fs::rmdir (inode_t&, StringView)                       { return ERR_not_supported; }
    errno_t Fs::rename(inode_t&, StringView, inode_t&, StringView) { return ERR_not_supported; }

    void init() {
        Vfs::init();
    }
}
