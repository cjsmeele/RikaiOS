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

// Headers shared between kernel/userspace.
#include <os-std/types.hh>
#include <os-std/memory.hh>
#include <os-std/math.hh>
#include <os-std/bitset.hh>
#include <os-std/string.hh>
#include <os-std/literals.hh>

// Internal headers.
#include "common/asm.hh"
#include "common/debug.hh"
#include "common/panic.hh"
#include "common/kprint.hh"

// The ostd namespace is used throughout the kernel - this does not harm other
// libraries and userspace code.
using namespace ostd;
using namespace ostd::literals;
