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

// Here we include headers that are needed by pretty much all kernel code.

/// Notify included headers that they are being included in kernel code.
/// (they may add additional functionality when included in userspace code)
#define KERNELSPACE

// The os standard library.
#include <os-std/ostd.hh>

// Internal headers.
#include "common/asm.hh"
#include "common/debug.hh"
#include "common/panic.hh"
#include "common/kprint.hh"
#include "common/new-delete.hh"

// The ostd namespace is used throughout the kernel - this does not harm other
// libraries and userspace code.
using namespace ostd;
using namespace ostd::literals;

#define UNIMPLEMENTED assert(0, "tried to use an unimplemented feature");

// Triggers an exception, leading to a kernel panic.
// Useful for code readability when calling functions that cannot return.
#define UNREACHABLE __builtin_unreachable();

#ifndef KERNEL_VERSION
#define KERNEL_VERSION "9999"
#endif
