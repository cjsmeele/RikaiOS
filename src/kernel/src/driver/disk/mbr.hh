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
#include "filesystem/devfs.hh"

namespace Driver::Disk::Mbr {

    /// Scans a device's first sector and registers any primary MBR partitions in /dev.
    /// This will also attempt to mount each partition as a FAT32 filesystem.
    void scan_and_register_partitions(DevFs::device_t &dev, StringView dev_name);
}
