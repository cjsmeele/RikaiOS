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
#include "process/proc.hh"

namespace Elf {

    /**
     * Load an executable from disk.
     *
     * This will attempt to parse the given path as an ELF executable and load
     * it into memory.
     *
     * If succesful, the given program is enqueued and proc will point to it.
     *
     * On failure, the return value will be <0.
     *
     * This may fail if the file is not found / not readable, if the ELF
     * headers are invalid, or the executable is otherwise incompatible with this OS.
     * (you cannot simply copy an ELF from a different unix-like OS and run it!)
     */
    errno_t load_elf(StringView path
                    ,const Process::proc_arg_spec_t &args
                    ,Process::proc_t *&proc);
}
