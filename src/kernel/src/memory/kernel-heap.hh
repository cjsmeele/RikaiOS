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
 * \namespace Memory::Heap
 *
 * Kernel Heap: A memory allocator for the kernel.
 *
 * This allocator is cheap, in the "you get what you pay for" sense. :^)
 *
 * We use perhaps one of the simplest of heap algorithms: We create one doubly
 * linked list for all allocations and holes - that's it.
 *
 * Allocating memory means traversing the list in order to find a hole that
 * fits the requested amount of bytes, including any required alignment (which
 * is crazy inefficient, once the amount of allocations increases).
 *
 * Freeing means finding the allocation structure associated with an address
 * (which is fast, since the structure is located right before the allocated
 * data), marking it as free, and merging it with any neighbour holes.
 *
 * So, the heap structure looks something like this (assuming the
 * 'prev-next-status' allocation struct takes up 12 bytes and the kernel heap
 * starts at byte 100):
 *
 *     ┌─────────────────┐ 100  ─┐
 *     │ prev: *0        │       │
 *     │ next: *132      │       ├─ node 1: an allocation
 *     │ status: used    │       │
 *     ├─────────────────┤ 112   │
 *     │ <20 B of data>  │      ─┘
 *     ├─────────────────┤ 132  ─┐
 *     │ prev: *100      │       │
 *     │ next: *600      │       ├─ node 2: a hole
 *     │ status: free    │       │
 *     ├─────────────────┤ 144   │
 *     │ <456 B empty>   │      ─┘
 *     ├─────────────────┤ 600  ─┐
 *     │ prev: *132      │       │
 *     │ next: *620      │       ├─ node 3: an allocation
 *     │ status: used    │       │
 *     ├─────────────────┤ 612   │
 *     │ <8 B of data>   │      ─┘
 *     ├─────────────────┤ 620  ─┐
 *     │ prev: *600      │       │
 *     │ next: *0        │       ├─ node 4: the last node
 *     │ status: free    │       │
 *     ├─────────────────┤ 632  ─┘
 *     │ <free>          │
 *     └─────────────────┘ ?
 *
 * Note that the last node (node 4 in this case) is never counted as a hole:
 * Instead it can be split up and grown as long as it does not exceed the end
 * of kernel heap memory (at the 1 GiB mark).
 *
 * Normally, whenever we grow the heap at the end, we would ask the virtual
 * memory manager to make that memory available to us (so that it is backed by
 * actual physical memory). Instead, we take a lazier approach: We simply
 * assume that all heap memory exists, and start writing to it when we need it.
 * Writing to unmapped memory then triggers a page fault exception, which we
 * handle by creating the required mapping. \see Interrupt::handle_pagefault
 */
namespace Memory::Heap {

    void dump_stats();
    void dump_all();

    [[nodiscard]]
    void *alloc(size_t size, size_t align);
    void  free(void *p);

    void init();
}
