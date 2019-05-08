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
#include "ata.hh"
#include "ata/protocol.hh"
#include "process/proc.hh"

DRIVER_NAME("ata");

/**
 * \namespace Driver::Disk::Ata
 *
 * A bare-bones ATA disk driver.
 *
 * Only 28-bit LBA PIO-mode reads/writes are supported, limiting disk capacity
 * to 128 GiB, and performance to that of a comatose turtle.
 *
 * ATA overview:
 *
 * side note: Modern computers do not commonly have ATA devices. Instead, they
 * often provide an ATA interface to non-ATA drive interfaces using emulation.
 *
 * PCs usually have two ATA buses: a *primary* and a *secondary* one. Each bus
 * supports two drives: A *master* and a *slave*. A drive can be, for example,
 * a fixed disk (HDD) or a drive for removable discs (such as CD-ROMS).
 *
 * Communication with a drive happens via the IO ports assigned to the ATA bus
 * that the drive is connected to. This means that in order to talk to a drive
 * via one of the buses, we must first tell the bus to "select" that drive.
 *
 * Usually, reading and writing disk sectors happens using Direct Memory Access
 * (DMA): The drive has direct access to system memory, allowing for fast
 * transfers while the CPU can run other processes.
 *
 * Currently, instead of DMA, we use Programmed Input/Output (PIO) mode: In
 * this mode, the CPU must actively read/write the bus' IO ports for all data
 * transfers. This is incredibly slow, but less complex and easier to
 * implement.
 */
namespace Driver::Disk::Ata {

    struct job_t {
        enum class Type {
            none = 0,
            read,
            write
        } type;

        u8     drive_no;
        u8    *buffer;
        size_t size;
    };

    // TODO. (see Protocol::read_blocks)
    // errno_t read (u8 disk_no, u64 lba, u8 *buffer, size_t block_count) {
    //     return -1;
    // }
    // errno_t write(u8 disk_no, u64 lba, const u8 *buffer, size_t block_count) {
    //     return -1;
    // }

    // TODO. Structs for keeping track of drives. {{{
    //
    // struct bus_t {
    //     u8 selected_drive = 255;

    //     Process::thread_t *worker = nullptr;
    // };

    // Array<bus_t, 2> buses {};
    // struct disk_t {
    //     bool exists = false;

    //     u8 bus;
    //     u8 drive;

    //     u64 max_lba;
    // };

    // Array<disk_t, 2*2> disks { {{false, 0, 0, 0}
    //                            ,{false, 0, 1, 0}
    //                            ,{false, 1, 0, 0}
    //                            ,{false, 1, 1, 0}} };

    // }}}

    void init() {

        Protocol::reset_all();

        Protocol::identify_result_t id;

        // Identify disks.
        for (int bus : range(2)) {
            for (int drive : range(2)) {
                u64 max_lba = Protocol::identify(bus, drive, id);
                if (max_lba) {
                    kprint("disk{}: {} sectors\n", bus*2+drive, max_lba);

                    // kprint("\ntrying to read bootsector from ata{}:{}...\n", bus, drive);
                    // alignas(512) u8 data[512];
                    // if (Protocol::read_blocks(bus, drive, 0, data, 1)) {
                    //     dprint("so apparently that worked!\n\n");
                    //     hex_dump(data, 512);
                    // } else {
                    //     dprint("nope :)\n");
                    // }
                }
            }
        }
    }


    void test(Array<u8, 512> &buf) {
        Protocol::read_blocks(0, 0, 0, buf.data(), 1);
    }
}
