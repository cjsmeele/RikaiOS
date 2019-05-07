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
#include "hacks.hh"

DRIVER_NAME("vga");

namespace Driver::Vga::Hacks {

    // This is a dirty hack to get the physical address of the Bochs VBE
    // graphics linear framebuffer.
    //
    // This works in QEMU, Bochs and VirtualBox.
    //
    // We scan the PCI bus for a device with a certain ID, and return its BAR0
    // address and size.
    //
    // This hack is intentionally left barely documented:
    // Writing a cleaner version will require that we integrate PCI support
    // into the kernel, which is beyond our current scope.
    // Getting this one single address is currently the *only* reason we ever
    // interact with PCI in any way.

    // PCI ports.
    constexpr u16 port_addr = 0x0cf8;
    constexpr u16 port_data = 0x0cfc;

    // Device IDs for Bochs-ish graphical adapters.
    // (low 16 bits for the vendor, high 16 bits for the device)
    static constexpr u32 device_id_bochs = 0x1111'1234; // (also QEMU)
    static constexpr u32 device_id_vbox  = 0xbeef'80ee;

    static u32 make_address(u8 bus, u8 slot, u8 func, u8 offset) {
        return        1 << 31
            | u32(bus)  << 16
            | u32(slot) << 11
            | u32(func) <<  8
            | (offset & 0xfc);
    }

    static u32 read_config(u8 bus, u8 slot, u8 func, u8 offset) {

        Io::out_32(port_addr, make_address(bus, slot, func, offset));
        return Io::in_32(port_data);
    }
    static void write_config(u8 bus, u8 slot, u8 func, u8 offset, u32 value) {

        Io::out_32(port_addr, make_address(bus, slot, func, offset));
        Io::out_32(port_data, value);
    }

    static Memory::region_t get_bar0(int bus, int slot, int fn) {
        auto x = read_config(bus, slot, fn, 2*4);
        x = read_config(bus, slot, fn, (4+0)*4);

        if (x) {
            u8 flags = x&0xf;
            write_config(bus,slot,fn,(4+0)*4, ~0xfUL | flags);
            auto y = read_config(bus, slot, fn, (4+0)*4) & ~0xf;
            write_config(bus, slot, fn, (4+0)*4, x);
            return { x&~0xf, ~y+1 };
        }
        return {};
    }

    Memory::region_t get_bochs_vbe_lfb_phy_region() {

        // Enumerate PCI devices.

        for (int bus : range(256)) {
            for (int slot : range(32)) {
                auto x = read_config(bus, slot, 0, 0);
                if (x == intmax<u32>::value)
                    continue;

                x = read_config(bus, slot, 0, 3*4);

                if (x >> 16 & 0x80) {
                    // Multi-function, check all functions.
                    for (int function : range(8)) {
                        x = read_config(bus, slot, function, 0);
                        if (x == device_id_bochs || x == device_id_vbox)
                            return get_bar0(bus, slot, function);
                    }
                } else {
                    x = read_config(bus, slot, 0, 0);
                    if (x == device_id_bochs || x == device_id_vbox)
                        return get_bar0(bus, slot, 0);
                }
            }
        }
        return {};
    }
}
