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
#include "common.hh"
#include "boot/bootinfo.hh"
#include "memory/memory.hh"
#include "interrupt/interrupt.hh"
#include "driver/driver.hh"
#include "process/proc.hh"
#include "ipc/semaphore.hh"
#include "filesystem/filesystem.hh"
#include "kshell.hh"
#include "filesystem/vfs.hh"
#include "process/elf.hh"

/**
 * \file
 * Contains the kernel's main function (the C++ entrypoint).
 */

/**
 * A thread that tries to load an "init" program.
 *
 * If no init program is found on the first mounted disk, the built-in kernel
 * shell is started instead.
 *
 * This thread immediately exits afterwards.
 */
static void run_init() {

    // Find the "init" executable on the first mounted disk.

    path_t init_path = "/";

    // First, find the disk mount.
    int fd = Vfs::open("/", o_dir);
    assert(fd > 0, "could not open vfs root");
    while (true) {
        dir_entry_t entry;
        int n = Vfs::read_dir(fd, entry);
        if (n > 0) {
            // Take any directory under / starting with "disk" as a mounted disk.

            if (entry.name.starts_with("disk")) {
                init_path += entry.name;
                break;
            }
        } else if (n == 0) {
            // End of directory.
            break;
        } else {
            kprint("couldn't read root directory: {}\n", error_name(n));
        }
    }
    Vfs::close(fd);

    // init_path should now be /diskXpY
    if (init_path.starts_with("/disk")) {

        path_t rootfs = init_path;

        init_path += "/init.elf";

        Process::proc_t *proc;
        Process::proc_arg_spec_t args { "init" };

        // Create a process. It is automatically scheduled to run.
        errno_t err = Elf::load_elf(init_path, args, proc);

        if (err < 0) {
            kprint("could not find/load <{}>, good luck!\n", init_path);
            kprint("(error was: {})\n", error_name(err));

            Kshell::enable();
            return;
        }

        proc->working_directory = rootfs;

        // All done.
        return;
    }

    kprint("could not find init.elf, good luck!\n");
    Kshell::enable();
}

/// This is the C++ kernel entrypoint (run right after start.asm).
extern "C" void kmain(const boot_info_t &boot_info);
extern "C" void kmain(const boot_info_t &boot_info) {

    // Make sure we can write to the console.
    kprint_init();

    kprint("\n welcome to EOS-OS, version {}.\n\n", KERNEL_VERSION);

    // Initialise subsystems.
    Interrupt ::init();          // Configure the interrupt controller.
    Memory    ::init(boot_info); // Set up segments, enable paging.
    Process   ::init();          // Initialise the scheduler.
    FileSystem::init();          // Initialise the virtual filesystem.
    Driver    ::init();          // Detect and initialise hardware.

    // Create kernel threads.

    Process::make_kernel_thread(run_init,       "run_init");
    Process::make_kernel_thread(Kshell::kshell, "kshell");

    // Dispatch the first thread, enable the scheduler.
    // (this function does not return)
    Process::run();

    // This UNREACHABLE text (see common.hh) is just for clarity.
    // As a bonus, if it somehow *were* to be reached, it would trigger a panic.
    UNREACHABLE
}
