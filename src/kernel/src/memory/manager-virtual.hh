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

/**
 * \namespace Memory::Virtual
 *
 * Virtual memory management (paging).
 *
 * We want to:
 *
 * - Allow processes to run on predictable addresses
 * - Prevent processes from interfering in eachother's memory
 * - Give processes limited access to the kernel
 * - Prevent memory fragmentation
 *
 * Paging to the rescue!
 *
 * We divide physical memory into 4KiB pages, and hand these out to the kernel
 * and user-mode processes when needed.
 * Every process gets its own address space, where every 4KiB chunk of virtual
 * memory may or may not be backed by actual physical memory.
 *
 * In order to allow processes to cheaply make requests to the kernel, we make
 * sure that all (physical) kernel memory is present in every address space.
 *
 * For example, when 3 processes are running, there will be three separate
 * address spaces like this, where every process sees only itself and the kernel:
 *
 *       Address space A          Address space B          Address space C
 *     ┌─────────────────┐ 0M   ┌─────────────────┐ 0M   ┌─────────────────┐ 0M
 *     ├─────────────────┤ 1M   ├─────────────────┤ 1M   ├─────────────────┤ 1M
 *     │ kernel code     │      │ kernel code     │      │ kernel code     │
 *     │ kernel data     │      │ kernel data     │      │ kernel data     │
 *     │ kernel stack    │      │ kernel stack    │      │ kernel stack    │
 *     │ kernel heap     │      │ kernel heap     │      │ kernel heap     │
 *     ├─────────────────┤ 3G   ├─────────────────┤ 3G   ├─────────────────┤ 3G
 *     │ proc A code     │      │ proc B code     │      │ proc C code     │
 *     │ proc A data     │      │ proc B data     │      │ proc C data     │
 *     │ proc A heap     │      │ proc B heap     │      │ proc C heap     │
 *     │ proc A stack    │      │ proc B stack    │      │ proc C stack    │
 *     ├─────────────────┤ ?    ├─────────────────┤ ?    ├─────────────────┤ ?
 *     └─────────────────┘ 4G   └─────────────────┘ 4G   └─────────────────┘ 4G
 *
 * (things get more interesting when we let processes share memory with eachother)
 *
 * \todo document paging structures.
 */
namespace Memory::Virtual {

    /**
     * \name Paging structures
     *
     *@{
     */

    using pde_t = u32; /// A page directory entry is a 32-bit bitfield.
    using pte_t = u32; /// A page table entry is a 32-bit bitfield.

    /**
     * \name Flags that indicate access rights on a page of memory.
     *
     * These are valid for both pages and entire page tables.
     *
     *@{
     */
    constexpr u32 flag_present  = 1 << 0;
    constexpr u32 flag_writable = 1 << 1;
    constexpr u32 flag_user     = 1 << 2; ///< accessible from user-mode?
    constexpr u32 flag_nocache  = 1 << 4; ///< should be 1 for memory-mapped I/O.
    constexpr u32 flag_borrowed = 1 << 9; ///< 0 if this virt page "owns" the phy page.
    ///@}

    // Note: alignas is not valid on type aliases (why?).
    //       the gcc-/clang specific aligned attribute works, but generates a
    //       warning when this type is used in a container.
    //       so we resort to requiring instantiations to be marked alignas() instead.

    using PageDir = Array<pde_t,1024>;
    using PageTab = Array<pte_t,1024>;
    ///@}

    /// Get the current page directory.
    PageDir &current_dir();

    /// note: virt and size must be page-aligned.
    addr_t map(addr_t virt, addr_t phy, size_t size, u32 flags);

    /// note: virt and size must be page-aligned.
    void unmap(addr_t virt, size_t size);

    /// Maps memory-mapped IO memory.
    /// If virt is 0, will allocate virtual address space.
    addr_t map_mmio(addr_t virt, addr_t phy, size_t size, u32 flags);

    bool is_mapped(addr_t virt);

    /// Lookup the physical address belonging to the given virtual address in
    /// the current address space. (returns 0 if not present)
    addr_t virtual_to_physical(addr_t virt);

    /// Lookup the physical address belonging to the given virtual address in
    /// the current address space. (returns 0 if not present)
    addr_t virtual_to_physical(void *virt);

    /// Switch to a different address space.
    void switch_address_space(PageDir &page_dir);

    /// Initialises the memory manager.
    void init();
}
