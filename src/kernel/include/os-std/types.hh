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

/// \file Type aliases.

/// \name Common integer types.
///@{

using u64 = unsigned long long;
using u32 = unsigned int;
using u16 = unsigned short;
using u8  = unsigned char;

using s64 = signed long long;
using s32 = signed int;
using s16 = signed short;
using s8  = signed char;

using size_t    = u32;
using ssize_t   = s32;
using ptrdiff_t = s32;
using addr_t    = u32;

// gcc considers these differently from u32/s32 in overloads, clang doesn't.
using uli32 = unsigned long int;
using sli32 =   signed long int;

static_assert(sizeof(uli32)   == sizeof(u32));
static_assert(sizeof(sli32)   == sizeof(s32));
static_assert(sizeof(addr_t)  == sizeof(void*));
static_assert(sizeof(size_t)  >= sizeof(void*));
static_assert(sizeof(ssize_t) >= sizeof(void*));

///@}
/// \name OS integer types.
///@{

using errno_t = s32; ///< Error number.
using pid_t   = s32; ///< Process id.
using tid_t   = s32; ///< Thread id.
using fd_t    = s32; ///< File descriptor.

///@}

// "size" argument type of new/delete calls.
// GCC expects unsigned long int (uli32), clang needs unsigned int (u32/size_t).
#ifdef __clang__
    using malloc_size_t = u32;
#else
    using malloc_size_t = uli32;
#endif
