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

#include "common.hh"
#include "memory.hh"

/**
 * \namespace Memory::Layout
 *
 * Here we decide where things are layed out in virtual memory.
 *
 * For a quick overview:
 *
 *     ┌─────────────────────┐ 0x0000'0000
 *     │ (reserved)          │
 *     ├─────────────────────┤ 0x0010'0000  - @ 1 MiB
 *     │ Kernel code + data  │
 *     ├─────────────────────┤ ?
 *     │ Kernel heap         │
 *     ├─────────────────────┤ 0x3000'0000  - @ 768  MiB
 *     │ Memory mapped I/O   │
 *     ├─────────────────────┤ 0x3fc0'0000  - @ 1020 MiB
 *     │ Page tables (4M)    │
 *     ├─────────────────────┤ 0x4000'0000  - @ 1 GiB
 *     │ User code + data    │
 *     │ User heap           │
 *     │ User stack          │
 *     ├─────────────────────┤ 0xffff'f000
 *     └─────────────────────┘ 0xffff'ffff  - @ 4 GiB
 *
 * * One page (4K) below and above the stack are reserved - this is used to
 *   detect stack under- and overflows in early kernel code.
 * * Before the first user-mode task is started, the kernel stack is
 *   still located within kernel data.
 *
 * See layout.cc for the actual values
 */
namespace Memory::Layout {

    region_t kernel();       ///< Returns the kernel region.
    region_t   user();       ///< Returns the user region.

    region_t kernel_stack(); ///< The per-task kernel stack.
    region_t page_tables();  ///< The per-task page tables.
    region_t kernel_image(); ///< The kernel binary (text+data, including bss).
    region_t kernel_heap();  ///< The global kernel heap.
    region_t kernel_mmio();  ///< Memory mapped I/O.
}
