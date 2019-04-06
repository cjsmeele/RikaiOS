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
#include "manager-physical.hh"

// This magic symbol indicates the end address of the kernel.
extern int KERNEL_BSS_END;

namespace Memory::Physical {

    // Note that for performance reasons, these counters can only retain
    // accuracy when free() is only called on allocated memory.
    // i.e.: trying to free physical memory twice (double freeing) will result
    // in skewed counters.

    static size_t total_pages_free_     = 0; ///< Memory that can be allocated.
    static size_t total_pages_used_     = 0; ///< Memory that has been allocated.
    static size_t total_pages_reserved_ = 0; ///< Memory-mapped I/O and otherwise unusable or non-existent mem.

    size_t total_pages_free    () { return total_pages_free_;     }
    size_t total_pages_used    () { return total_pages_used_;     }
    size_t total_pages_reserved() { return total_pages_reserved_; }

    /**
     * The memory usage bitmap.
     *
     * There's more than one way to keep track of physical memory allocations.
     * We choose to use a bitmap for its simplicity - it's nothing more than a
     * long array of bits. We can search through it efficiently enough by
     * checking 32 bits at a time, making use of the fact that Bitset<> is
     * actually an Array<> of 32-bit numbers.
     *
     * We make each bit represent one page of memory (4KiB), where a value of 1
     * (true) indicates that it's free, and a value of 0 indicates it's used or
     * reserved.
     *
     * We create a bitmap for the entire 32-bit address space, that is 4GiB.
     * 4G / 4K = 1 Mbit, or 128KiB that we need to store this bitmap - not a
     * whole lot for modern computers.
     */
    static Bitset<4_GiB / page_size> bitmap;

    // Determine how big the underlying values in the bitmap are.
    // (should be u32, such that word_bits is 32)
    using WordType = decltype(bitmap)::T;
    static constexpr size_t word_bits = sizeof(WordType) * 8;

    /// What is the lowest free bit (page) in the bitmap?
    static size_t first_free_page = 0;

    /// Given a starting page, finds the first free page number.
    static size_t find_next_free_page(size_t start_page) {

        // Check the availability of 32 pages (32 bits) at once.
        // Once we find a 32-bit number that isn't 0, we know that we've
        // encountered a free page.
        for (size_t word_i = start_page / word_bits
            ;word_i < bitmap.data().size()
            ;++word_i) {

            WordType word = bitmap.data()[word_i];
            if (word != 0)
                // The first free page will be the lowest '1' bit in this word.
                return word_i * word_bits + count_trailing_0s(word);
        }

        // No free pages found.
        return 0;
    }

    size_t allocate_one() {
        size_t page_no = find_next_free_page(first_free_page);
        if (page_no > 0) {
            // Success!
            bitmap.set(page_no, 0);

            ++total_pages_used_;
            --total_pages_free_;

            first_free_page = page_no+1;
        } else {
            kprint("!! failed to allocate physical memory\n");
        }

        return page_no;
    }

    void free_one(size_t page_no) {
        bitmap.set(page_no, 1);

        --total_pages_used_;
        ++total_pages_free_;

        if (page_no < first_free_page)
            first_free_page = page_no;
    }

    size_t allocate(size_t) {
        /// \todo Allow efficient allocation of multiple pages at once?

        UNIMPLEMENTED

        return 0;
    }

    void free(size_t page_no, size_t n_pages) {
        bitmap.set_range(page_no, n_pages, 1);
        total_pages_used_ -= n_pages;
        total_pages_free_ += n_pages;

        if (page_no < first_free_page)
            first_free_page = page_no;
    }

    void dump_bitmap() {
        kprint("\nphysical memory bitmap:\n{}\n", bitmap);
    }

    void dump_stats() {
        kprint("\n"
               "total unreserved memory: {6S} ({8} pages)\n"
               "total used memory:       {6S} ({8} pages)\n"
               "total free memory:       {6S} ({8} pages)\n"
              ,u64(total_pages_free_ + total_pages_used_) * page_size
              ,    total_pages_free_ + total_pages_used_
              ,u64(total_pages_used_) * page_size, total_pages_used_
              ,u64(total_pages_free_) * page_size, total_pages_free_);
    }

    /// Mark a memory region from the boot info struct as free in our bitmap.
    static void mark_region_free(boot_info_t::memory_region_t region) {

        if (region.start             >= intmax<u32>::value
          ||region.start + page_size >= intmax<u32>::value)
            // Ignore regions that are not accessible using 32-bit addresses.
            // (We don't support more than 4GiB memory)
            return;

        if (region.size < page_size)
            // We cannot allocate anything in such a small region.
            return;

        // Make sure region start and size are rounded to page_size.
        size_t rest = region.start % page_size;
        if (rest) {
            region.start += page_size - rest;
            region.size  -= page_size - rest;
        }

        size_t page_start = region.start / page_size;
        size_t page_count = region.size  / page_size;

        kprint("  {} - {}  {6S} free\n"
              ,(u8*)region.start
              ,(u8*)region.start + (region.size-1)
              ,region.size);

        // Mark as free.
        bitmap.set_range(page_start
                        ,page_count
                        ,1);

        // Update counters.
        total_pages_free_     += page_count;
        total_pages_reserved_ -= page_count;
    }

    void init(const boot_info_t &boot_info) {

        kprint("memory map:\n");

        // Start out assuming all memory is reserved.
        total_pages_reserved_ = intmax<size_t>::value;

        // Mark free regions the bitmap with information from the boot info
        // struct.

        for (size_t i = 0; i < boot_info.memory_region_count; ++i)
            mark_region_free(boot_info.memory_regions[i]);

        // Mark the first 1M as reserved (avoids overwriting boot info
        // structure), and mark kernel code+data+bss as used.

        size_t kernel_pages = div_ceil((u32)&KERNEL_BSS_END - 1_MiB, page_size);
        bitmap.set_range(0,                 1_MiB / page_size, 0);
        bitmap.set_range(1_MiB / page_size, kernel_pages,      0);

        // Update stat counters.
        total_pages_free_     -= 1_MiB / page_size + kernel_pages;
        total_pages_reserved_ += 1_MiB / page_size;
        total_pages_used_     += kernel_pages;

        dump_stats();
    }
}
