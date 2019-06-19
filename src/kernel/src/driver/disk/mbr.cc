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
#include "mbr.hh"
#include "filesystem/vfs.hh"
#include "filesystem/fat32.hh"
#include "../driver.hh"

DRIVER_NAME("mbr");

namespace Driver::Disk::Mbr {

    /// Partition information structure, as contained in the boot sector of a
    /// MBR-partitioned disk.
    struct partition_entry_t {
        char ignored1_[4]; ///< legacy stuff.
        u8   partition_id; ///< indicates file system type.
        char ignored2_[3]; ///< legacy stuff.
        u32  lba_start;    ///< start sector number.
        u32  block_count;  ///< sector count (assuming block size = 512 bytes).
    } __attribute__((packed));

    void scan_and_register_partitions(DevFs::device_t &dev, StringView dev_name) {
        Array<u8,512> mbr;
        ssize_t n = dev.read(0, mbr.data(), mbr.size());

        if (n != 512) {
            if (n < 0)
                 dprint("could not scan partitions on device {}: {}\n", error_name(n));
            else dprint("could not scan partitions on device {}\n");
            return;
        }

        auto const &partition_table = *(Array<partition_entry_t, 4>*)(mbr.data()+0x1be);

        for (auto [i, entry] : enumerate(partition_table)) {
            if (!entry.partition_id || !entry.block_count)
                // Empty partition / zero partition type = invalid.
                continue;

            dprint("primary partition {}p{}: lba {}, {S}\n"
                  ,dev_name
                  ,i
                  ,entry.lba_start
                  ,entry.block_count*512);

            if (entry.lba_start                     <  dev.size()/512
             && entry.lba_start + entry.block_count <= dev.size()/512) {

                DevFs *devfs = Vfs::get_devfs();
                if (devfs) {
                    file_name_t devname = "";
                    fmt(devname, "{}p{}", dev_name, i);

                    DevFs::partition_device_t *part
                        = new DevFs::partition_device_t(dev
                                                       ,entry.lba_start  *512
                                                       ,entry.block_count*512);
                    if (!part) {
                        dprint("error: could not allocate partition device\n");
                        return;
                    }

                    devfs->register_device(devname, *part, 0600);
                    Fat32 *fat = Fat32::try_mount(*part, devname);
                }
            } else {
                dprint("warning: partition {} has invalid dimensions (lba {}, {} blocks)\n"
                      ,i, entry.lba_start, entry.block_count);
            }
        }
    }
}
