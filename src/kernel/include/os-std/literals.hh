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

#include <os-std/types.hh>

/**
 * \namespace ostd::literals
 *
 * Literals.
 *
 * This allows for clearer/shorter notation for big numbers.
 * e.g. you can now write 16_MiB instead of 16*1024*1024.
 */
namespace ostd::literals {
    u64 constexpr operator ""_KiB(u64 i) { return i * 1024;                      }
    u64 constexpr operator ""_MiB(u64 i) { return i * 1024 * 1024;               }
    u64 constexpr operator ""_GiB(u64 i) { return i * 1024 * 1024 * 1024;        }
    u64 constexpr operator ""_TiB(u64 i) { return i * 1024 * 1024 * 1024 * 1024; }
    u64 constexpr operator ""_K  (u64 i) { return i * 1024;                      }
    u64 constexpr operator ""_M  (u64 i) { return i * 1024 * 1024;               }
    u64 constexpr operator ""_G  (u64 i) { return i * 1024 * 1024 * 1024;        }
    u64 constexpr operator ""_T  (u64 i) { return i * 1024 * 1024 * 1024 * 1024; }
}
