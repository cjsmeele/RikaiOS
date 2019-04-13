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
#include "boot/bootinfo.hh"

/**
 * \namespace Memory::Physical
 *
 * Physical memory manager.
 *
 * The physical memory manager is responsible for allocating and freeing
 * physical memory regions.
 */
namespace Memory::Physical {

    /// \name Functions for querying usage information for physical memory.
    ///@{
    size_t total_pages_free    ();
    size_t total_pages_used    ();
    size_t total_pages_reserved();
    ///@}

    /// \name Debug functions for dumping memory statistics on screen.
    ///@{
    void dump_bitmap();
    void dump_stats();
    ///@}

    /**
     * Tries to allocate one page of memory.
     *
     * If successful, returns the page number. Otherwise, returns 0.
     * (the memory manager will never allow allocations in the first 1 MiB, so
     * 0 is never a valid page number)
     */
    [[nodiscard]] size_t allocate_one();

    /// Frees one page.
    void free_one(size_t page_no);

    /// Initialises the memory manager using memory information from the
    /// boot info struct.
    void init(const boot_info_t &info);
}
