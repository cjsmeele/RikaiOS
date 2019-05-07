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
#include "layout.hh"

/// Defined by the linker script.
extern int KERNEL_HEAP_START;

namespace Memory::Layout {

    region_t kernel()        { return {         0, 1_GiB         }; }
    region_t   user()        { return {     1_GiB, 3_GiB - 4_KiB }; }

    region_t page_tables()   { return {0x3fc00000, 4_MiB        }; }

    region_t kernel_image()  { return {1_MiB
                                      ,size_t(addr_t(&KERNEL_HEAP_START) - 1_MiB) }; }

    region_t kernel_heap()   { return {addr_t(&KERNEL_HEAP_START)
                                      ,0x30000000 - addr_t(&KERNEL_HEAP_START)}; }

    region_t kernel_mmio()   { return {0x30000000
                                      ,0x3fc00000 - 0x30000000}; }
}
