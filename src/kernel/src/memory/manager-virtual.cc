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
#include "manager-virtual.hh"
#include "manager-physical.hh"
#include "layout.hh"

namespace Memory::Virtual {

    static bool paging_enabled = false;

    /// Enable paging by writing to CR0.
    /// The page directory must have been loaded with set_pagedir() prior to this.
    static void enable_paging() {
        asm_cr0(asm_cr0()
               | 1 << 16   // Enable paging write-protect feature.
               | 1 << 31); // Enable paging.

        paging_enabled = true;

        // Note: We don't ever disable paging after enabling it.
    }

    /// Initial kernel page directory.
    alignas(4_KiB) PageDir kernel_dir;

    /// We pre-allocate page tables to map all of kernel memory (the first 1G of virtual memory).
    /// This costs 1 MiB memory, but makes kernel memory allocation easier.
    alignas(4_KiB) Array<PageTab,256> kernel_tabs;

    /**
     * Initial kernel recursive page table, used for bootstrapping.
     *
     * The recursive page table is loaded into the page directory such that it
     * describes the memory from 0x3fc0000 to 0x3fffffff (at the top of kernel memory).
     *
     * Each of the 1024 PTEs in the recursive table points to the corresponding
     * page table in the current page directory, so that all memory mappings
     * for the current process can be managed through this 4 MiB window.
     *
     * Since user-mode processes need their own page tables for user memory (> 1 GiB),
     * all processes need their own recursive table. However, kernel memory is
     * mapped the same way for all processes, so all recursive tables' bottom
     * 1/4th is the same... except for the 255th entry, which must point to the
     * recursive page table itself! (aargh)
     */
    PageTab &kernel_recursive_tab = kernel_tabs[255];

    /**
     * A pointer to the current recursive page table.
     *
     * After paging has been enabled, the recursive page table for the current
     * process is always accessible through itself at the top of kernel memory.
     *
     * \see kernel_recursive_tab
     */
    PageTab * const recursive_tab_ = &((PageTab*)(1_GiB - 4_MiB))[255];

    /// Always points to the currently active page directory.
    PageDir *current_dir_ = nullptr;

    /// Get the current page directory.
    static PageDir &current_dir() {
        if (paging_enabled)
             return *current_dir_;
        else return  kernel_dir;
    }

    /**
     * Always returns the current recursive page table.
     *
     * If paging has not yet been enabled, returns the kernel (bootstrap)
     * recursive page table.
     */
    static PageTab &recursive_tab() {
        if (paging_enabled)
             return *recursive_tab_;
        else return kernel_recursive_tab;
    }

    /// Returns an array containing all pagetables.
    static Array<PageTab,1024> &tables() {
        return *(Array<PageTab,1024>*)Layout::page_tables().start;
    }

    /// Clears a page from the cache (the TLB).
    static void invalidate(addr_t addr) {
        asm_invlpg(addr);
    }

    /// Throws out the entire cache.
    static void flush_tlb() {
        asm_cr3(asm_cr3());
    }

    /// Creates a 32-bit page directory entry.
    static pde_t make_pde(addr_t phy_addr, u32 flags) { return (phy_addr & 0xfffff000) | flags; }

    /// Creates a 32-bit page table entry.
    static pte_t make_pte(addr_t phy_addr, u32 flags) { return (phy_addr & 0xfffff000) | flags; }

    /// Functions for address conversions.
    ///@{
    [[maybe_unused]] static u32    addr_tab   (addr_t x) { return  x >> 22;          }
    [[maybe_unused]] static u32    addr_page  (addr_t x) { return  x >> 12;          }
    [[maybe_unused]] static u32    addr_pagei (addr_t x) { return (x >> 12) & 0x3ff; }
    [[maybe_unused]] static u32    addr_offset(addr_t x) { return  x & 0xfff;        }
    [[maybe_unused]] static u32    page_addr  (u32    x) { return  x << 12;          }
    [[maybe_unused]] static addr_t pte_addr   (pte_t  x) { return  x & ~(0xfff);     }
    ///@}

    /**
     * Get the page table containing an entry for the given virtual address.
     *
     * if allocate is true, will try to allocate a table if it does not yet exist.
     *
     * When a table does not exist or could not be allocated, nullptr is returned.
     */
    static PageTab *get_tab(addr_t for_addr, bool allocate = false) {

        // kprint("    tab {08x}\n", for_addr);

        PageDir &dir = current_dir();
        u32 tab_no   = addr_tab(for_addr);
        if (!(dir[tab_no] & flag_present)) {
            if (!allocate) return nullptr;

            u32 phy_page = Physical::allocate_one();
            if (!phy_page) return nullptr;

            // kprint("    tab {} install {08x} into {}\n", tab_no, phy_page*page_size, &recursive_tab()[tab_no]);

            recursive_tab()[tab_no] = make_pte(page_addr(phy_page), flag_present | flag_writable);

            if (addr_in_region(for_addr, Layout::user()))
                 dir[tab_no] = make_pde(page_addr(phy_page), flag_present
                                                           | flag_writable
                                                           | flag_user);
            else dir[tab_no] = make_pde(page_addr(phy_page), flag_present
                                                           | flag_writable);

            // kprint("    tab installed\n");

            flush_tlb();
        }

        return &tables()[addr_tab(for_addr)];
    }

