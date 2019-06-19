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

namespace Memory {

    /// Represents a region in virtual memory.
    struct region_t {
        addr_t start; /// Starting address (alias for u32).
        size_t size;  /// Region size, must be >0
    };

    /// A memory region is valid if it has non-zero size and start+(size-1)
    /// does not overflow.
    constexpr bool region_valid(region_t r) {
        return r.size > 0 && r.start + (r.size-1) >= r.start;
    }

    /// Checks that two memory regions do *not* overlap.
    constexpr bool regions_disjoint(region_t x, region_t y) {
        return region_valid(x)
            && region_valid(y)
            && ((x.start < y.start && x.start + (x.size-1) < y.start)
             || (y.start < x.start && y.start + (y.size-1) < x.start));
    }

    constexpr bool region_contains(region_t x, region_t y) {
        return region_valid(x)
            && region_valid(y)
            && x.start <= y.start
            && x.start + (x.size-1) >= y.start + (y.size-1);
    }

    constexpr bool addr_in_region(addr_t addr, region_t r) {
        return r.size
            && addr >= r.start
            && addr <= r.start + (r.size-1);
    }
}

namespace ostd::Format {

    /// Formatter for memory regions.
    template<typename F>
    constexpr int format(F &print, Flags f, const Memory::region_t &region) {
        int count = 0;
        count += fmt(print, "{08x} - {08x}"
                    ,region.start
                    ,region.start + (region.size-1));

        if (f.size)
            count += fmt(print, " {6S}", region.size);

        return count;
    }
}
