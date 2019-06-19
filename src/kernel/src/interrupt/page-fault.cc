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
#include "page-fault.hh"
#include "../memory/manager-virtual.hh"
#include "../memory/layout.hh"
#include "process/proc.hh"

namespace Interrupt {

    using namespace Memory;

    bool handle_pagefault(interrupt_frame_t &frame) {

        // The address causing the fault is in CR2.
        addr_t address = asm_cr2();

        // if (Process::current_thread())
        //     kprint("[{}] paging in {08x}\n", *Process::current_thread(), address);

        // Note: This code must not trigger additional faults.

        // If the access was:
        // - within the kernel heap
        // - performed by the kernel
        // - not an instruction fetch
        // - to a non-present page...

        if (addr_in_region(address, Layout::kernel_heap())
         && (frame.error_code & 0x1d) == 0) {

            addr_t aligned = address & ~(page_size-1);

            // ... then we lazily allocate that heap page.

            if (Virtual::map(aligned, 0, page_size, Virtual::flag_writable) >= 0) {

                mem_set((u32*)aligned, 0xdeaddead, page_size/4);
                // (deaddead is a sentinel value that may indicate accesses to
                //  uninitialized heap memory)
                return true;
            }

            // map failed: let the exception handler crash (do not call panic
            // directly) so that the frame dump will nicely indicate the
            // source of the issue.
            kprint("!! failed to lazily allocate kernel heap space\n");

            // This lazy heap implementation is easy and ok-fast, but may lead to
            // issues when we implement measures to deal with out-of-memory
            // situations: The accessor is utterly unable to handle the issue
            // itself, so this handler will be forced to free memory somewhere
            // in order to continue normal operation of the interrupted kernel
            // code.
        }

        return false;
    }
}
