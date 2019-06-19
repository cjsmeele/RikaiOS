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

#include "common.hh"
#include "types.hh"

namespace FileSystem {

    /**
     * Abstract filesystem class.
     *
     * All filesystem implementations must implement this interface.
     */
    class Fs {
    public:
        // Metadata.
        virtual const file_name_t &type() const = 0;
        virtual const file_name_t &name() const = 0;

        // Minimum required features.
        virtual errno_t get_root_node(inode_t &inode) = 0;

        // i is an implementation-defined index number. i 0 must always return
        // the first directory entry, if any.
        // The function will increment i in an implementation-defined manner
        // that ensures all entries are returned exactly once.
        virtual ssize_t read_dir(inode_t &inode, u64 &i, dir_entry_t &dest) = 0;

        // Optional features.

        // lookup and seek have default implementations, the rest of these
        // simply return a "not supported" error code by default.
        virtual errno_t lookup  (inode_t &inode, StringView name, inode_t &dest);
        virtual ssize_t read    (inode_t &inode, u64 offset,       void *buffer, size_t nbytes);
        virtual ssize_t write   (inode_t &inode, u64 offset, const void *buffer, size_t nbytes);
        virtual ssize_t truncate(inode_t &inode, u64 size);
        virtual errno_t unlink  (inode_t &inode, StringView name);
        virtual errno_t create  (inode_t &inode, const file_name_t &name, inode_t &dest);

        virtual errno_t seek(inode_t &inode, u64 &pos, seek_t dir, s64 offset);

        virtual errno_t mkdir (inode_t &inode, StringView name);
        virtual errno_t rmdir (inode_t &inode, StringView name);
        virtual errno_t rename(inode_t &src_inode, StringView src_name
                              ,inode_t &dst_inode, StringView dst_name);
    };

    void init();
}
