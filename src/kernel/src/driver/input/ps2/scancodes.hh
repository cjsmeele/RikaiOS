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

#include "../keys.hh"

namespace Driver::Input::Ps2 {

    static constexpr u8 sc_escape       = 0xe0;
    static constexpr u8 sc_break_normal = 0xf0;

    extern const Array<Key, 256> keys_normal;
    extern const Array<Key, 256> keys_escaped;
}
