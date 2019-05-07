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

/**
 * \file
 *
 * This includes all ostd headers.
 */

/**
 * The operating system standard library.
 *
 * Somewhat similar to the C++ standard library, the code in this library is not
 * meant to be easily understandable, but must provide an easy-to-grasp interface
 * for programmers exposed to the basics of the C++ standard library (e.g.
 * interactions with std::string, std::vector and cmath).
 */
namespace ostd { }

#include "types.hh"
#include "type-traits.hh"
#include "math.hh"
#include "literals.hh"
#include "memory.hh"
#include "atomic.hh"
#include "fmt.hh"
#include "container.hh"
#include "array.hh"
#include "bitset.hh"
#include "string.hh"
#include "queue.hh"
