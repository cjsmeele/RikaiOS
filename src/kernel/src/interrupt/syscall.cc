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
#include "syscall.hh"
#include "process/proc.hh"

#include "filesystem/vfs.hh"
#include "memory/manager-virtual.hh"
#include "memory/layout.hh"
#include "ipc/semaphore.hh"
#include "process/elf.hh"

#include <syscall-numbers.hh>

namespace Interrupt::Syscall {

    /**
     * Verify that a user-provided buffer is valid.
     *
     * A buffer syscall argument must lie completely within user memory,
     * and must be mapped (resident) in its entirety.
     */
    static bool is_buffer_valid(Memory::region_t region) {
        return region_valid(region) // Does addr+size not overflow?
            && region_contains(Memory::Layout::user(), region)
            && Memory::Virtual::is_mapped(region);
    }

    /**
     * Verify that a user-provided buffer is valid.
     *
     * The same as is_buffer_valid(), but also checks a size limit.
     */
    static bool is_buffer_valid(Memory::region_t region, size_t max_size) {
        return region.size <= max_size
            && is_buffer_valid(region);
    }

    void handle_syscall(Interrupt::interrupt_frame_t &frame) {

        // TODO: Instead of one long if/else train, we should create a vector
        //       table and a separate handler function for each syscall number.
        //       (this also makes it easier to add documentation)

        assert(!Process::current_thread()->is_kernel_thread
              ,"system call performed by kernel thread");

        using namespace Interrupt;

        // Syscall args are provided in these registers as 32-bit numbers.
        // EAX (args[0]) is the syscall number.
        Array args = { frame.regs.eax
                     , frame.regs.ebx
                     , frame.regs.ecx
                     , frame.regs.edx
                     , frame.regs.esi
                     , frame.regs.edi };

        // The result is stored in EAX (overwrites the syscall number).
        u32 &ret = frame.regs.eax;

        // The meaning of each argument depends on which system call number was
        // requested. Although we are limited to receiving only 32-bit numbers as
        // arguments, we can receive strings and buffers as pairs of a memory
        // address and a size.
        //
        // However, we must take into account that the process may have more
        // than one thread running, and the other thread might free/unmap the
        // referenced memory when we context switch in syscalls that lead to
        // I/O, such as read&write.
        //
        // To handle memory safely, we must validate & copy provided buffers
        // into kernel memory before the next context switch takes place.
        // Buffers are copied onto the current stack (the per-thread kernel
        // stack in the Process::thread_t struct).

        // Check the syscall number and execute the requested function.

        // kprint("SYS{} ", args[0]);

        if (args[0] == SYS_YIELD) {

            // () => ()

            // Hand CPU control over to the next runnable thread.
            Process::yield();

        } else if (args[0] == SYS_THREAD_DELETE) {

            // (pid) => ()

            // Exit the running thread.
            // This call triggers a context switch to the next runnable thread.
            delete_thread(Process::current_thread());

            UNREACHABLE

        } else if (args[0] == SYS_OPEN) {

            // (fd, path*, path_len, flags) => fd

            Memory::region_t path_ { args[2], args[3] };
            if (!is_buffer_valid(path_, max_path_length)) {
                kprint("open path buffer invalid: {}\n", path_);
                ret = ERR_invalid; return;
            }

            // Copy path string onto kernel stack.
            path_t path = StringView((const char*)path_.start, path_.size);

            ret = Vfs::open(args[1], path, args[4]);

            // kprint("open result: {}/{} (path = {} @{08x})\n"
            //       ,ret
            //       ,error_name(ret)
            //       ,StringView((char*)args[2], args[3])
            //       ,args[1]);

        } else if (args[0] == SYS_CLOSE) {

            // (fd) => err

            ret = Vfs::close(args[1]);

        } else if (args[0] == SYS_READ) {

            // (fd, buf*, buf_len) => bytes_read

            // TODO: Split into X-byte chunks, and check&copy each.

            Memory::region_t buffer { args[2], args[3] };

            if (!is_buffer_valid(buffer)) {
                kprint("read buffer invalid: {}\n", buffer);
                ret = ERR_invalid;
                return;
            }

            ret = Vfs::read(args[1], (void*)buffer.start, buffer.size);
            if ((s32)ret < 0)
                kprint("read result (fd {}): {}\n", args[1], error_name(ret));

        } else if (args[0] == SYS_WRITE) {

            // (fd, buf*, buf_len) => bytes_written

            // TODO: Split into X-byte chunks, and check&copy each.

            Memory::region_t buffer { args[2], args[3] };

            if (!is_buffer_valid(buffer)) {
                kprint("write buffer invalid: {}\n", buffer);
                ret = ERR_invalid;
                return;
            }

            ret = Vfs::write(args[1], (void*)buffer.start, buffer.size);
            if ((s32)ret < 0)
                kprint("write result (fd {}): {}\n", args[1], error_name(ret));

        } else if (args[0] == SYS_READ_DIR) {

            // (fd, buf*, buf_len) => bytes_read

            Memory::region_t buffer { args[2], args[3] };

            if (!is_buffer_valid(buffer)
              || buffer.size < sizeof(syscall_dir_entry_t)
              || (buffer.start & (alignof(syscall_dir_entry_t)-1))) {
                kprint("read_dir buffer invalid: {}\n", buffer);
                ret = ERR_invalid;
                return;
            }

            syscall_dir_entry_t &entry_out = *(syscall_dir_entry_t*)buffer.start;
            dir_entry_t entry;

            ret = Vfs::read_dir(args[1], entry);
            if ((s32)ret < 0)
                kprint("read_dir result (fd {}): {}\n", args[1], error_name(ret));

            str_copy(entry_out.name, entry.name.data(), entry.name.size());
            entry_out.inode_i = entry.inode.i;
            entry_out.type    = entry.inode.type;
            entry_out.perm    = entry.inode.perm;
            entry_out.size    = entry.inode.size;

        } else if (args[0] == SYS_SEEK) {

            // (fd, whence, offset) => err

            ret = Vfs::seek(args[1], (seek_t)args[2], args[3]);
            if ((s32)ret < 0)
                kprint("seek result: {}\n", error_name(ret));

        } else if (args[0] == SYS_SPAWN) {

            // (path*, path_len, args_spec*, args_spec_len) => pid

            Memory::region_t path_ { args[1], args[2] };
            if (!is_buffer_valid(path_, max_path_length)) {
                kprint("spawn path buffer invalid: {}\n", path_);
                ret = ERR_invalid; return;
            }
            path_t path = StringView((const char*)path_.start, path_.size);

            // Path is valid.

            Memory::region_t arg_spec_ { args[3], args[4] };
            if (arg_spec_.size < sizeof(syscall_spawn_args_t)
             || !is_buffer_valid(arg_spec_)) {
                ret = ERR_invalid; return;
            }
            syscall_spawn_args_t spec = *(syscall_spawn_args_t*)args[3];

            // Arg spec struct is valid.

            if (spec.args.count > max_args
             || !is_buffer_valid(Memory::region_t { (addr_t)spec.args.data
                                                  , spec.args.count
                                                  * sizeof(*spec.args.data) })) {
                ret = ERR_invalid; return;
            }

            // Argument list struct is valid.

            // Our strategy for copying the process' arguments:
            // Create a single cmdline string, append each argument
            // to it as a null-terminated string, and create a separate array
            // of StringViews which point into the cmdline string.
            //
            // The capacity of the cmdline string includes space for one NUL
            // byte per argument.

            Array<char, max_args_chars+max_args> cmdline;
            Array<StringView, max_args> arg_strs;
            int argc = spec.args.count;

            size_t cmdline_i = 0;
            for (size_t argi : range(argc)) {

                const syscall_string_t &elem = spec.args.data[argi];

                if (cmdline_i + elem.count >= cmdline.size()) {
                    // cmdline too long.
                    ret = ERR_invalid; return;
                }

                arg_strs[argi] = { &cmdline[cmdline_i], elem.count };

                // Copy the argument
                for (size_t i : range(elem.count))
                    cmdline[cmdline_i++] = elem.data[i];

                cmdline[cmdline_i++] = 0;
            }

            Array<fd_t, 3> fd_transplant;
            fd_transplant = spec.fd_transplant;

            // All arguments are now safe.
            // Next, we acquire locks for all handles that need to be transplanted.
            // (this may cause the thread to block until other read/write
            // actions on those handles are completed)

            Process::proc_t *current_proc = Process::current_proc();

            // First make sure the fds exist.

            for (int i : range(3)) {
                fd_t fd = fd_transplant[i];
                if (fd < 0) continue;

                // Make sure the same fd is not transplanted multiple times.
                for (int j : range(i-1, -1, -1)) {
                    if (fd == fd_transplant[j]) {
                        ret = ERR_invalid;
                        return;
                    }
                }

                if ((size_t)fd >= current_proc->files.size()
                 || !current_proc->files[fd]) {

                    // source fd invalid, unlock and return an error.
                    for (int j : range(i-1, -1, -1)) {
                        fd_t fd_ = fd_transplant[j];
                        if (fd_ < 0) continue;
                        mutex_unlock(current_proc->files[fd_]->lock);
                    }
                    ret = ERR_bad_fd;
                    return;
                }
                mutex_lock(current_proc->files[fd]->lock);
            }

            // All to be transplanted handles are locked.
            // Now load the ELF file (this may yield multiple times for disk IO).

            Process::proc_t *proc;
            errno_t err = Elf::load_elf(path, arg_strs, proc);

            // The remaining part of this system call will not yield.
            // Unlock the handles.
            for (fd_t fd : fd_transplant) {
                if (fd < 0) continue;
                mutex_unlock(current_proc->files[fd]->lock);
            }

            if (err < 0) {
                ret = err;
                return;
            }

            // Now actually transplant the handles.
            for (auto [i, fd] : enumerate(fd_transplant)) {
                if (fd >= 0) {
                    err = Vfs::transplant_fd(proc, i, fd);
                    if (err < 0)
                        kprint("kernel: warning: fd transplant failed\n");
                }
            }

            ret = err;
            if (spec.do_wait)
                wait(proc->exit_sem);

        } else if (args[0] == SYS_WAIT_PID) {

            // (pid) => ()

            Process::proc_t *tgt = Process::process_by_pid(args[1]);
            if (!tgt) {
                ret = ERR_not_exists;
                return;
            }
            wait(tgt->exit_sem);

        } else if (args[0] == SYS_GET_CWD) {

            // (buffer, length) => length

            Memory::region_t path_ { args[1], args[2] };
            if (!is_buffer_valid(path_)) {
                ret = ERR_invalid; return;
            }
            char *buffer     = (char*)path_.start;
            const path_t &wd = Process::current_proc()->working_directory;

            size_t len = min(path_.size, wd.length());

            memcpy(buffer, wd.data(), len);

            ret = len;

        } else if (args[0] == SYS_SET_CWD) {

            // (buffer, length) => err

            Memory::region_t path_ { args[1], args[2] };
            if (!is_buffer_valid(path_, max_path_length)) {
                ret = ERR_invalid; return;
            }
            path_t path = StringView((const char*)path_.start
                                    ,path_.size);

            // Make sure the directory exists.
            errno_t err = Vfs::open(path, o_dir);
            if (err < 0) {
                ret = err;
                return;
            }
            Vfs::close(err);

            Process::current_proc()->working_directory
                = Vfs::canonicalise_path(path);

            ret = ERR_success;

        } else if (args[0] == SYS_DUPLICATE_FD) {

            ret = Vfs::duplicate_fd(args[1], args[2]);

        } else if (args[0] == SYS_PIPE) {

            if (!is_buffer_valid(Memory::region_t { args[1], sizeof(fd_t)*2 })) {
                ret = ERR_invalid; return;
            }

            // NB: make_pipe must not yield.
            ret = Vfs::make_pipe(((fd_t*)args[1])[0]
                                ,((fd_t*)args[1])[1]);

        } else {
            kprint("syscalled! (eax = {})\n", args[0]);
            ret = ERR_invalid;
        }
    }
}
