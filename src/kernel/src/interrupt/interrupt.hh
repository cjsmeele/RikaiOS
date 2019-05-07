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

#include "common.hh"

namespace Interrupt {

    inline bool is_enabled() { return bit_get(asm_eflags(), 9); }

    void disable(); ///< Disables interrupts.
    void enable();  ///< Enables interrupts.

    void init();
}

/// Disable interrupts.
#define enter_critical_section()                       \
    bool ints_were_enabled_ = Interrupt::is_enabled(); \
    Interrupt::disable();

/// Re-enable interrupts, if they were disabled by the previous 'enter'.
#define leave_critical_section() \
    if (ints_were_enabled_)      \
        Interrupt::enable();
