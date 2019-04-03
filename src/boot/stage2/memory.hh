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

#include "types.hh"
#include "../include/boot/bootinfo.hh"

// Functions for generating and querying the system's memory map.

/// Indicates the location and sizes of all usable memory regions.
struct memory_map_t {
    constexpr static size_t max_memory_regions = 32;

    u32 region_count = 0;
    boot_info_t::memory_region_t regions[max_memory_regions];
};

void make_memory_map(memory_map_t &map);

bool is_memory_available(const memory_map_t &map
                        ,u32 start
                        ,u32 size);
