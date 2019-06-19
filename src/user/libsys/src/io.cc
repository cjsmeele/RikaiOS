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
#include "io.hh"

#include <os-std/memory.hh>

fd_t stdin  = 0;
fd_t stdout = 1;
fd_t stderr = 2;

fd_t open(fd_t fd, ostd::StringView path, ostd::StringView mode) {

    u32 flags = 0;
    for (char c : mode) {
             if (c == 'r') flags |= o_read;     // flags |= 1 << 0;
        else if (c == 'w') flags |= o_write;    // flags |= 1 << 1;
        else if (c == 'a') flags |= o_append;   // flags |= 1 << 2;
        else if (c == 't') flags |= o_truncate; // flags |= 1 << 3;
        else if (c == 'c') flags |= o_create;   // flags |= 1 << 4;
        else if (c == 'd') flags |= o_dir;      // flags |= 1 << 5;
        else return ostd::ERR_invalid;
    }

    return sys_open(fd, path, flags);
}

fd_t open(ostd::StringView path, ostd::StringView mode) {
    return open(-1, path, mode);
}

errno_t close(fd_t fd) {
    return sys_close(fd);
}

fd_t duplicate_fd(fd_t dest, fd_t fd) {
    return sys_duplicate_fd(dest, fd);
}
errno_t pipe(fd_t &in, fd_t &out) {
    return sys_pipe(in, out);
}

ssize_t read (fd_t fd,       void *p, size_t nbytes) { return sys_read (fd, p, nbytes); }
ssize_t write(fd_t fd, const void *p, size_t nbytes) { return sys_write(fd, p, nbytes); }

ssize_t read_dir(fd_t fd, dir_entry_t &d) {
    syscall_dir_entry_t e;
    ssize_t ret = sys_read_dir(fd, e);
    if (ret <= 0) return ret;
    d.name.length_ = ostd::str_length(e.name);
    memcpy(d.name.data(), e.name, d.name.length()+1);

    d.inode_i = e.inode_i;
    d.type    = e.type;
    d.perm    = e.perm;
    d.size    = e.size;

    return ret;
}

errno_t seek (fd_t fd, int whence, ssize_t off) { return sys_seek(fd, whence, off); }
