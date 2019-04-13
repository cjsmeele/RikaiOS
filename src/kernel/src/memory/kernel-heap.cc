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
#include "kernel-heap.hh"
#include "manager-virtual.hh"
#include "layout.hh"

/// Magic marker that indicates the end of the kernel image and the start of the heap.
/// This address is page-aligned.
extern int KERNEL_HEAP_START;

namespace Memory::Heap {

    /// \todo Clean up debugging code.

    static const addr_t heap_start =               Layout::kernel_heap().start;
    static const addr_t heap_end   = heap_start + (Layout::kernel_heap().size - 1);
    static const size_t heap_size  =               Layout::kernel_heap().size;

    size_t alloced_heap_end = heap_start;

    size_t total_allocated = 0;
    size_t total_hole_size = 0;
    size_t total_overhead  = 0;

    static constexpr bool clear_alloced_space = true;

    /// A magic value embedded in allocation structs that can be used to
    /// detect heap corruption.
    static constexpr Array<char,3> sentinel_value       {'U','U','U'}; // 0x555555
    static constexpr Array<char,3> invalidated_sentinel {'V','V','V'}; // 0x565656

    struct node_t {
        node_t *prev;
        node_t *next;
        bool    used;
        Array<char,3> sentinel; // Sentinel value.
    };

    static node_t *first_node    = nullptr;
    static node_t * last_node    = nullptr;

    /// If there are this many unused bytes in a node during allocation,
    /// split the node.
    static constexpr size_t node_split_threshold = sizeof(node_t) + 32;

    /// Check if a node is valid.
    static bool verify_node(const node_t *node) {
        if (// verify sentinel value
            // (node && !mem_eq(node->sentinel, "\x55\x55\x55", 3))
            (node && node->sentinel != sentinel_value)
            // verify that contiguous free nodes are merged
          ||(!node->used && ((node->prev && !node->prev->used)
                           ||(node->next && !node->next->used)))
            // verify node ordering
          ||(node->next && node->next <= node)
          ||(node->prev && node->prev >= node)) {

            return false;
        }
        return true;
    }

    /// Returns the usable amount of bytes within a node.
    static size_t node_size(const node_t *n) {
        if (n->next) return (addr_t)n->next - ((addr_t)n + sizeof(node_t));
        else         return heap_end        - ((addr_t)n + sizeof(node_t)) + 1;
    }

    /// Crash and burn if our stat counters are no longer consistent.
    static void verify_stats() {
        size_t overhead  = (addr_t)first_node - heap_start;
        size_t hole_size = 0;
        size_t allocated = 0;

        for (const node_t *node = first_node; node; node = node->next) {
            overhead += sizeof(node_t);

            if (node->used)
                 allocated += node_size(node);
            else if (node->next)
                 hole_size += node_size(node);
        }

        if (overhead != total_overhead)
            dump_all(),
            panic("heap overhead counter inconsistent: {} vs actual {}"
                 ,total_overhead, overhead);
        if (allocated != total_allocated)
            dump_all(),
            panic("heap allocated counter inconsistent: {} vs actual {}"
                 ,total_allocated, allocated);
        if (hole_size != total_hole_size)
            dump_all(),
            panic("heap hole_size counter inconsistent: {} vs actual {}"
                 ,total_hole_size, hole_size);
    }

    /// Initialize a node with some values.
    static void init_node(node_t *node, node_t *prev, node_t *next, bool used) {
        node->prev     = prev;
        node->next     = next;
        node->used     = used;
        node->sentinel = sentinel_value;
    }

    /// Make sure a node no longer looks like a valid node.
    static void invalidate_node(node_t *node) {
        node->used = false;
        node->sentinel = invalidated_sentinel;
    }

    /// See if data of a given size and alignment can fit in a node.
    /// Returns a pointer to the node, adjusted for padding, if it fits.
    /// If no padding is needed, the original node pointer is returned.
    static node_t *fit_node(const node_t *node, size_t size, size_t alignment) {

        u8 *data = (u8*)align_up((addr_t)node+sizeof(node_t), alignment);
        // kprint("FIT: {} -> {} ? {}\n"
        //       ,node, data, data + size <= (const u8*)node+sizeof(node_t) + node_size(node));

        if (data + size <= (const u8*)node+sizeof(node_t) + node_size(node))
             return (node_t*)(void*)data-1;
        else return nullptr;
    }

