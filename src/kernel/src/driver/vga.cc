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
#include "filesystem/vfs.hh"

DRIVER_NAME("vga");

namespace Driver::Vga {

    constexpr u16 port_bochs_vbe_index = 0x1ce;
    constexpr u16 port_bochs_vbe_data  = 0x1cf;

    u32 *framebuffer = nullptr;

    u32 width  = 0;
    u32 height = 0;

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
    static void mode_set(u16 w, u16 h, u16 bpp = 32);

    void test(u16 w, u16 h) {

        if (!framebuffer)
            return;

        // Set screen resolution.
        mode_set(w, h, 32);

        // Paint the screen red.
        for (int i : range(w*h))
            framebuffer[i] = 0x00008080;

        // Paint a 10x10 white square in every corner.
        for (int y : range(10)) {
            for (int x : range(10)) {
                framebuffer[                 (y+10)*w+(x+10)] = 0x00ffffff;
                framebuffer[         (w-30) +(y+10)*w+(x+10)] = 0x00ffffff;
                framebuffer[(h-30)*w        +(y+10)*w+(x+10)] = 0x00ffffff;
                framebuffer[(h-30)*w+(w-30) +(y+10)*w+(x+10)] = 0x00ffffff;
            }
        }
    }

    static DevFs::memory_device_t fbdev { };

    static struct mode_device_t : public DevFs::line_device_t<32> {
        errno_t get(String<32> &str) {
            str = "";
            fmt(str, "{}x{}\n", width, height);
            return ERR_success;
        }
        errno_t set(StringView v) {
            String<32> xs = "";
            String<32> ys = "";
            bool have_x = false;
            for (char c : v) {
                if (have_x)
                     ys += c;
                else if (c == 'x')
                     have_x = true;
                else xs += c;
            }

            u32 w, h;

            if (string_to_num(xs, w)
             && string_to_num(ys, h)
             && w % 8 == 0
             && h % 8 == 0
             && w > 8
             && h > 8) {

                if (w != width || h != height)
                    mode_set(w, h);

                return ERR_success;

            } else {
                dprint("invalid vga dimensions: <{}> (x,y must be at least 16 and div by 8)\n", v);
                return ERR_io;
            }
        }
    } modedev;

    static void mode_set(u16 w, u16 h, u16 bpp) {
        width  = w;
        height = h;

        write(4, 0);    // Disable VBE.
        write(1, w);    // Write screen width, height and bits per pixel.
        write(2, h);
        write(3, bpp);
        write(4, 0x41); // Enable VBE, with linear framebuffer (LFB) support.

        // Update /dev/video file size.
        fbdev.region.size = w*h*(bpp/8);
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
                framebuffer = nullptr;
                addr_t virt = 0;

                errno_t err = Memory::Virtual::map_mmio(virt
                                                       ,phy.start
                                                       ,align_up(phy.size, page_size)
                                                       ,Memory::Virtual::flag_writable);

                if (err < 0) {
                    dprint("could not map framebuffer memory\n");
                    return;
                }

                framebuffer = (u32*)virt;

                dprint("linear framebuffer of {S} at phy {08x}, mapped at {08x}\n"
                      ,phy.size, phy.start, framebuffer);

                // Register device files.

                DevFs *devfs = Vfs::get_devfs();
                if (devfs) {
                    fbdev = { Memory::region_t{ (addr_t)framebuffer, align_up(phy.size, page_size) } };
                    devfs->register_device("video",      fbdev,   0600);
                    devfs->register_device("video-mode", modedev, 0600);
                }
            } else {
                dprint("error: could not obtain framebuffer address\n");
            }
        }
    }
}
