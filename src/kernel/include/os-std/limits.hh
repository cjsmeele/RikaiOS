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

/// \file Size limits.

namespace ostd {

    /// Max length of a single path component.
    static constexpr size_t max_file_name_length =  64;

    /// Max length of a complete path.
    static constexpr size_t max_path_length      = 256;

    /// Max open files across all processes.
    static constexpr size_t max_open_files       = 256;

    /// Max file handles across all processes.
    static constexpr size_t max_file_handles     = 256;

    /// Max file handles per process.
    static constexpr size_t max_proc_files       = 32;

    /// Amount of bytes in a memory page.
    /// Modern x86 can use 4K, 2M, 4M, and even 1G page sizes.
    /// This allows for more efficiency when allocating large amounts of virtual memory.
    /// For the sake of simplicity however, we only support 4K pages.
    constexpr inline size_t page_size            = 4096; // 4K

    /// Max length of a process name.
    static constexpr size_t max_proc_name        = 32;

    /// Max amount of process arguments.
    static constexpr size_t max_args             = 32;

    /// Max total amount of chars in a process' arguments.
    static constexpr size_t max_args_chars       = 1024;
}