    /// Insert a node between prev and next, decreasing the size of prev, if it exists.
    /// prev must be non-existent or free.
    static void insert_node(node_t *dst
                           ,node_t *prev
                           ,node_t *next
                           ,bool used) {

        // Note that we never explicitly map memory: We simply start writing in
        // kernel heap memory, and the page fault handler automatically
        // allocates memory for the accessed pages.

        // kprint("insert node\n");

        // We assert that:
        // prev must be either non-existent (this may the first node)
        // or free (since we are eating from it).

        assert(!(prev && prev->used)
              ,"cannot insert heap node: used prev node cannot be moved or shrunk");

        size_t prev_dst_diff = (u8*)dst - (u8*)prev;

        bool node_created = false;

        if (used && prev && prev_dst_diff < node_split_threshold) {

            // kprint("reuse prev for allocation ({} vs new {}, {} diff)\n"
            //       ,prev
            //       ,dst
            //       ,prev_dst_diff);

            // dst and prev are too close, we must move prev into dst instead.
            mem_move(dst, prev);
            prev = dst->prev; // NB: prev is now the prev of the original prev.

            dst->used = used;

            if (prev) {
                // We've just enlarged prev->prev, update counters.
                // Since the original prev was free, prev must have been used,
                // so we have effectively increased allocated space.
                total_allocated += prev_dst_diff;

            } else {
                // We effectively changed the heap starting position.
                // (this is not an issue - the moved amount is capped at the
                //  maximum requested alignment)
                // Record it as "overhead".
                total_overhead += prev_dst_diff;
            }
            if (next) {
                // We've eaten memory that was part of a hole.
                total_hole_size -= prev_dst_diff;
            }

        } else {
            // Create a new node.

            // kprint("create node\n");

            init_node(dst, prev, next, used);
            total_overhead  += sizeof(node_t);
            if (next)
                total_hole_size -= sizeof(node_t);

            node_created = true;
        }

        // Update pointers of our surrounding nodes.
        if (prev) prev->next = dst;
        else      first_node = dst;
        if (next) next->prev = dst;
        else      last_node  = dst;

        size_t data_size = node_size(dst);

        if (used) {
            total_allocated += data_size;

            // Is this not the last node?
            // Then we just filled a hole.
            if (next) total_hole_size -= data_size;

        } else {
            // If this is the last node, we just created a hole behind us.
            if (node_created && prev && !next)
                total_hole_size += node_size(prev);
        }
    }

    /// Merge a free node into a free node before it.
    static node_t *merge_into_prev_block(node_t *node) {

        // kprint("MERGE...\n");

        if (node->next) {
            // This is not the last node.
            node->next->prev = node->prev;
            total_hole_size += sizeof(node_t);
        } else {
            // This is the last node.
            total_hole_size -= node_size(node->prev);
        }

        total_overhead -= sizeof(node_t);

        node->prev->next = node->next;

        invalidate_node(node);

        return node->prev;
    }

    /// Tries to merge adjacent free blocks.
    /// This may change the position of node: a new pointer is returned.
    [[nodiscard]] static node_t *maybe_merge_with_adjacents(node_t *node) {
        // kprint("CHECK {}\n", node);

        if (node->used)
            return node;

        // kprint("MERGE1...\n");

        verify_stats();
        if (node->next && !node->next->used)
            node = merge_into_prev_block(node->next);

        verify_stats();

        // kprint("MERGE2...\n");

        // dump_stats();
        // dump_all();

        if (node->prev && !node->prev->used)
            node = merge_into_prev_block(node);

        verify_stats();

        return node;
    }

    void dump_stats() {
        kprint("\nkernel heap:\n");
        kprint("  start @{}\n", (void*)heap_start);
        kprint("  end   @{}\n", (void*)heap_end);
        kprint("  break @{} ({} %)\n", last_node, ((addr_t)last_node - heap_start) / page_size * 100 / (heap_size / page_size));
        kprint("  total allocated: {} ({S} of total {S})\n"
              ,total_allocated
              ,total_allocated
              ,heap_end - heap_start);
        kprint("  total hole size: {} ({S})\n"
              ,total_hole_size
              ,total_hole_size);
        kprint("  total overhead:  {} ({S})\n"
              ,total_overhead
              ,total_overhead);
    }

