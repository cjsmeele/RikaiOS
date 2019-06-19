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

#include "sys.hh"
#include <os-std/string.hh>
#include <os-std/errno.hh>

inline tid_t get_tid() { return sys_get_tid(); }
inline pid_t get_pid() { return sys_get_pid(); }

constexpr inline fd_t spawn_fd_clear   = -1; // fd in spawned proc will be closed.
constexpr inline fd_t spawn_fd_inherit = -2; // fd in spawned proc will match parent's stdin/out/err.

pid_t spawn(ostd::StringView path
           ,int argc
           ,const ostd::StringView *args
           ,bool do_wait = false
           ,ostd::Array<fd_t, 3> transplanted_files = { spawn_fd_inherit
                                                      , spawn_fd_inherit
                                                      , spawn_fd_inherit });

template<size_t N>
pid_t spawn(ostd::StringView path
           ,const ostd::Array<ostd::StringView, N> &args
           ,bool do_wait = false
           ,ostd::Array<fd_t, 3> transplanted_files = { spawn_fd_inherit
                                                      , spawn_fd_inherit
                                                      , spawn_fd_inherit }) {

    return spawn(path, args.size(), args.data(), do_wait, transplanted_files);
}

template<size_t N>
errno_t get_cwd(ostd::String<N> &path) {
    path = "";
    ssize_t len = sys_get_cwd(path.data(), path.size());
    if (len < 0) return len;
    path.length_ = len;
    return len;
}

inline errno_t set_cwd(ostd::StringView path) {
    return sys_set_cwd(path);
}
