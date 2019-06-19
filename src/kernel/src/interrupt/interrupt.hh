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

// #include "common.hh"
#include <os-std/math.hh>
#include "common/asm.hh"

namespace Interrupt {

    inline bool is_enabled() { return ostd::bit_get(asm_eflags(), 9); }

    void disable(); ///< Disables interrupts.
    void enable();  ///< Enables interrupts.

    void init();
}

// The code below can be used to disable interrupts within certain scopes.
// However, the kernel is currently entirely non-preemtible, so these features
// are left unused.  If one were to create a driver that ran a kernel thread
// with interrupts enabled, they would need to use the following structures to
// disable interrupts around all calls to other kernel code.

/// Use RAII to pop state at the end of a scope.
struct critical_scope {
    bool ints_were_enabled;

    critical_scope(const critical_scope& ) = delete;
    critical_scope(      critical_scope&&) = delete;

    critical_scope() : ints_were_enabled (Interrupt::is_enabled()) {
        if (ints_were_enabled) Interrupt::disable();
    }

    ~critical_scope() {
        if (ints_were_enabled) Interrupt::enable();
    }
};

// Macro style, can be slightly more flexibly scoped:

/// Disable interrupts.
#define enter_critical_section()                       \
    bool ints_were_enabled_ = Interrupt::is_enabled(); \
    Interrupt::disable();

/// Re-enable interrupts, if they were disabled by the previous 'enter'.
#define leave_critical_section() \
    if (ints_were_enabled_)      \
        Interrupt::enable();