    void dump_all() {
        kprint("\nkernel heap allocations:\n");
        for (node_t *node = first_node; node; node = node->next) {
            if (node->used) {
                kprint(":{}={}: ", (u8*)node+sizeof(node_t), node_size(node));
            } else if (node->next) {
                kprint("[HOLE {}] ", node_size(node));
            } else {
                kprint("[{}] ", node_size(node));
            }
        }
        kprint("\n");
    }

    void free(void *p) {
        kprint("FREE: {}\n", p);

        if ((addr_t)p < heap_start + sizeof(node_t))
            panic("bad pointer passed to free: {}", p);

        node_t *node = (node_t*)p-1;
        if (!verify_node(node) || !node->used)
            panic("pointer passed to free is invalid node", p);

        node->used = false;
        size_t size = node_size(node);
        total_allocated -= size;
        total_hole_size += size;

        node = maybe_merge_with_adjacents(node);

        verify_stats();
    }

    void *alloc(size_t size, size_t align) {

        // round the aligmnent requirement up to the alignment of a node.
        align = max(align, size_t(alignof(node_t)));

        // round the size up so that the allocation can be safely interleaved
        // with nodes.
        size = align_up(size, alignof(node_t));

        if (!size) size = alignof(node_t);

        // round the alignment up to a power of two.
        if (bit_count(align) != 1)
            align = 1U << 31 >> (count_leading_0s(align) - 1);

        kprint("ALLOC: {} aligned {}\n", size, align);

        for (node_t *node = first_node; node; node = node->next) {

            // Sanity check.
            if (!verify_node(node))
                panic("kernel heap corruption detected at node {}", node);

            // Skip non-free nodes.
            if (node->used) continue;

            // Assumption:
            // At this point neither node->prev nor node->next can be free.
            // Adjacent free nodes can only exist temporarily when created below.

            // See if our data fits in this node.
            node_t *dst = fit_node(node, size, align);

            if (dst) {
                // It does!
                //
                // Try to create a new node after this one for any remaining
                // free space here.

                size_t node_space_available = node_size(node);
                size_t unused_bytes_after
                    // ... the end of the node
                    = sizeof(node_t) + node_space_available
                    // ... minus the space used for alignment (if any)
                    - ((addr_t)dst - (addr_t)node)
                    // ... minus the space actually used for this allocation
                    - (sizeof(node_t) + size);

                // kprint("-> fits in {} @{} ({} avail) with {} rest\n"
                //       ,node
                //       ,dst
                //       ,node_space_available
                //       ,unused_bytes_after);

                if (unused_bytes_after > node_split_threshold) {
                    // We need to create a node so that the padding at the
                    // end can be used for other allocations.

                    node_t *padding = (node_t*)(void*)
                                    ((u8*)dst
                                    + sizeof(node_t)
                                    + size);

                    // kprint("insert padding at {} (would have wasted {})\n", padding, unused_bytes_after);
                    insert_node(padding, node, node->next, false);
                    verify_stats();
                    // dump_stats();
                    // dump_all();

                    assert(fit_node(node, size, align) == dst
                          ,"heap node fit sanity check failed");
                } else {
                    // Too few bytes, consuuuuume them into this allocation instead.
                    // (splitting would create too much overhead)
                    size += unused_bytes_after;
                }

                if (dst != node) {
                    // Due to alignment requirements, the allocation must start
                    // a little further. Create a new node for that.

                    // kprint("insert dst\n");
                    insert_node(dst, node, node->next, true);
                    verify_stats();
                    // dump_stats();
                    // dump_all();
                } else {
                    dst->used = true;
                    total_allocated += size;

                    if (dst->next)
                        // This was not the last node -> We filled a hole!
                        total_hole_size -= size;
                }

                // Finally merge any free blocks we may have created.
                if (dst->prev) dst->prev = maybe_merge_with_adjacents(dst->prev);
                if (dst->next) dst->next = maybe_merge_with_adjacents(dst->next);

                // The aligned address where the actual data will live.
                void *data = (u8*)dst + sizeof(node_t);

                if (clear_alloced_space)
                    mem_set((u8*)data, u8(0), size);

                // dump_stats();
                // dump_all();

                verify_stats();

                return data;
            }
        }
        // dump_stats();
        // dump_all();

        // Failed to find a large enough free node.
        return nullptr;
    }

    void init() {
        first_node = last_node = (node_t*)heap_start;
        insert_node(first_node, nullptr, nullptr, false);
    }
}
