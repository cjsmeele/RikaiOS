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
#include "ipc/semaphore.hh"
#include "interrupt/handlers.hh"
#include "filesystem/vfs.hh"
#include "mbr.hh"

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

    /// LBAs can be 28 or 48-bit. 64 1-bits is never a valid LBA.
    static constexpr u64 invalid_lba = intmax<u64>::value;

    struct dev_t : public DevFs::device_t {

        u8   bus_i;
        u8 drive_i;

        u64 max_lba;

        ssize_t read (u64 offset,       void *buffer, size_t nbytes) override {
            if (offset % block_size)            return ERR_io;
            if (nbytes % block_size)            return ERR_io;
            if (offset >= max_lba * block_size) return 0;

            errno_t err = Driver::Disk::Ata::read(bus_i*2+drive_i
                                                 ,offset / block_size
                                                 ,(u8*)buffer
                                                 ,nbytes / block_size);
            if (err) return err;
            else     return nbytes;
        }
        ssize_t write(u64 offset, const void *buffer, size_t nbytes) override {
            if (offset % block_size)            return ERR_io;
            if (nbytes % block_size)            return ERR_io;
            if (offset >= max_lba * block_size) return ERR_nospace;

            errno_t err = Driver::Disk::Ata::write(bus_i*2+drive_i
                                                  ,offset / block_size
                                                  ,(const u8*)buffer
                                                  ,nbytes / block_size);
            if (err) return err;
            else     return nbytes;
        }
        s64 size() override {
            // May not overflow: lba48 means max 48 bits are set.
            return max_lba * block_size;
        }

        dev_t(u8 bus, u8 drive, u64 max_lba_)
            : bus_i(bus), drive_i(drive), max_lba(max_lba_) { }
    };

    struct disk_t {

        u64 max_lba = 0;

        dev_t device_file;

        disk_t(u8 bus, u8 drive, u64 max_lba_)
            : max_lba(max_lba_)
            , device_file(bus, drive, max_lba_) { }
    };

    struct bus_t {
        // A bus can have 2 disks.
        Array<disk_t*, 2>  disks;
        int selected_drive = 0;

        mutex_t lock;
    };

    Array<bus_t, 2> buses;

    static void irq_handler_common(int x) {
        // Nothing to do.
        Protocol::get_status(x);
    }

    static void irq_handler_ata0(const Interrupt::interrupt_frame_t &) { irq_handler_common(0); }
    static void irq_handler_ata1(const Interrupt::interrupt_frame_t &) { irq_handler_common(1); }

    static errno_t make_job(u8     disk_no
                           ,u64     lba
                           ,u8     *buffer
                           ,size_t  block_count
                           ,bool    write) {

        u8   bus_i = disk_no / 2;
        u8 drive_i = disk_no % 2;

        if (  bus_i > buses.size()
         || drive_i > buses[bus_i].disks.size()
         ||!buses[bus_i].disks[drive_i])
            // The disk does not exist.
            return ERR_io;

        bus_t  &bus  = buses[bus_i];
        disk_t &disk = *bus.disks[drive_i];

        for (size_t i : range(block_count)) {
            if (lba + i >= disk.max_lba)
                return ERR_io;

            mutex_lock(bus.lock);

            errno_t err = 0;
            int retries = 0;
            do {
                if (err == ERR_timeout)
                    Process::yield();

                if (write) {
                    err = Protocol::write_blocks(bus_i
                                                ,drive_i
                                                ,lba    + i
                                                ,buffer + i*block_size
                                                ,1);
                } else {
                    err = Protocol::read_blocks(bus_i
                                               ,drive_i
                                               ,lba    + i
                                               ,buffer + i*block_size
                                               ,1);
                }
            } while (err == ERR_timeout && retries++ < 10);

            if (err < 0) {
                mutex_unlock(bus.lock);
                Process::yield();
                return err;
            }

            mutex_unlock(bus.lock);
            Process::yield();
        }

        return ERR_success;
    }

    errno_t read (u8 disk_no, u64 lba, u8 *buffer, size_t block_count) {
        return make_job(disk_no, lba, buffer, block_count, false);
    }
    errno_t write(u8 disk_no, u64 lba, const u8 *buffer, size_t block_count) {
        // const_cast: eww. it saves so much code duplication though.
        return make_job(disk_no, lba, const_cast<u8*>(buffer), block_count, true);
    }

    void init() {
        Interrupt::Handler::register_irq_handler(14, irq_handler_ata0);
        Interrupt::Handler::register_irq_handler(15, irq_handler_ata1);

        Protocol::reset_all();

        Protocol::identify_result_t id;

        // Identify disks.
        for (auto [bus_i, bus] : enumerate(buses)) {
            for (auto [drive_i, disk_p] : enumerate(bus.disks)) {

                u64 max_lba = Protocol::identify(bus_i, drive_i, id);
                if (max_lba) {
                    // This disk exists!

                    disk_p = new disk_t(bus_i, drive_i, max_lba);
                    if (!disk_p) {
                        dprint("failed to create disk structure for disk {}:{}\n", bus_i, drive_i);
                        continue;
                    }

                    DevFs *devfs = Vfs::get_devfs();
                    if (devfs) {
                        String<32> devname = "";
                        fmt(devname, "disk{}", bus_i*2 + drive_i);
                        devfs->register_device(devname, disk_p->device_file, 0600);

                        Mbr::scan_and_register_partitions(disk_p->device_file, devname);
                    }

                    // kprint("disk{}: {} sectors\n", bus_i*2+drive_i, max_lba);
                }
            }
        }
    }

    void test(Array<u8, 512> &buf) {
        Protocol::read_blocks(0, 0, 0, buf.data(), 1);
    }
}
