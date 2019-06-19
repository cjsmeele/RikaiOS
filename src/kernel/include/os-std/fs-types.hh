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

#include "types.hh"
#include "string.hh"
#include "limits.hh"

using path_t      = ostd::String<ostd::max_path_length>;
using file_name_t = ostd::String<ostd::max_file_name_length>;

// File modes:
// On UNIX-like systems, the file mode is a number describing both the file
// type (regular, directory, device or otherwise) and the file permissions.
//
// We separate these properties for the sake of clarity.

enum ftype_t : u32 {
    t_regular = 0,
    t_dir,
    t_dev,
    t_pipe,
};

/// \name File permissions
/// @{

using perm_t = u32; // We only make use of the lower 9 bits currently.

// These octal mode numbers correspond to what UNIX-like systems use.
// We do not currently make use of anything other than the 'user' part however.
static constexpr perm_t perm_ur = 0400;
static constexpr perm_t perm_uw = 0200;
static constexpr perm_t perm_ux = 0100;
static constexpr perm_t perm_gr = 0040;
static constexpr perm_t perm_gw = 0020;
static constexpr perm_t perm_gx = 0010;
static constexpr perm_t perm_or = 0004;
static constexpr perm_t perm_ow = 0002;
static constexpr perm_t perm_ox = 0001;
/// @}

/// \name Open flags
/// @{

using open_flags_t = u32;

static constexpr open_flags_t o_read     = 1 << 0;
static constexpr open_flags_t o_write    = 1 << 1;
static constexpr open_flags_t o_append   = 1 << 2;
static constexpr open_flags_t o_truncate = 1 << 3;
static constexpr open_flags_t o_create   = 1 << 4;
static constexpr open_flags_t o_dir      = 1 << 5;
/// @}

enum seek_t {
    seek_set = 0,
    seek_end,
    seek_cur,
};
