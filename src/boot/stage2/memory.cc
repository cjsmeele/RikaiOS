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
#include "memory.hh"
#include "asm.hh"

void make_memory_map(memory_map_t &map) {
    // Clear memory map.
    for (auto &r : map.regions)
        r = {0, 0};

    // "continuation" value.
    // This is passed to the BIOS call so that it can keep
    // track of where we are in the list of memory regions.
    u32 cont = 0;

    for (map.region_count = 0
        ;map.region_count < memory_map_t::max_memory_regions
        ;map.region_count++) {

        bool available = false;
        do {
            bool success =
                asm_get_memory_map(map.regions[map.region_count].start
                                  ,map.regions[map.region_count].size
                                  ,available
                                  ,cont);

            // Ignore any empty regions.
            if (map.regions[map.region_count].size == 0)
                available = false;

            if (!success) return;
            if (!cont) {
                if (available) map.region_count++;
                return;
            }
        } while (!available);
    }
}

bool is_memory_available(const memory_map_t &map
                        ,u32 start
                        ,u32 size) {

    if (size == 0) return true;

    if (start + (size-1) < start)
        // Check for overflow.
        return false;

    for (size_t i = 0; i < map.region_count; ++i) {

        const auto &r = map.regions[i];

        if (start >= r.start
         && r.start + (r.size-1) >= start + (size-1)) {

            // If it fits, I sits.
            return true;
        }
    }
    return false;
}
