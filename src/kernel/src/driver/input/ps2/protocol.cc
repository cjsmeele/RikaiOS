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

DRIVER_NAME("ps2");

namespace Driver::Input::Ps2::Protocol {

    /// How many times do we want to check if data is available before we give up?
    static constexpr size_t max_io_retries = 1'000;

    /// Port 0x64 is a status port when read, and a command port when written.
    static constexpr u16 port_status  = 0x64;
    static constexpr u16 port_command = 0x64;
    static constexpr u16 port_data    = 0x60;

    static constexpr u8 status_output_full = 1 << 0;
    static constexpr u8 status_input_full  = 1 << 1;

    // Commands for the PS/2 controller.
    static constexpr u8 ctl_command_nr_read_cfg  = 0x20;
    static constexpr u8 ctl_command_nr_write_cfg = 0x60;
    static constexpr u8 ctl_command_nr_disable_1 = 0xad;
    static constexpr u8 ctl_command_nr_disable_2 = 0xa7;
    static constexpr u8 ctl_command_nr_enable_1  = 0xae;
    static constexpr u8 ctl_command_nr_enable_2  = 0xa8;
    static constexpr u8 ctl_command_nr_write_2   = 0xd4;

    // Commands for attached PS/2 devices.
    static constexpr u8 command_nr_reset             = 0xff;
    static constexpr u8 command_nr_disable_streaming = 0xf5;
    static constexpr u8 command_nr_enable_streaming  = 0xf4;
    static constexpr u8 command_nr_set_sample_rate   = 0xf3;

    static constexpr u8 cfg_ien_1         = 1 << 0;
    static constexpr u8 cfg_ien_2         = 1 << 1;
    static constexpr u8 cfg_clockdis_1    = 1 << 4;
    static constexpr u8 cfg_clockdis_2    = 1 << 5;
    static constexpr u8 cfg_translation_1 = 1 << 6;

    static constexpr u8 response_ack = 0xfa;

    bool output_full() {
        return Io::in_8(port_status) & status_output_full;
    }

    static bool wait_for_output_full() {
        for (size_t i : range(max_io_retries)) {
            if (output_full())
                return true;
        }
        return false;
    }

    static bool wait_for_input_empty() {
        for (size_t i : range(max_io_retries)) {
            u8 status = Io::in_8(port_status);
            if (!(status & status_input_full))
                return true;
        }
        return false;
    }

    bool read_data(u8 &byte) {
        if (wait_for_output_full()) {
            byte = Io::in_8(port_data);
            return true;
        } else {
            byte = 0;
            return false;
        }
    }

    void flush() {
        while (Io::in_8(port_status) & status_output_full)
            Io::in_8(port_data);
    }

    static bool write_data(u8 byte) {
        if (wait_for_input_empty()) {
            Io::out_8s(port_data, byte);
            return true;
        } else {
            return false;
        }
    }

    static bool write_command(u8 byte) {
        u8 result;
        if (write_data(byte)
         && read_data(result)) {

            return result == response_ack;
        }
        return false;
    }
    static bool write_command(u8 byte1, u8 byte2) {
        u8 result1;
        u8 result2;
        if (write_data(byte1)
         && read_data(result1)
         && write_data(byte2)
         && read_data(result2)) {

            return result1 == response_ack
                && result2 == response_ack;
        }
        return false;
    }


    static bool write_ctl_command(u8 byte) {
        if (wait_for_input_empty()) {
            Io::out_8s(port_command, byte);
            return true;
        } else {
            return false;
        }
    }

    static bool write_aux_data(u8 byte) {
        return write_ctl_command(ctl_command_nr_write_2)
            && write_data(byte);
    }

