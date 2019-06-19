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
#include "gdt.hh"

namespace Memory::Gdt {

    /**
     * Task State Segment.
     *
     * This structure is used for multitasking: When the processor switches
     * from user-mode to kernel-mode, the CPU loads the kernel stack pointer
     * from the TSS.
     *
     * There's a lot of unused bytes in this struct, which are only interesting
     * when you use "hardware-multitasking", which is a very x86-specific thing
     * that none of the major desktop operating systems do at this point.
     */
    struct tss_t {
        u32 _unused1;
        u32 esp0;         ///< Kernel stack pointer.
        u16 ss0;          ///< Kernel stack (data) segment.
        u8 _unused2[92];
        u16 iomap_offset; ///< Unused, but needs to be filled in.
    } __attribute__((packed));

    /// The Task State Segment.
    tss_t tss;

    /**
     * The Global Descriptor Table.
     *
     * GDT entries (descriptors) are 64-bit bitfields.
     * For the sake of brevity, we hard-code descriptors as 64-bit integers below.
     *
     * The GDT describes memory segments, their offsets, limits and access
     * characteristics (including privileges).
     *
     * Our segments cover the entire 4G memory space, with no offsets. We use
     * this "flat" model so that we can handle all memory layout complexity in
     * one place, using paging.
     *
     * After initialization, the GDT is never modified.
     * We only change the segment registers (cs, ds, etc.) to point to either
     * kernel-mode or user-mode segments, for the sake of security.
     */
    Array table { 0x00'00'00'00'0000'0000ULL    // "null" descriptor, required.
                , 0x00'cf'9a'00'0000'ffffULL    // Kernel code segment (cs).
                , 0x00'cf'92'00'0000'ffffULL    // Kernel data segment (ds, ss, es, fs, gs).
                , 0x00'cf'fa'00'0000'ffffULL    // User code segment (cs).
                , 0x00'cf'f2'00'0000'ffffULL    // User data segment (ds, ss, es, fs, gs).
                , 0x00'50'89'00'0000'0067ULL }; // Task State Segment (TSS).
    //              └┤ ││ └┤ └┤ └──┤ └──┴─ limit bits 0-15
    //               │ ││  │  │    └────── base bits  0-15
    //               │ ││  │  └─────────── base bits 16-23
    //               │ ││  └────────────── access bits
    //               │ │└───────────────── limit bits 16-19
    //               │ └────────────────── flags
    //               └──────────────────── base bits 24─31

    /// A 48-bit datastructure that indicates the location and size of the table.
    static const u64 gdt_ptr = (u64)table.data() << 16 | (sizeof(table) - 1);

    void set_tss_stack(addr_t kernel_stack) {
        tss.ss0  = i_kernel_data*8;
        tss.esp0 = kernel_stack;
    }

    void init() {
        tss.iomap_offset = 0x68; // Indicates we do not have an iomap.

        // Insert the TSS address into the GDT.
        table[i_tss]
             |= (((u64)&tss & 0x00ffffff) << 16)
             |  (((u64)&tss & 0xff000000) << 32);

        // Load the GDT and set the segment registers to point to the kernel
        // code/data segment descriptors.
        asm volatile ("lgdtl (%0)           \
                    \n mov %2,    %%ax      \
                    \n mov %%ax,  %%ds      \
                    \n mov %%ax,  %%es      \
                    \n mov %%ax,  %%fs      \
                    \n mov %%ax,  %%gs      \
                    \n mov %%ax,  %%ss      \
                    \n ljmpl %1, $new_cs___ \
                    \n new_cs___:           \
                    \n mov %3, %%ax         \
                    \n ltr %%ax"

                    :: "a" (&gdt_ptr)
                     , "i" (i_kernel_code*8)
                     , "i" (i_kernel_data*8)
                     , "i" (i_tss        *8)
                     : "cc", "memory");
    }
}
