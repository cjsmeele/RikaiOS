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
#include "proc.hh"
#include "io.hh"

using namespace ostd;


pid_t spawn(ostd::StringView path
           ,int argc
           ,const ostd::StringView *args
           ,bool do_wait
           ,ostd::Array<fd_t, 3> transplanted_files) {

    ostd::Array<bool, 3> created_handles;

    for (auto [i, fd] : enumerate(transplanted_files)) {
        if (fd == spawn_fd_inherit) {
            // Clone handles that need to be inherited.
            fd = duplicate_fd(i);
            if (fd < 0) {
                // Probably we didn't have this file open either, so ignore.
                if  (fd == ostd::ERR_bad_fd) {
                     fd = spawn_fd_clear;
                } else {
                    // Rollback created handles.
                    for (auto [c, fd_] : zip(created_handles, transplanted_files))
                        if (c) close(fd_);
                    return fd;
                }
            }

            created_handles[i] = true;
        }
    }

    errno_t err = sys_spawn(path, argc, args, do_wait, transplanted_files);

    if (err < 0) {
        // spawn failed, we must close any handles we just created.
        for (auto [c, fd] : zip(created_handles, transplanted_files))
            if (c) close(fd);
    }

    return err;
}
