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
#include "types.hh"
#include "console.hh"
#include "memory.hh"
#include "disk.hh"
#include "../include/boot/bootinfo.hh"

extern "C"
int stage2_main(u8  disk_no
               ,u32 kernel_start
               ,u32 kernel_blocks);

extern "C"
int stage2_main(u8  disk_no
               ,u32 kernel_start
               ,u32 kernel_blocks) {

    // Responsibilities of stage2:
    //
    // - Ask the BIOS how much memory is available.
    // - Load the kernel at address 0x00100000
    // - Switch to 32-bit protected mode and run the kernel.

    // Clear stage2's .bss segment.
    // (bss is not included in the stage2 binary, so we must zero it ourselves)
    asm_mem_clear_bss();

    // Enable memory access beyond 1 MB.
    asm_mem_enable_high_memory();

    // Initialize the console so that we can write error messages.
    init_console();
    clear_console();

    // Get a memory layout from the BIOS.
    static memory_map_t map { };
    make_memory_map(map);

    // Make sure that the kernel fits in available memory.
    if (!is_memory_available(map
                            ,0x00100000
                            ,kernel_blocks*512
                            // We don't know the size of the kernel's .bss segment.
                            // For now, assume that 16MiB is enough.
                            +16*1024*1024)) {

        puts("kernel does not fit in available memory, aborting.\n");
        asm_hang();
    }

    // Read the kernel from disk into memory at 1M.
    if (!disk_read((u8*)0x00100000, disk_no, kernel_start, kernel_blocks)) {
        puts("could not read kernel binary from disk.\n");
        asm_hang();
    }

    // Create a structure for information that we pass to the kernel.
    // (We gathered this information in the bootloader because the kernel
    //  won't be able to query the BIOS)
    static boot_info_t boot_info;
    boot_info.memory_regions      = map.regions;
    boot_info.memory_region_count = map.region_count;

    // Jump to the kernel with the given boot information.
    asm_boot(boot_info);

    // (unreachable)
    return 0;
}
