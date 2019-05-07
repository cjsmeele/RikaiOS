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
#include "../../driver.hh"

namespace Driver::Disk::Ata::Protocol {

    using identify_result_t = Array<u16, 256>;

    /**
     * Identify a drive (find out its parameters).
     *
     * If the given drive on the given bus exists and is disk, this returns the
     * max LBA (=the amount of addressable sectors).
     *
     * If the drive does not exist or is not addressable as a disk, returns 0.
     */
    u64 identify(u8 bus
                ,u8 drive
                ,identify_result_t &result);

    u64 send_cmd_read(u8  bus
                     ,u8  drive
                     ,u64 lba
                     ,u8  count);

    /// note: a count of 0 means 256 blocks as a special case.
    bool read_blocks(u8  bus
                    ,u8  drive
                    ,u64 lba
                    ,u8 *dest
                    ,u8  count);

    void reset_all();
}
