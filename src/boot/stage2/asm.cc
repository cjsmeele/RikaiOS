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
#include "asm.hh"
#include "../include/boot/bootinfo.hh"

// C++ wrappers for code that we can only write in assembler,
// such as BIOS function calls.

// Time & cpu control {{{

void asm_hang() {
    asm volatile ("cli \
                \n hlt");
}

void asm_boot(boot_info_t &boot_info) {

    // Switch to protected mode and jump to the kernel at 1M.

    // This assumes asm_mem_enable_high_memory() has been called already
    // and that kernel code has been loaded at 0x00100000.

    asm ("cli                                \
       \n                                    \
       \n // Enable protected mode           \
       \n mov %%cr0, %%eax                   \
       \n orb $1, %%al                       \
       \n mov %%eax, %%cr0                   \
       \n                                    \
       \n // Load 32-bit data segment        \
       \n mov %1,   %%ax                     \
       \n mov %%ax, %%ds                     \
       \n mov %%ax, %%es                     \
       \n mov %%ax, %%fs                     \
       \n mov %%ax, %%gs                     \
       \n mov %%ax, %%ss                     \
       \n                                    \
       \n // Load 32-bit code segment        \
       \n // and jump to kernel entrypoint   \
       \n ljmpl %0, $0x00100000"

       :: "i" (1*8)        // code segment descriptor number.
        , "i" (2*8)        // data segment descriptor number.
        , "d" (&boot_info) // A pointer to boot information is passed in EDX.
        : "eax", "memory", "cc");

    //((void(*)())(u32*)0x00100000)();
}

// }}}
// Text output (video) {{{

void asm_console_print_char(char c) {
    asm ("int $0x10"
      :: "a" (c | 0x0e00)
       , "b"     (0x0007)
       : "cc");
}

void asm_console_clear() {
    // Clear screen.
    asm ("int $0x10"
      :: "a" (0x0600)
       , "b" (0x0700)
       , "c" (0x0000)
       , "d" (25 << 8 | 80)
       : "cc");

    // Reset cursor position.
    asm ("int $0x10"
      :: "a" (0x0200)
       , "b" (0x0000)
       , "c" (0x0000)
       , "d" (0x0000)
       : "cc");
}

// }}}
// Serial input/output {{{

bool asm_serial_is_available() {
    // Check the relevant bits in the BIOS Data Area.
    return (((*(volatile u16*)0x410) >> 9) & 0x3) > 0;
}

void asm_serial_init() {
    asm ("int $0x14"
      :: "a" (0x0073)
       , "d" (0)
       : "cc");
}

void asm_serial_print_char(char c) {
    asm ("int $0x14"
      :: "a" (0x0100 | c)
       , "d" (0)
       : "cc");
}

// }}}
// Disk input/output {{{

/// Parameter structure for BIOS disk functions.
struct disk_address_packet_t {
    u8  length = 0x10;
    u8  _1 = 0;
    s8  block_count; ///< The amount of blocks to read (max. 127).
    u8  _2 = 0;
    u16 dest_offset;
    u16 dest_segment = 0;
    u64 lba;         ///< Logical Block Address (block number).
    u64 _3 = 0;
} __attribute__((packed));

bool asm_disk_read(void *dest
                  ,u8    disk_no
                  ,u32   block
                  ,s8    block_count) {

    disk_address_packet_t dap;
    dap.dest_offset = (u16)(u32)dest; // eww.
    dap.lba         = block;
    dap.block_count = block_count;

    u32 success = 0x4200;

    asm ("int $0x13              \
       \n jnc .read_disk_success \
       \n xor %%eax, %%eax       \
       \n jmp .read_disk_done    \
       \n .read_disk_success:    \
       \n mov $1, %%eax          \
       \n .read_disk_done:"

        : "+a" (success)
        : "d"  (disk_no)
        , "S"  (&dap)
        : "cc", "memory");

    return success;
}

// }}}
// Memory {{{

bool asm_get_memory_map(u64  &start
                       ,u64  &size
                       ,bool &available
                       ,u32  &cont) {

    // Output buffer.
    struct { u64 x; u64 y; u32 z; } buffer;

    u32 success = 0x0000e820;

    asm ("int $0x15          \
       \n jnc .no_e820_error \
       \n xor %%eax, %%eax   \
       \n .no_e820_error:"

         : "+b" (cont),
           "+a" (success)
         : "c" (0x00000020) // Buffer size
         , "d" (0x534d4150) // "SMAP".
         , "D" (&buffer)
         : "cc", "memory");

    start     = buffer.x;
    size      = buffer.y;
    available = buffer.z == 1;

    return success != 0;
}

void asm_mem_clear_bss() {
    // These magic symbols are defined in our linker script.
    extern volatile u8 STAGE2_BSS_START;
    extern volatile u8 STAGE2_BSS_END;

    for (auto *p = &STAGE2_BSS_START
        ;      p < &STAGE2_BSS_END
        ;    ++p) {

        *p = 0;
    }
}

void asm_mem_enable_high_memory() {

    // This creates a Global Descriptor Table with 32-bit data and code
    // entries. We then switch to protected mode temporarily to load the GDT
    // and the data segment.
    //
    // Once completed, we can safely perform 32-bit data accesses above 1M / 0x00100000.

    // GDT entries (descriptors) are 64-bit bitfields.
    // For the sake of brevity, we hard-code descriptors as 64-bit integers:

    static u64 gdt[3] = { 0ULL                          // "null" descriptor, required.
                        , 0x00'cf'9a'00'0000'ffffULL    // Code segment (cs).
                        , 0x00'cf'92'00'0000'ffffULL }; // Data segment (ds, ss, es, fs, gs).
    //                      └┤ ││ └┤ └┤ └──┤ ╰──┴── limit bits 0-15
    //                       │ ││  │  │    ╰─────── base bits  0-15
    //                       │ ││  │  ╰──────────── base bits 16-23
    //                       │ ││  ╰─────────────── access bits
    //                       │ │╰────────────────── limit bits 16-19
    //                       │ ╰─────────────────── flags
    //                       ╰───────────────────── base bits 24─31

    // This 48-bit structure indicates the address and size of the GDT.
    static u64 gdt_ptr = (u32)gdt << 16 | (sizeof(gdt) - 1);

    asm ("cli                                \
       \n push %%ds                          \
       \n                                    \
       \n // Enable a20-gate (addresses >1M) \
       \n inb  $0x92, %%al                   \
       \n orb  $2,    %%al                   \
       \n andb $0xfe, %%al // bit0 = reset   \
       \n outb %%al, $0x92                   \
       \n                                    \
       \n // Load GDT                        \
       \n lgdtl (%0)                         \
       \n                                    \
       \n // Enable protected mode           \
       \n mov %%cr0, %%eax                   \
       \n orb $1, %%al                       \
       \n mov %%eax, %%cr0                   \
       \n                                    \
       \n // Load 32-bit data segment        \
       \n mov %1, %%ds                       \
       \n                                    \
       \n // Disable protected mode          \
       \n andb $0xfe, %%al                   \
       \n mov %%eax, %%cr0                   \
       \n                                    \
       \n pop %%ds                           \
       \n sti"

       :: "r" (&gdt_ptr)
        , "b" (2*8) // References data segment descriptor.
        : "eax", "memory", "cc");
}

// }}}
