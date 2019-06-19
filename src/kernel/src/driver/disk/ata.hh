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

#include "../driver.hh"

namespace Driver::Disk::Ata {

    /// We only support disks with this block size.
    static constexpr size_t block_size = 512;

    /**
     * Read block_count amount of sectors starting at sector lba into buffer.
     *
     * This may block the current thread.
     */
    errno_t read (u8 disk_no, u64 lba, u8 *buffer, size_t block_count);

    /**
     * Write block_count amount of sectors starting at sector lba from buffer.
     *
     * This may block the current thread.
     */
    errno_t write(u8 disk_no, u64 lba, const u8 *buffer, size_t block_count);

    void init();

    void test(Array<u8, 512> &buf);
}
