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

#include "os-std/types.hh"
#include "os-std/limits.hh"
#include "os-std/fs-types.hh"

/**
 * Syscall numbers.
 *
 * These system call numbers are passed in register EAX in system calls to
 * indicate what a process wants to ask the kernel to do.
 *
 * Other registers (EBX, ECX, EDX, ESI and EDI, in that order) are used to pass
 * arguments to the specified system call.
 */
enum syscall_t {
    SYS_YIELD         =  0,
    SYS_GET_TID       =  1,
    SYS_GET_PID       =  2,
    SYS_THREAD_DELETE =  3,
    SYS_OPEN          =  4,
    SYS_CLOSE         =  5,
    SYS_READ          =  6,
    SYS_WRITE         =  7,
    SYS_READ_DIR      =  8,
    SYS_SEEK          =  9,
    SYS_SPAWN         = 10,
    SYS_WAIT_PID      = 11,
    SYS_GET_CWD       = 12,
    SYS_SET_CWD       = 13,
    SYS_DUPLICATE_FD  = 14,
    SYS_PIPE          = 15,
};

/**
 * Minimalistic types for syscall arguments.
 *
 * We'd like to directly pass C++ objects from userspace to the kernel.
 * However, class invariants can not be guaranteed, since we cannot restrict
 * how system calls are called from the user side of things.
 * For example, a broken program might simply trigger a system call without
 * passing arguments by using the INT 0xca instruction.
 *
 * So, both for security and stability reasons, the kernel needs to carefully
 * verify all arguments passed in system calls.
 *
 * To aid in this, we create a minimal set of structures with accompanying
 * functions (in kernel interrupt/syscall.cc) to verify them.
 *
 * Verification steps performed by the kernel include:
 *
 * - Do buffers referenced by syscall arguments point into process memory?
 *   - (holds both for e.g. read/write buffers and for all string arguments)
 * - Are buffer sizes within limits?
 * - Do file descriptors exist?
 *
 * @{
 */

/// Argument structure for arrays passed to syscalls.
/// T is the array element type.
template<typename T>
struct syscall_array_t {
    const T *data;
    size_t   count;
};

/// A string argument is an array of characters.
using syscall_string_t = syscall_array_t<char>;

struct syscall_dir_entry_t {
    char name[ostd::max_file_name_length+1];

    u64     inode_i = 0;
    ftype_t type    = t_regular;
    perm_t  perm    = 0;
    u64     size    = 0;
};

/// Argument structure for spawning processes.
struct syscall_spawn_args_t {

    /// List of argument strings.
    syscall_array_t<syscall_string_t> args;

    /**
     * Transplantation specification for the new process' stdin, stdout and stderr.
     *
     * Values <0 mean: do not transplant.
     * Other values map to the spawner process' file numbers.
     *
     * (the amount of file descriptors that can be transplanted is fixed to 3)
     */
    fd_t fd_transplant[3];

    /// Whether to wait for the spawed process to finish executing.
    bool do_wait = false;
};

/**
 * @}
 */
