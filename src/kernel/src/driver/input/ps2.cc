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
#include "ps2.hh"
#include "../../interrupt/handlers.hh"
#include "../../debug-keys.hh"
#include "ps2/protocol.hh"
#include "ps2/scancodes.hh"
#include "filesystem/vfs.hh"
#include "ipc/queue.hh"

#include "../../kshell.hh"

DRIVER_NAME("ps2");

namespace Driver::Input::Ps2 {

    using namespace Interrupt;
    using namespace Protocol;

    static bool dvorak_enabled = false;

    static bool have_mouse    = false;
    static bool have_keyboard = false;
    static u8   mouse_type    = 0;

    // Keep track of whether we have received magic "escape" or "break" bytes
    // from the keyboard.
    static bool kb_have_escape  = false;
    static bool kb_have_release = false;

    static bool kb_control_pressed = false;
    static bool kb_shift_pressed   = false;
    static bool kb_alt_pressed     = false;

    struct mouse_event_t {
        s16 move_x;
        s16 move_y;
        u8 buttons;
    };

    KQueue<mouse_event_t, 32> mouse_events;
    KQueue<char, 32>          keyboard_input;

    /// Interrupt handler for keyboard events.
    static void irq_handler_keyboard(const interrupt_frame_t &) {
        // Handle keyboard input.

        u8 scancode;
        while (read_data(scancode)) {

            if (scancode == sc_escape) {
                kb_have_escape = true;
                continue;
            } else if (scancode == sc_break_normal) {
                kb_have_release = true;
                continue;
            }

            Key key;

            if (kb_have_escape)
                 key = keys_escaped[scancode];
            else key = keys_normal [scancode];

            if (dvorak_enabled)
                key = keymap_dvorak[(size_t)key];

            // Update modifier keys.
            if (key == Key::LShift   || key == Key::RShift)   kb_shift_pressed   = !kb_have_release;
            if (key == Key::LControl || key == Key::RControl) kb_control_pressed = !kb_have_release;
            if (key == Key::LAlt     || key == Key::RAlt)     kb_alt_pressed     = !kb_have_release;

            // Ignore key releases for now.
            if (kb_have_release) {
                kb_have_release = false;
                kb_have_escape  = false;
                continue;
            }

            kb_have_release = false;
            kb_have_escape  = false;

            if (key == Key::F12) {
                dvorak_enabled = !dvorak_enabled;
                dprint("dvorak keyboard translation {}\n", dvorak_enabled ? "enabled" : "disabled");
                continue;
            }

            char ch = key_to_char(key, kb_shift_pressed, kb_control_pressed, kb_alt_pressed);

            if (Kshell::want_all_input()) {
                if (!Kshell::input.try_enqueue(ch))
                    dprint("kshell input queue full, char dropped\n");
            } else if (!Kshell::enabled() && (key == Key::F1 || key == Key::Esc)) {
                Kshell::enable();
            } else {
                // handle_debug_key(ch);
                if (ch)
                    keyboard_input.try_enqueue(ch);
            }
        }
    }

    /// Interrupt handler for mouse events.
    static void irq_handler_mouse(const interrupt_frame_t &) {
        static int x = 0;
        static int y = 0;

        constexpr static int cx_max = 640;
        constexpr static int cy_max = 480;
        static int cx = 0;
        static int cy = 0;

        static Array<u8, 4> packet;
        static int packet_i = 0;

        u8 b;
        while (read_data(b)) {
            packet[packet_i++] = b;

            // Quick'n'dirty console mouse pointer / paint program.
            //
            if ((mouse_type == 0 && packet_i >= 3) || packet_i >= 4) {

                bool left   = bit_get(packet[0], 0);
                bool right  = bit_get(packet[0], 1);
                bool middle = bit_get(packet[0], 2);

                // xm and ym in our packet are 9-bit signed numbers.
                // extract the sign bit and subtract it from the 8-bit part.
                int xm = s16(packet[1]) - (s16(packet[0]) << 4 & 0x100);
                int ym = s16(packet[2]) - (s16(packet[0]) << 3 & 0x100);

                x = clamp(0, cx_max, x +  xm);
                y = clamp(0, cy_max, y + -ym);
                // kprint("xy {3},{3}\n", x, y);

                packet_i = 0;

                int cx2 = clamp(0, 79, x / (cx_max/80));
                int cy2 = clamp(0, 24, y / (cy_max/25));

                if (left) {
                }
                if (right) {
                    for (int i : range(80*25))
                        ((u8*)0xb8000)[i*2+1] = 0x0f;
                }

                if (cx2 != cx || cy2 != cy) {
                    if (!left)
                        ((u8*)0xb8000)[cy *80*2 + cx *2+1] = 0x0f;
                    ((u8*)0xb8000)[cy2*80*2 + cx2*2+1] = 0xf0;
                    cx = cx2;
                    cy = cy2;
                }

                mouse_event_t event { (s16)xm
                                    , (s16)ym
                                    , (u8)((u8)left | (u8)middle << 1 | (u8)right << 2) };

                mouse_events.try_enqueue(event);
            }
        }
    }

    // Device /dev/mouse
    struct mouse_dev_t : public DevFs::device_t {

        ssize_t read (u64, void *buffer_, size_t nbytes) override {

            char  *buffer  = (char*)buffer_;
            size_t written = 0;

            // Always read exactly one event.
            mouse_event_t event = mouse_events.dequeue();

            String<32> event_text;
            fmt(event_text, "{} {}\n"
               ,event.move_x, event.move_y);

            for (char c : event_text) {
                if (written >= nbytes)
                    break;
                buffer[written++] = c;
            }

            return written;
        }
        ssize_t write(u64, const void*, size_t) override {
            return ERR_nospace;
        }
        s64 size() override { return 0; }
    };
    static mouse_dev_t mouse_dev;

    // Device /dev/keyboard
    struct keyboard_dev_t : public DevFs::device_t {

        ssize_t read (u64, void *buffer_, size_t nbytes) override {

            char  *buffer  = (char*)buffer_;
            size_t written = 0;

            while (written < nbytes) {
                char c = keyboard_input.dequeue();
                // Translate carriage return to newline.
                if (c == '\r') c = '\n';

                if (written == 0 && c == ascii_ctrl('D')) {
                    // ^D, or EOT, indicates end-of-file.
                    // (if encountered in the middle of a line, the character is passed on as-is)
                    return 0;
                }

                buffer[written++] = c;
                if (c == '\n')
                    // Automatic line buffering: Return early on newline.
                    return written;
            }

            return written;
        }
        ssize_t write(u64, const void*, size_t) override {
            return ERR_nospace;
        }
        s64 size() override { return 0; }
    };
    static keyboard_dev_t keyboard_dev;


    void init() {

        init_result_t result = init_controller();

        have_keyboard = result.have_keyboard;
        have_mouse    = result.have_mouse;
        mouse_type    = result.mouse_type;

        DevFs *devfs = Vfs::get_devfs();

        // Register our handlers.
        if (have_keyboard) {
            dprint("ps2:0: keyboard detected\n");
            Handler::register_irq_handler( 1, irq_handler_keyboard);

            if (devfs) devfs->register_device("keyboard", keyboard_dev, 0400);
        }
        if (have_mouse) {
            dprint("ps2:1: mouse detected\n");
            Handler::register_irq_handler(12, irq_handler_mouse);

            if (devfs) devfs->register_device("mouse", mouse_dev, 0400);
        }

        flush();
    }
}
