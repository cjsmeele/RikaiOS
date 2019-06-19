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
#include "idle.hh"

namespace Process {

    /**
     * Kernel idle thread.
     *
     * Whenever there is no other ready thread, the idle thread is dispatched.
     * The idle thread puts the CPU in a low-power state until the next interrupt occurs.
     */
    void idle() {
        // Enable interrupts while this thread is running.
        asm volatile ("sti");

        while (true) {
            // Do nothing, wait for the next interrupt.
            asm volatile ("hlt");
        }
    }
}
