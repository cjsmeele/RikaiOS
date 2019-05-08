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
#include "debug-keys.hh"
#include "interrupt/interrupt.hh"
#include "memory/manager-physical.hh"
#include "memory/manager-virtual.hh"
#include "memory/kernel-heap.hh"
#include "process/proc.hh"

void handle_debug_key(char ch) {
         if (ch == '\x1b') kprint("ESC"), crash();
    else if (ch == '1')    Memory::Physical::dump_stats();
    else if (ch == '2')    Memory::Physical::dump_bitmap();
    else if (ch == '3')    kprint("<debug3>"); // <- todo: VMM debug keys.
    else if (ch == '4')    (void)Memory::Heap::alloc(1000, 8);
    else if (ch == '5')    Memory::Heap::dump_stats()
                         , Memory::Heap::dump_all();
    else if (ch == '6')    Process::dump_ready_queue();
    else if (ch == '7')    Process::dump_all();
    else if (ch == '8')    kprint("\n** scheduler paused **\n")
                         , Process::pause();
    else if (ch == '9')    kprint("\n** scheduler resumed **\n")
                         , Process::resume();
    else if (ch == '0')    kprint("\n<pressed the dedicated *boop* key - OS was succesfully booped>\n");
    else if (ch == '\x7f') kprint("\n** system reset **\n")
                         , Io::wait(1_M)
                         , Io::out_8(0x64, 0xfe);
    //              ^ DEL char. ^ Reset via PS/2 controller.
}
