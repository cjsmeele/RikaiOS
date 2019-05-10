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
#include "kshell.hh"

#include "process/proc.hh"
#include "memory/manager-physical.hh"
#include "memory/kernel-heap.hh"
#include "driver/disk/ata.hh"
#include "driver/vga.hh"
#include "ipc/semaphore.hh"

namespace Kshell {

    KQueue<char, 64> input;

    static bool enabled_ = false;

    semaphore_t lock {{0}, {}};

    void enable() {
        if (!enabled_) {
            Process::pause_userspace();
            enabled_ = true;
            signal(lock);
        }
    }

    static void disable() {
        if (enabled_) {
            enabled_ = false;
            Process::resume_userspace();
            wait(lock);
        }
    }

    bool enabled() { return enabled_; }

    static void run_command(StringView s) {
               if (s == "ps") {
            Process::dump_all();
        } else if (s == "psr") {
            Process::dump_ready_queue();
        } else if (s == "hello") {
            kprint("Hello, world!\n");
        } else if (s == "mem") {
            Memory::Physical::dump_stats();
        } else if (s == "bmap") {
            Memory::Physical::dump_bitmap();
        } else if (s == "heap") {
            Memory::Heap::dump_stats();
            Memory::Heap::dump_all();
        } else if (s == "mbr") {
            alignas(512) Array<u8,512> buf;
            Driver::Disk::Ata::test(buf);
            hex_dump(buf.data(), 512);
        } else if (s == "help") {
            kprint("available commands:\n");
            kprint("\n  {-8} {}", "bmap"    , "print physical memory usage bitmap"    );
            kprint("\n  {-8} {}", "die"     , "crash and burn"                        );
            kprint("\n  {-8} {}", "exit"    , "disable kshell, resume userspace"      );
            kprint("\n  {-8} {}", "heap"    , "print heap statistics"                 );
            kprint("\n  {-8} {}", "hello"   , "print 'Hello, World!'"                 );
            kprint("\n  {-8} {}", "help"    , "print this text"                       );
            kprint("\n  {-8} {}", "mbr"     , "read and print the master boot record" );
            kprint("\n  {-8} {}", "mem"     , "print physical memory statistics"      );
            kprint("\n  {-8} {}", "ps"      , "print process information"             );
            kprint("\n  {-8} {}", "psr"     , "print ready queue"                     );
            kprint("\n  {-8} {}", "reboot"  , "reboot the machine"                    );
            kprint("\n  {-8} {}", "vgatest" , "test video modes"                      );
            kprint("\n\n");
        } else if (s == "vgatest") {
            struct Z { int w, h; };
            for (auto [w,h] : Array { Z{ 640, 480}
                                    , Z{ 800, 600}
                                    , Z{1024, 768}
                                    , Z{1280, 720}
                                    , Z{1920,1080}
                                    , Z{ 320, 200} }) {

                Driver::Vga::test(w, h);
                Io::wait(10_M);
            }
        } else if (s == "kill") {
            Process::thread_t *t = Process::thread_by_tid(rand()%100);
            if (t) Process::delete_thread(t);
        } else if (s == "die") {
            crash();
        } else if (s == "reboot") {
            kprint("\n** system reset **\n");
            // Io::wait(1_M);
            Io::out_8(0x64, 0xfe);
        } else if (s == "exit") {
            kprint("** disabled kshell **\n");
            disable();
            kprint("\n** enabled kshell **\n");
        } else {
            kprint("no such command '{}'\n", s);
        }
    }

    void kshell() {

        wait(lock);

        kprint("\n");
        kprint("** enabled kshell **\n");

        String<256> last_command = "";

        while (true) {

            kprint("kshell> _\b");

            String<256> line;

            while (true) {
                char ch = input.dequeue();

                if (ch == '\b' || ch == 0x7f) {
                    if (line.length()) {
                        line[line.length()-1] = 0;
                        line.length_--;

                        kprint(" \b\b_\b");
                    }

                } else if (ch == '\r') {
                    if (line == "")
                         line = last_command;
                    else last_command = line;

                    kprint(" \n");
                    line.rtrim();
                    if (line.length())
                        run_command(line);
                    break;

                } else if (is_print(ch) && line.length() < line.size()) {
                    line += ch;
                    kprint("{}_\b", ch);
                }
            }
        }
    }
}
