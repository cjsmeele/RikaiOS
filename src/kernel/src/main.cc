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
// #include "memory/kernel-heap.hh"
#include "interrupt/interrupt.hh"
#include "driver/driver.hh"
#include "process/proc.hh"
#include "ipc/semaphore.hh"

/**
 * \file
 * Contains the kernel's main function.
 */

// Testing threads and mutexes {{{

mutex_t mutex;

static void test_a() {
    size_t size = 50;

    for (u64 i = 0;; ++i) {
        Io::wait(100); // some microseconds to get interrupted (so we don't stall the machine).
        if (i % size == 0) {
            mutex.lock();
        } else if (i % size == size - 1) {
            kprint("(A done)\n");
            mutex.unlock();
            Process::yield(); // give B some time to capture the mutex.
        } else {
            kprint("A");
            Process::yield(); // control should be returned to us immediately.
        }
    }
}

static void test_b() {
    size_t size = 20;

    for (u64 i = 0;; ++i) {
        Io::wait(100); // some microseconds to get interrupted (so we don't stall the machine).
        if (i % size == 0) {
            mutex.lock();
        } else if (i % size == size - 1) {
            kprint("(B done)\n");
            mutex.unlock();
            Process::yield(); // give A some time to capture the mutex.
        } else {
            kprint("B");
            // Process::yield();
        }
    }
}

// }}}

/// This is the C++ kernel entrypoint (run right after start.asm).
extern "C" void kmain(const boot_info_t &boot_info);
extern "C" void kmain(const boot_info_t &boot_info) {

    // Make sure we can write to the console.
    kprint_init();

    kprint("\neos-os is booting.\n\n");

    // Initialise subsystems.
    Interrupt::init();          // Configure the interrupt controller.
    Memory   ::init(boot_info); // Set up segments, enable paging.
    Process  ::init();          // Initialise the scheduler.
    Driver   ::init();          // Detect and initialise hardware.

    kprint("\n(nothing to do - press ESC to crash and burn)\n\n");

    Process::make_kernel_thread(test_a, "test-a");
    Process::make_kernel_thread(test_b, "test-b");

    // (this is where we would load an 'init' process from disk and run it)

    Process::run();

    panic("reached end of kmain");
}
