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

#include "driver.hh"

/**
 * \namespace Driver::Vga
 *
 * Driver for a Bochs-compatible graphics adapter.
 *
 * Supported machines include Bochs, QEMU and VirtualBox.
 *
 * For other (non-virtual) graphics hardware, we need the help of the
 * bootloader to set a VBE graphics mode early on in real mode[1] (not yet
 * implemented) since the standardised protected mode interface that adapters
 * provide is often missing/incomplete (VBE PMI) or too limited (VGA) to
 * support any interesting video modes (unless we want to go with 640x480 with
 * only 16 colors).
 * Switching video modes without standard VGA or VBE will require very
 * hardware-specific drivers, which is beyond our scope.
 *
 * [1] see https://github.com/cjsmeele/stoomboot/blob/master/stage2/src/vbe.c
 *     for older code that can be ported
 */
namespace Driver::Vga {

    void test(u16 w, u16 h);
    void init();
}
