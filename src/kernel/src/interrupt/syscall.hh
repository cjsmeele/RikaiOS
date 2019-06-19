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

#include "frame.hh"

namespace Interrupt::Syscall {

    /**
     * Handle a request by a user-mode process.
     *
     * Arguments are captured from the registers at the time of the interrupt
     * (in `frame`).
     *
     * The return value is stored in frame.regs.eax.
     * Return values <0 indicate an error condition.
     */
    void handle_syscall(Interrupt::interrupt_frame_t &frame);
}