    addr_t virtual_to_physical(addr_t virt) {
        // If paging is not yet enabled, then virtual addresses map one-to-one
        // to equal physical addresses.
        if (!paging_enabled) return virt;

        PageTab *table_ = get_tab(virt);
        if (!table_) return 0;

        PageTab table = *table_;
        pte_t entry = table[addr_pagei(virt)];
        return pte_addr(entry);
    }
    addr_t virtual_to_physical(void *virt) {
        return virtual_to_physical((addr_t)virt);
    }

    /// Changes the current address space.
    static void set_pagedir(PageDir &dir) {
        // Load a pointer to the page directory in CR3.
        addr_t phy_addr = virtual_to_physical(&dir);
        assert(phy_addr, "tried to switch to unmapped page directory");
        asm_cr3(phy_addr);
        current_dir_ = &dir;
    }

    void switch_address_space(PageDir &page_dir) {
        kprint("switch to pd @{}\n", &page_dir);
        set_pagedir(page_dir);
    }

    /// Removes a memory mapping.
    static void unmap_one(addr_t virt) {

        kprint("  unmap {08x}\n", virt);

        PageTab *tab_ = get_tab(virt);

        if (tab_) {
            PageTab &tab = *tab_;
            pte_t &pte = tab[addr_pagei(virt)];
            if (pte & flag_present) {
                // If this virt page owns the corresponding phy page,
                // then free the phy page.
                if (!(pte & flag_borrowed))
                    Physical::free_one(addr_page(pte_addr(pte)));
                pte = 0;
                invalidate(virt);
            }
        }
    }

    /// Removes memory mappings.
    void unmap(addr_t virt, size_t size) {
        kprint("* UNMAP {08x}        {6S}\n", virt, size);

        for (size_t i = 0; i < size/page_size; ++i)
            unmap_one(virt + i*page_size);
    }

    /// Creates a memory mapping.
    ///
    /// if phy is 0, tries to allocate a physical page.
    static bool map_one(addr_t virt, addr_t phy, u32 flags) {
        // kprint("  map {08x} -> {08x}\n", virt, phy);

        PageTab *tab_ = get_tab(virt, true);

        if (!tab_) return false;
        // kprint("  map {08x} -> {08x} have tab: {}\n", virt, phy, tab_);

        // Do we need to allocate?
        if (phy == 0) {
            u32 phy_page = Physical::allocate_one();
            if (!phy_page) return false;

            phy = phy_page * page_size;
        }

        PageTab &tab = *tab_;
        tab[addr_pagei(virt)] = make_pte(phy, flags | flag_present);
        invalidate(virt);

        return true;
    }

    /// Creates memory mappings.
    ///
    /// if phy is 0, tries to allocate physical pages.
    addr_t map(addr_t virt, addr_t phy, size_t size, u32 flags) {

        // kprint("* MAP {08x} -> {08x} {6S}\n", virt, phy, size);

        for (size_t i = 0; i < size/page_size; ++i) {
            if (!map_one(virt + i*page_size
                        ,phy ? phy  + i*page_size : 0
                        ,flags)) {

                // map failed - remove mappings up to this point.
                unmap(virt, i*page_size);

                return 0;
            }
        }
        return virt;
    }

    addr_t map_mmio(addr_t virt, addr_t phy, size_t size, u32 flags) {

        assert(phy, "map_mmio makes no sense without a physical address");

        flags |= flag_borrowed | flag_nocache;

        if (virt) {
            return map(virt, phy, size, flags);
        } else {
            static size_t mmio_mapped = 0; //Layout::kernel_mmio().start;
            if (size <= Layout::kernel_mmio().size - mmio_mapped) {
                virt = Layout::kernel_mmio().start + mmio_mapped;

                if (map(virt, phy, size, flags))
                    mmio_mapped += size;
                return virt;
            } else {
                kprint("warning: no mmio space left for map_mmio()\n");
                return 0;
            }
        }
    }

    bool is_mapped(addr_t virt) {
        PageTab *tab = get_tab(virt);

        return tab && ((*tab)[addr_pagei(virt)] & flag_present);
    }

    void init() {

        // kprint("ktabs at {}\n", kernel_tabs.data());

        // Iterate over the 256 page tables that will describe the first 1 GiB
        // of memory (kernel memory).
        for (auto [i, ktab] : enumerate(kernel_tabs)) {

            // Insert all kernel memory page tables into the page directory.
            kernel_dir[i] = make_pde((u32)&ktab, flag_present | flag_writable);

            // Insert kernel page tables into the recursive page table.
            kernel_recursive_tab[i] = make_pte((u32)&ktab, flag_present | flag_writable);

            // Create identity mappings for kernel code/data/bss.
            // (i.e. we make virt addr 0x00100000 point to phy addr 0x00100000)
            for (auto [j, page] : enumerate(ktab)) {

                addr_t addr = i*4_MiB + j*4_KiB;

                if (addr < Layout::kernel_image().start
                         + Layout::kernel_image().size) {

                    page = make_pte(addr, flag_present | flag_writable);

                } else {
                    break;
                }
            }
        }

        kprint("virtual memory layout:\n");
        kprint("  kernel:   {S}\n", Layout::kernel());
        kprint("    image:  {S}\n", Layout::kernel_image());
        kprint("    heap:   {S}\n", Layout::kernel_heap());
        kprint("    mmio:   {S}\n", Layout::kernel_mmio());
        kprint("    ptabs:  {S}\n", Layout::page_tables());
        kprint("  user:     {S}\n", Layout::user());

        set_pagedir(kernel_dir);

        enable_paging();
    }
}
