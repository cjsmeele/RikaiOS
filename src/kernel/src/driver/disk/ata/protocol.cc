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
#include "protocol.hh"

DRIVER_NAME("ata");

namespace Driver::Disk::Ata::Protocol {

    /**
     * Hard-coded IO-port addresses for the primary and secondary ATA bus.
     *
     * These should be the same on all PCs.
     */
    constexpr
    Array<Array<u16,2>,2> iobase {{ { 0x1f0, 0x3f6 }
                                  , { 0x170, 0x376 } }};

    // The max amount of io-waits we will perform while waiting for the status
    // register to change.
    static constexpr size_t max_io_wait = 10_K;

    /**
     * IO port numbers.
     */
    ///@{
    static constexpr u16 port_data      (u8 bus) { return 0 + iobase[bus][0]; }
    static constexpr u16 port_sector_cnt(u8 bus) { return 2 + iobase[bus][0]; }
    static constexpr u16 port_lba0      (u8 bus) { return 3 + iobase[bus][0]; }
    static constexpr u16 port_lba8      (u8 bus) { return 4 + iobase[bus][0]; }
    static constexpr u16 port_lba16     (u8 bus) { return 5 + iobase[bus][0]; }
    static constexpr u16 port_drive     (u8 bus) { return 6 + iobase[bus][0]; }
    static constexpr u16 port_status    (u8 bus) { return 7 + iobase[bus][0]; }
    static constexpr u16 port_command   (u8 bus) { return 7 + iobase[bus][0]; }
    static constexpr u16 port_status_alt(u8 bus) { return 0 + iobase[bus][1]; }
    static constexpr u16 port_control   (u8 bus) { return 0 + iobase[bus][1]; }
    ///@}

    constexpr u8 status_error = 1 << 0;
    constexpr u8 status_drq   = 1 << 3;
    constexpr u8 status_fault = 1 << 5;
    constexpr u8 status_ready = 1 << 6;
    constexpr u8 status_busy  = 1 << 7;

    /// Sets the selected drive of either bus to either 0 or 1 (also called
    /// "master" and / "slave", respectively).
    static void select_drive(u8 bus, u8 drive, u8 lba_bits = 0) {
        Io::out_8(port_drive(bus), 0xe0 | (drive & 1) << 4 | (lba_bits & 0x0f));
        Io::wait(10);
    }

    u8 get_status(u8 bus) { return Io::in_8(port_status(bus)); }

    static errno_t wait_until_not_busy(u8 bus, size_t attempts) {

        for (size_t i = 0; i < attempts; ++i) {
            u8 status = get_status(bus);

            // kprint("id status for ata{}: {02x}\n", bus, status);

            if (status == 0xff || status == 0)
                return ERR_io; // No disk present?

            if (!(status & status_busy))
                return ERR_success;
        }
        return ERR_timeout;
    }

    static errno_t wait_until_drq(u8 bus, size_t attempts) {
        for (size_t i = 0; i < attempts; ++i) {
            u8 status = get_status(bus);

            if (status & status_error) return ERR_io;
            if (status & status_fault) return ERR_io;
            if (status & status_drq)   return ERR_success;
        }
        return ERR_timeout;
    }

    static errno_t wait_until_ready(u8 bus, size_t attempts) {
        for (size_t i = 0; i < attempts; ++i) {
            u8 status = get_status(bus);

            if (status & status_error) return ERR_io;
            if (status & status_fault) return ERR_io;
            if (status & status_ready) return ERR_success;
        }
        return ERR_timeout;
    }

