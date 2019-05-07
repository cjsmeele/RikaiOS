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
#include "pit.hh"
#include "../../interrupt/handlers.hh"
#include "../../process/proc.hh"

DRIVER_NAME("pit");

namespace Driver::Timer::Pit {

    using namespace Interrupt;

    static u64 ticks                  = 0;
    static u64 ticks_in_current_slice = 0;

    static u64 i = 0;

    static void irq_handler(const interrupt_frame_t&) {

        ++ticks;
        ++ticks_in_current_slice;

        // Switch threads if a timeslice is used up.
        if (Process::scheduler_enabled()) {
            if (ticks_in_current_slice >= Process::ticks_per_slice) {

                ticks_in_current_slice = 0;
                Process::yield_noreturn();
            } else {
                ++Process::current_thread()->ticks_running;
            }
        }
    }

    void init() {
        // XXX temporary - sets PIT frequency to 1 KHz.
        Io::out_8s(0x43, 0x34);
        // Io::out_8s(0x40, (8*1193) & 0xff);
        // Io::out_8 (0x40, (8*1193) >> 8);
        Io::out_8s(0x40, (1*1193) & 0xff);
        Io::out_8 (0x40, (1*1193) >> 8);
        // Io::out_8s(0x40, (1193/3) & 0xff);
        // Io::out_8 (0x40, (1193/3) >> 8);

        Handler::register_irq_handler(0, irq_handler);
    }
}

