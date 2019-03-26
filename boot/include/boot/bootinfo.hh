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

// Information that we pass on to the loaded kernel.
//
// This header is separate from the stage2 source directory so that it can be
// easily included from kernel code.

struct boot_info_t {

    using u64    = unsigned long long;
    using size_t = unsigned int;

    /// Indicates the location and size of a usable memory region.
    struct memory_region_t {
        u64 start = 0;
        u64 size  = 0;
    };

    memory_region_t *memory_regions = nullptr;
    size_t memory_region_count      = 0;
};
