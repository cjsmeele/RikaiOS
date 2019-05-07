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
#include "vga.hh"
#include "vga/hacks.hh"
#include "memory/manager-virtual.hh"

DRIVER_NAME("vga");

namespace Driver::Vga {

    constexpr u16 port_bochs_vbe_index = 0x1ce;
    constexpr u16 port_bochs_vbe_data  = 0x1cf;

    u32 *framebuffer;

    /// Writes to a VBE register.
    static void write(u16 index, u16 data) {
        Io::out_16(port_bochs_vbe_index, index);
        Io::out_16(port_bochs_vbe_data,  data);
    }

    /// Reads from a VBE register.
    static u16 read(u16 index) {
        Io::out_16(port_bochs_vbe_index, index);
        return Io::in_16(port_bochs_vbe_data);
    }

    /// Set a screen resolution.
    static void mode_set(u16 w, u16 h, u16 bpp) {
        write(4, 0);    // Disable VBE.
        write(1, w);    // Write screen width, height and bits per pixel.
        write(2, h);
        write(3, bpp);
        write(4, 0x41); // Enable VBE, with linear framebuffer (LFB) support.
    }

    static void test(u16 w, u16 h) {

        if (!framebuffer)
            return;

        // Set screen resolution.
        mode_set(w, h, 32);

        // Paint the screen red.
        for (int i : range(w*h))
            framebuffer[i] = 0x00ff0000;

        // Paint a 10x10 white square in the top left corner.
        for (int y : range(10,20))
            for (int x : range(10,20))
                framebuffer[y*w+x] = 0x00ffffff;
    }

    void init() {

        u16 version = read(0);

        if (version >= 0xb0c0 && version <= 0xb0cf) {
            dprint("bochs vbe hardware detected, version {x}\n", version);

            // Magically obtain the location of the memory-mapped I/O region for
            // the Bochs linear framebuffer.
            Memory::region_t phy = Hacks::get_bochs_vbe_lfb_phy_region();

            if (region_valid(phy)) {
                // Map the framebuffer somewhere in virtual memory.
                framebuffer = (u32*)Memory::Virtual::map_mmio(0
                                                             ,phy.start
                                                             ,align_up(phy.size, page_size)
                                                             ,Memory::Virtual::flag_writable);

                dprint("linear framebuffer of {S} at phy {08x}, mapped at {08x}\n"
                      ,phy.size, phy.start, framebuffer);

                // test(1920, 1080);
                // test(1280, 720);
                // test(1024, 768);
                // test(800, 600);
                // test(640, 480);

            } else {
                dprint("error: could not obtain framebuffer address\n");
            }
        }
    }
}