    static bool write_aux_command(u8 byte) {
        u8 result;
        if (write_ctl_command(ctl_command_nr_write_2)
         && write_data(byte)
         && read_data(result)) {

            return result == response_ack;
        }
        return false;
    }
    static bool write_aux_command(u8 byte, u8 byte2) {
        u8 result1, result2;
        if (write_ctl_command(ctl_command_nr_write_2)
         && write_data(byte)
         && write_ctl_command(ctl_command_nr_write_2)
         && write_data(byte2)
         && read_data(result1)
         && read_data(result2)) {

            return result1 == response_ack && result2 == response_ack;
        }
        return false;
    }


    static bool read_config(u8 &byte) {
        if (write_ctl_command(ctl_command_nr_read_cfg)
         && read_data(byte)) {

            return true;
        } else {
            return false;
        }
    }

    static bool write_config(u8 byte) {
        if (write_ctl_command(ctl_command_nr_write_cfg)
         && write_data(byte)) {

            return true;
        } else {
            return false;
        }
    }

    static bool wait_for_self_test_complete() {

        for (size_t i : range(10)) {
            // We may get an ACK first, don't care.
            u8 byte;
            if (read_data(byte)) {
                if (byte == 0xaa)
                    return true;
            }
        }

        return false;
    }

    static bool get_mouse_id(u8 &id) {
        return (write_aux_command(0xf2)
             && read_data(id));
    }

    init_result_t init_controller() {

        init_result_t result { };

        // Try to disable both the keyboard and mouse during setup.
        write_ctl_command(ctl_command_nr_disable_1);
        write_ctl_command(ctl_command_nr_disable_2);

        // Clear any scancodes / mouse data currently in the buffer.
        flush();

        u8 cfg;
        if (!read_config(cfg)) {
            dprint("error: could not read ps/2 config\n");
            return result;
        }
        // dprint("cfg: {02x}\n", cfg);

        // Disable legacy scancode translation, disable interrupts.
        if (!write_config((cfg & ~(cfg_translation_1
                                  |cfg_ien_1
                                  |cfg_ien_2)))) {

            dprint("error: could not write ps/2 config\n");
            return result;
        }

        // Reset keyboard, run self-test.
        if (write_data(command_nr_reset)
         && wait_for_self_test_complete()) {

            result.have_keyboard = true;
        } else {
            dprint("warning: no keyboard detected - good luck! :-)\n");
        }

        // Reset mouse, run self-test.
        if (write_aux_data(command_nr_reset)
         && wait_for_self_test_complete()) {

            result.have_mouse = true;
        } else {
            dprint("warning: no mouse detected\n");
        }

        // Enable interrupts.
        if (!write_config((cfg & ~(cfg_translation_1
                                  |cfg_clockdis_1
                                  |cfg_clockdis_2))
                          |(result.have_keyboard ? cfg_ien_1 : 0)
                          |(result.have_mouse    ? cfg_ien_2 : 0))) {

            dprint("error: could not write ps/2 config\n");
            return result;
        }

        flush();

        // Enable keyboard.
        if (result.have_keyboard
         && !(write_ctl_command(ctl_command_nr_enable_1)
           && write_command(command_nr_enable_streaming))) {

            dprint("warning: could not enable keyboard\n");
        }

        flush();

        // Enable mouse.
        if (result.have_mouse) {
            if (write_ctl_command(ctl_command_nr_enable_2)
             && write_aux_command(command_nr_enable_streaming)
             && get_mouse_id(result.mouse_type)) {

                // Perform a magical incantation in order to summon the spirit of
                // the mouse's scrollwheel.
                if (!(write_aux_command(command_nr_set_sample_rate, 200)
                   && write_aux_command(command_nr_set_sample_rate, 100)
                   && write_aux_command(command_nr_set_sample_rate,  80)
                   && get_mouse_id(result.mouse_type))) {

                    dprint("warning: could not enable scrollwheel\n");
                }

                // Try to set the sample rate.
                if (!write_aux_command(command_nr_set_sample_rate, 100))
                    dprint("warning: could not set mouse sample rate\n");

            } else {
                dprint("warning: could not enable mouse\n");
            }
        }

        flush();

        return result;
    }
}
