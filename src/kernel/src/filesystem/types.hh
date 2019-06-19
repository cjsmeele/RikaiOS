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
#include "ipc/semaphore.hh"
#include <os-std/fs-types.hh>

struct file_handle_t;
struct file_t;

namespace FileSystem { class Fs; }

/**
 * File information struct.
 *
 * Registers the type, size, permissions, the associated mounted filesystem,
 * and a unique file number within that file system.
 */
struct inode_t {
    u64     i          = 0;
    ftype_t type       = t_regular;
    perm_t  perm       = 0;
    u64     size       = 0;

    FileSystem::Fs *fs = nullptr;

    // Some filesystems may need to store extra information in an inode.
    // For example, in FAT32 we must keep track of the parent directory, since
    // file metadata is stored within directory entries (in contrast to e.g. ext2).
    // These four "context" values can be used by FS implementations as they see fit.
    u64 context1 = 0;
    u64 context2 = 0;
    u64 context3 = 0;
    u64 context4 = 0;
    // Always having these values available instead of creating separate inode
    // types per filesystem simplifies all operations that work on inodes:
    // For example, we do not need to ask the filesystem to allocate inode_t
    // structs for us whenever we iterate over directory contents.
};

/// A directory entry contains a name and file metadata.
struct dir_entry_t {
    String<max_file_name_length> name;
    inode_t inode;
};

namespace FileSystem { class Fs; }

/**
 * Contains unique bookkeeping information for a file that is currently opened
 * via one or more handles.
 *
 * Note that we find file_t structs by full file path: We do not currently
 * support hard- or soft links.
 */
struct file_t {
    int     file_i = -1;
    inode_t inode;
    path_t  path;

    // A linked list containing all current handles for this file.
    file_handle_t *first_handle = nullptr;
    file_handle_t  *last_handle = nullptr;
};

namespace Process { struct proc_t; }

/**
 * A file handle contains an offset, open flags, and (process) owner
 * information for an open file.
 *
 * Eech handle has a globally unique handle_i number (an index into the VFS'
 * global `handles` array), and an fd number (`procfd`) that is unique within
 * the owner process.
 */
struct file_handle_t {
    int          handle_i   = -1;
    file_t      *file       = nullptr;
    open_flags_t flags      = 0;
    u64          pos        = 0;

    Process::proc_t *proc   = nullptr; ///< owner proc.
    fd_t             procfd = -1;      ///< fd number within the proc.

    // Links to other handles for the same file_t.
    file_handle_t *prev = nullptr;
    file_handle_t *next = nullptr;

    /// Any operation on this handle must lock it first.
    /// This prevents two threads in the same proc from interfering with eachother over one file.
    mutex_t lock;
};

// TODO: This does not belong here.
constexpr inline String<10> format_inode_mode(const inode_t &inode) {
    String<10> s = "";

    ftype_t t = inode.type;
    perm_t  p = inode.perm;

    s += t == t_regular ? '-'
       : t == t_dir     ? 'd'
       : t == t_dev     ? 'b' : '?';

    s += p & perm_ur ? 'r':'-'; s += p & perm_uw ? 'w':'-'; s += p & perm_ux ? 'x':'-';
    s += p & perm_gr ? 'r':'-'; s += p & perm_gw ? 'w':'-'; s += p & perm_gx ? 'x':'-';
    s += p & perm_or ? 'r':'-'; s += p & perm_ow ? 'w':'-'; s += p & perm_ox ? 'x':'-';

    return s;
}