    u64 identify(u8 bus
                ,u8 drive
                ,identify_result_t &result) {

        select_drive(bus, drive);

        // Give the drive some time to get ready.
        if (wait_until_not_busy(bus, max_io_wait)) return false;

        // Send IDENTIFY DEVICE command.
        Io::out_8s(port_command(bus), 0xec);

        if (wait_until_not_busy(bus, max_io_wait)) return false;
        if (wait_until_ready   (bus, max_io_wait)) return false;

        // Read 256 words.
        for (u16 &x : result)
            x = Io::in_16(port_data(bus));

        if (result[0] & 0x8000) return false;

        if (!bit_get(result[49], 9)) {
            dprint("ata{}:{}: dropped: does not support lba\n", bus, drive);
            return false;
        }

        u64 max_lba;

        if (!(result[83] & (1 << 10))) {

            max_lba = u64(result[60]) <<  0
                    | u64(result[61]) << 16;

        } else {
            max_lba = u64(result[100]) <<  0
                    | u64(result[101]) << 16
                    | u64(result[102]) << 32
                    | u64(result[103]) << 48;
        }

        dprint("ata{}:{}: {} disk detected, {} sectors ({S})\n"
              ,bus, drive
              ,result[0] & (1 << 7) ? "removable" : "fixed"
              ,max_lba
              ,max_lba*512);
        // dprint("ata{}:{}: {} sectors ({S})\n", bus, drive, max_lba, max_lba*512);

        if (max_lba >> 28)
            dprint("ata{}:{}: capped at 128GB (lba48 not currently supported)\n", bus, drive);

        // disks[2*bus+drive].exists  = true;
        // disks[2*bus+drive].max_lba = max_lba & 0x0fff'ffff;

        return max_lba & ~(intmax<u64>::value << 28);
    }

    errno_t read_blocks(u8  bus
                       ,u8  drive
                       ,u64 lba
                       ,u8 *dest
                       ,u8  count) {

        errno_t err = ERR_success;

        if (lba >> 28)
            return ERR_io;

        select_drive(bus, drive, lba >> 24);

        if ((err = wait_until_not_busy(bus, max_io_wait)) < 0) return err;
        if ((err = wait_until_ready   (bus, max_io_wait)) < 0) return err;

        Io::out_8(port_sector_cnt(bus), count);
        Io::out_8(port_lba0(bus),       lba >>  0);
        Io::out_8(port_lba8(bus),       lba >>  8);
        Io::out_8(port_lba16(bus),      lba >> 16);
        Io::out_8(port_command(bus),    0x20);

        // For this command, a sector count of 0 means 256 sectors.
        for (int i : range(count == 0 ? 256 : count)) {
            if ((err = wait_until_not_busy(bus, max_io_wait)) < 0) return err;
            if ((err = wait_until_drq     (bus, max_io_wait)) < 0) return err;

            for (int j : range(256)) {
                u16 w = Io::in_16(port_data(bus));
                dest[i*512 + j*2+0] = w;
                dest[i*512 + j*2+1] = w>>8;
            }
            Io::wait(10);
        }

        return ERR_success;
    }

    errno_t write_blocks(u8  bus
                        ,u8  drive
                        ,u64 lba
                        ,const u8 *src
                        ,u8  count) {

        errno_t err = ERR_success;

        if (lba >> 28)
            return ERR_io;

        select_drive(bus, drive, lba >> 24);

        if ((err = wait_until_not_busy(bus, max_io_wait)) < 0) return err;
        if ((err = wait_until_ready   (bus, max_io_wait)) < 0) return err;

        Io::out_8(port_sector_cnt(bus), count);
        Io::out_8(port_lba0(bus),       lba >>  0);
        Io::out_8(port_lba8(bus),       lba >>  8);
        Io::out_8(port_lba16(bus),      lba >> 16);
        Io::out_8(port_command(bus),    0x30);

        // For this command, a sector count of 0 means 256 sectors.
        for (int i : range(count == 0 ? 256 : count)) {
            if ((err = wait_until_not_busy(bus, max_io_wait)) < 0) return err;
            if ((err = wait_until_drq     (bus, max_io_wait)) < 0) return err;

            for (int j : range(256)) {
                Io::out_16(port_data(bus)
                          ,(u16)src[i*512 + j*2+0]
                          |(u16)src[i*512 + j*2+1] << 8);
            }
            Io::wait(10);
        }

        return ERR_success;
    }

    void reset_all() {
        // Reset both ATA buses.
        Io::out_8(port_control(0), 0x04);
        Io::out_8(port_control(1), 0x04);

        Io::wait(10);

        Io::out_8(port_control(0), 0x00);
        Io::out_8(port_control(1), 0x00);

        Io::wait(10);
    }
}
