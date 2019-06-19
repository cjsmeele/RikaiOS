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
#include <os-std/string.hh>
#include "interrupt/interrupt.hh"

/// Checks whether log messages should be printed at all.
/// (if false, klog() will not produce output)
bool kprint_debug_mode();

/// Print a character to the kernel console.
void kprint_char(char c);

/// Print a character to the log.
/// If debug mode is on, char is printed to console as well.
void klog_char(char c);

/// Print a string.
inline void kprint(ostd::StringView str) {
    for (char c : str)
        kprint_char(c);
}

/// Print a formatted string.
template<typename... Args>
void kprint(ostd::StringView format
           ,const Args&...   args) {

    ostd::fmt(kprint_char, format, args...);
}

/// Print a log string.
inline void klog(ostd::StringView str) {
    for (char c : str)
        klog_char(c);
}

/// Print a formatted log string.
template<typename... Args>
void klog(ostd::StringView format
         ,const Args&...   args) {

    ostd::fmt(klog_char, format, args...);
}

/// Initialize the kernel console (needs to be done only once).
void kprint_init();
