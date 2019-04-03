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

#include <os-std/fmt.hh>
#include <os-std/string.hh>

/// Reports an error condition and halts the machine (does not return).
void panic(ostd::StringView reason = "");

/// Reports a formatted error condition and halts the machine (does not return).
template<typename... As>
void panic(const char *reason
          ,const As&... args) {

    static ostd::String<80*25> formatted; // a screenful seems like a good limit.
    formatted = "";
    ostd::fmt(formatted, reason, args...);

    panic(formatted);
}
