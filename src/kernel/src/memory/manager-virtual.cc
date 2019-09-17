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
#include "kernel-heap.hh"
#include "layout.hh"
#include "interrupt/interrupt.hh"

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
    PageDir &current_dir() {
        if (paging_enabled)
             return *current_dir_;
        else return  kernel_dir;
    }

    address_space_t *kernel_space() {
        static Memory::Virtual::address_space_t kernel_space;
        kernel_space.pd     = &kernel_dir;
        kernel_space.pt_rec = &kernel_recursive_tab;
        return &kernel_space;
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
    [[maybe_unused]] static addr_t pde_addr   (pde_t  x) { return  x & ~(0xfff);     }
    ///@}

    static PageDir *new_pdir() {
        // We cannot simply do `new PageDir`, since the required alignment will
        // not be respected (it's not an attribute on the PageDir type).
        // Instead, we use a manual aligned allocation together with placement new.
        void *p = Heap::alloc(sizeof(PageDir), 4_K);
        if (!p) return nullptr;
        return new (p) PageDir;
    }

    static PageTab *new_ptab() {
        // See above.
        void *p = Heap::alloc(sizeof(PageTab), 4_K);
        if (!p) return nullptr;
        return new (p) PageTab;
    }

    /**
     * Get the page table containing an entry for the given virtual address.
     *
     * if allocate is true, will try to allocate a table if it does not yet exist.
     *
     * When a table does not exist or could not be allocated, nullptr is returned.
     */
    static PageTab *get_tab(addr_t for_addr, bool allocate = false) {

        // klog("    tab {08x}\n", for_addr);

        PageDir &dir = current_dir();
        u32 tab_no   = addr_tab(for_addr);
        if (!(dir[tab_no] & flag_present)) {
            if (!allocate) return nullptr;

            u32 phy_page = Physical::allocate_one();
            if (!phy_page) return nullptr;

            // klog("    tab {} install {08x} into {}\n", tab_no, phy_page*page_size, &recursive_tab()[tab_no]);

            recursive_tab()[tab_no] = make_pte(page_addr(phy_page), flag_present | flag_writable);

            if (addr_in_region(for_addr, Layout::user()))
                 dir[tab_no] = make_pde(page_addr(phy_page), flag_present
                                                           | flag_writable
                                                           | flag_user);
            else dir[tab_no] = make_pde(page_addr(phy_page), flag_present
                                                           | flag_writable);

            // klog("    tab installed\n");

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

        PageTab &table = *table_;
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

    void switch_address_space(address_space_t &space) {
        switch_address_space(*space.pd);
    }
    void switch_address_space(PageDir &page_dir) {
        // kprint("switch to pd @{}\n", &page_dir);
        set_pagedir(page_dir);
    }

    /// Removes a memory mapping.
    static void unmap_one(addr_t virt) {

        // klog("  unmap {08x}\n", virt);

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

        // klog("* UNMAP {08x}        {6S}\n", virt, size);

        for (size_t i = 0; i < size/page_size; ++i)
            unmap_one(virt + i*page_size);
    }

    /// Creates a memory mapping.
    ///
    /// if phy is 0, tries to allocate a physical page.
    static bool map_one(addr_t virt, addr_t phy, u32 flags) {
        // klog("  map {08x} -> {08x}\n", virt, phy);

        PageTab *tab_ = get_tab(virt, true);

        if (!tab_) return false;
        // klog("  map {08x} -> {08x} have tab: {}\n", virt, phy, tab_);

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
    errno_t map(addr_t virt, addr_t phy, size_t size, u32 flags) {

        assert(size % page_size == 0, "attempted map() of non-page-aligned size");

        // klog("* MAP {08x} -> {08x} {6S}\n", virt, phy, size);
        // kprint("* MAP {08x} -> {08x} {6S}\n", virt, phy, size);

        for (size_t i = 0; i < size/page_size; ++i) {
            errno_t err = map_one(virt + i*page_size
                                 ,phy ? phy  + i*page_size : 0
                                 ,flags);
            if (err < 0) {
                // map failed - remove mappings up to this point.
                unmap(virt, i*page_size);

                return err;
            }
        }
        return ERR_success;
    }

    errno_t map_mmio(addr_t &virt, addr_t phy, size_t size, u32 flags) {

        assert(phy, "map_mmio makes no sense without a physical address");

        flags |= flag_borrowed | flag_nocache;

        if (virt) {
            return map(virt, phy, size, flags);
        } else {
            virt = 0;
            static size_t mmio_mapped = 0;
            if (size <= Layout::kernel_mmio().size - mmio_mapped) {
                virt = Layout::kernel_mmio().start + mmio_mapped;

                errno_t err = map(virt, phy, size, flags);
                if (err >= 0)
                    mmio_mapped += size;
                return err;
            } else {
                kprint("vmm: warning: no mmio space left for map_mmio()\n");
                return ERR_nomem;
            }
        }
    }

    bool is_mapped(addr_t virt, size_t size) {

        size_t page_offset = virt & 0xfff;
        virt &= ~0xfff;

        size += page_offset;
        size_t npages = div_ceil(size, page_size);

        for (size_t pn : range(npages)) {

            addr_t a = virt + pn*page_size;

            // The page table must exist and the page must be marked present.
            PageTab *tab = get_tab(a);
            if (!tab) return false;
            if (!((*tab)[addr_pagei(a)] & flag_present)) return false;
        }

        return true;
    }

    address_space_t *make_address_space() {
        address_space_t *space = new address_space_t; if (!space) return nullptr;
        PageDir *pd_     = new_pdir(); if (!pd_)     { delete space; return nullptr; }
        PageDir *pt_rec_ = new_ptab(); if (!pt_rec_) { delete space; delete pd_; return nullptr; }

        PageDir &pd     = *pd_;
        PageTab &pt_rec = *pt_rec_;

        addr_t pdir_pa = virtual_to_physical(&pd);
        addr_t ptab_pa = virtual_to_physical(&pt_rec);

        // Clone recursive pagetable.
        pt_rec = *recursive_tab_;

        // Populate the new page directory.
        for (size_t i : range(1024)) {
            // (  0- 255 / 0x00000000-0x3fffffff = kernel mem
            // ,256-1023 / 0x40000000-0xffffffff = user mem)
            if (i >= 256) {
                // Zero user memory entries.
                pd[i]     = 0;
                pt_rec[i] = 0;
            } else {
                // Copy kernel memory pagetable numbers.
                pd[i] = current_dir()[i];
                // the kernel pagetables themselves need not be cloned, they
                // are the same for all tasks - except for the recursive
                // mapping pagetable, which we clone below.
            }
        }

        // - Insert new recursive pagetable into new pdir.
        // - Map the pagetable recursively.
        pd    [addr_tab(Layout::page_tables().start)] = make_pde(ptab_pa, flag_present | flag_writable);
        pt_rec[addr_tab(Layout::page_tables().start)] = make_pte(ptab_pa, flag_present | flag_writable);

        space->pd     = &pd;
        space->pt_rec = &pt_rec;
        return space;
    }

    void delete_address_space(address_space_t *space) {

        // Temporarily switch to the given address space so that we can easily
        // reach all its mappings.
        //
        PageDir &old = current_dir();
        switch_address_space(*space);

        // Find present tables and free all their present, non-borrowed pages.
        for (auto [pt_i, pde] : enumerate(*space->pd)) {
            if (pt_i < 256)
                // Skip kernel mappings - these are never deleted.
                continue;

            if (pde & flag_present) {
                // Page table present? Go through its entries.
                PageTab &table = tables()[pt_i];
                for (pte_t pte : table) {
                    if ((pte & flag_present) && !(pte & flag_borrowed)) {
                        // Page present and owned by this process? De-alloc.
                        Physical::free_one(addr_page(pte_addr(pte)));
                    }
                }
                // De-allocate the page table.
                Physical::free_one(addr_page(pde_addr(pde)));
            }
        }

        // All that now remains in the address space are the global kernel mappings.

        switch_address_space(old);

        delete space->pd;
        delete space->pt_rec;
        delete space;
    }

    void init() {
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

                    if (addr == 0)
                        // Skip the first 4 KiB page.
                        // This makes detecting null-pointer dereferences easier by
                        // triggering pagefaults.
                        // As a side effect, the BIOS Data Area (at 0x400) becomes inaccessible.
                        continue;

                    page = make_pte(addr, flag_present | flag_writable);

                } else {
                    break;
                }
            }
        }

        klog("virtual memory layout:\n");
        klog("  kernel:   {S}\n", Layout::kernel());
        klog("    image:  {S}\n", Layout::kernel_image());
        klog("    heap:   {S}\n", Layout::kernel_heap());
        klog("    mmio:   {S}\n", Layout::kernel_mmio());
        klog("    ptabs:  {S}\n", Layout::page_tables());
        klog("  user:     {S}\n", Layout::user());

        set_pagedir(kernel_dir);

        enable_paging();
    }
}
