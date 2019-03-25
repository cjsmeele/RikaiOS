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
#include "disk.hh"
#include "asm.hh"
#include "console.hh"

bool disk_read(u8    *dest
              ,u8     disk_no
              ,u32    block
              ,size_t block_count) {

    // Our block buffer takes up 8K on the stack.
    constexpr size_t buffer_block_count = 16;

    // We read into a buffer on our stack, and then manually copy it to the
    // 32-bit destination.
    // The reason we can't read data directly into high (>1M) memory is that
    // the BIOS function for reading disks does not necessarily support
    // addressing beyond 1M.

    u8 buffer[buffer_block_count * 512];

    for (size_t i = 0
        ;i < block_count
        ;i     += buffer_block_count
        ,block += buffer_block_count
        ,dest  += buffer_block_count) {

        u32 count; // How many blocks to read in this iteration.

        if (block_count - i < buffer_block_count)
             count = block_count - i;
        else count = buffer_block_count; // read max amount.

        if (!asm_disk_read(buffer, disk_no, block, count)) {
            puts("could not read disk\n");
            return false;
        }

        // Copy buffer to destination.
        for (size_t j = 0; j < count*512; ++j)
            dest[j] = buffer[j];
    }

    return true;
}
