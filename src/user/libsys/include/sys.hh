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

#include <syscall-numbers.hh>
#include <os-std/types.hh>
#include <os-std/string.hh>
#include <os-std/limits.hh>
#include <os-std/errno.hh>

inline int syscall(u32 a = 0,
                   u32 b = 0,
                   u32 c = 0,
                   u32 d = 0,
                   u32 e = 0,
                   u32 f = 0) {

    asm volatile ("int $0xca"
                 :"+a" (a)
                 :"b"  (b)
                 ,"c"  (c)
                 ,"d"  (d)
                 ,"S"  (e)
                 ,"D"  (f)
                 :"cc",
                  "memory");

    return a;
}

inline int sys_yield()   {
    return syscall(SYS_YIELD);
}

inline int sys_get_tid() {
    return syscall(SYS_GET_TID);
}

inline int sys_get_pid() {
    return syscall(SYS_GET_PID);
}

inline int sys_thread_delete(tid_t tid) {
    return syscall(SYS_THREAD_DELETE, tid);
}

inline int sys_open(fd_t fd, ostd::StringView path, int mode) {
    return syscall(SYS_OPEN, fd, (addr_t)path.data(), path.length(), mode);
}

inline int sys_close(fd_t fd) {
    return syscall(SYS_CLOSE, fd);
}

inline int sys_read(fd_t fd, void *buffer, size_t nbytes) {
    return syscall(SYS_READ, fd, (addr_t)buffer, nbytes);
}

inline int sys_write(fd_t fd, const void *buffer, size_t nbytes) {
    return syscall(SYS_WRITE, fd, (addr_t)buffer, nbytes);
}

inline int sys_read_dir(fd_t fd, syscall_dir_entry_t &buffer) {
    return syscall(SYS_READ_DIR, fd, (addr_t)&buffer, sizeof(buffer));
}

inline int sys_seek(fd_t fd, int whence, ssize_t off) {
    return syscall(SYS_SEEK, fd, whence, off);
}

inline int sys_spawn(ostd::StringView path
                    ,int argc
                    ,const ostd::StringView *args
                    ,bool do_wait
                    ,ostd::Array<fd_t, 3> transplanted_files) {

    if (argc < 0 || (size_t)argc > ostd::max_args) return ostd::ERR_invalid;

    syscall_string_t argv[ostd::max_args];
    for (int i : ostd::range(argc))
        argv[i] = { args[i].data(), args[i].length() };

    syscall_spawn_args_t spec;

    spec.args.count = argc;
    spec.args.data  = argv;
    spec.do_wait    = do_wait;

    for (int i : ostd::range(3))
        spec.fd_transplant[i] = transplanted_files[i];

    return syscall(SYS_SPAWN
                  ,(u32)path.data()
                  ,path.length()
                  ,(u32)&spec
                  ,sizeof(spec));
}

inline int sys_wait_pid(pid_t pid) {
    return syscall(SYS_WAIT_PID, pid);
}

inline int sys_get_cwd(char *path_buffer, size_t size) {
    return syscall(SYS_GET_CWD, (addr_t)path_buffer, size);
}

inline int sys_set_cwd(ostd::StringView path) {
    return syscall(SYS_SET_CWD, (addr_t)path.data(), path.length());
}

inline int sys_duplicate_fd(fd_t dest, fd_t fd) {
    return syscall(SYS_DUPLICATE_FD, dest, fd);
}

inline int sys_pipe(fd_t &in, fd_t &out) {
    fd_t fds[2];
    errno_t err = syscall(SYS_PIPE, (addr_t)fds);

    if (err < 0) return err;

    in  = fds[0];
    out = fds[1];

    return err;
}
