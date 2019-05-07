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
 * bootloader to set a VBE graphics mode early on (not yet implemented), since
 * the protected mode interface that adapters provide is usually too limited to
 * support any interesting video modes.
 */
namespace Driver::Vga {

    void init();
}
