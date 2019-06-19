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
#include "memory/region.hh"
#include "types.hh"
#include "filesystem.hh"
#include "filesystem/devfs.hh"

/**
 * The virtual filesystem.
 *
 * Abstracts over all filesystems.
 *
 * TODO: API documentation and overview is missing.
 */
namespace Vfs {

    void wait_until_lockable();

    void dump_mounts();
    void dump_open_files();

    DevFs *get_devfs();

    /**
     * Path canonicalisation.
     *
     * - turns relative paths into absolute paths (taking the current
     *   process' working directory into account)
     * - resolves all '..' and '.'
     * - removes double slashes and trailing slashes
     *
     * If `in` is empty, '/' is returned.
     */
    path_t canonicalise_path(StringView in);

    // (leave fd empty or -1 to allocate a file number automatically).
           fd_t open (fd_t fd, StringView path, open_flags_t flags);
    inline fd_t open (         StringView path, open_flags_t flags) {
        return open(-1, path, flags);
    }

    errno_t close(fd_t fd);

    errno_t seek(fd_t fd, seek_t dir, s64 off);

    ssize_t read (fd_t fd,       void *buffer, size_t nbytes);
    ssize_t write(fd_t fd, const void *buffer, size_t nbytes);
    ssize_t read_dir(fd_t fd, dir_entry_t &dest);
    errno_t truncate(fd_t fd);

    // (leave dest empty or -1 to allocate a file number automatically).
    fd_t duplicate_fd(fd_t dest, fd_t fd);

    // The handle is moved from the running process to the destination process.
    // dest is 0-2 in the new proc.
    errno_t transplant_fd(Process::proc_t *dest_proc, fd_t dest, fd_t src);

    errno_t make_pipe(fd_t &in, fd_t &out);

    ssize_t unlink(StringView path);
    ssize_t rmdir (StringView path);
    ssize_t mkdir (StringView path);
    ssize_t rename(StringView path, StringView new_path);

    // Convenience functions {{{

    inline ssize_t read (fd_t fd, char  c) { return read (fd, &c, 1); }
    inline ssize_t write(fd_t fd, char &c) { return write(fd, &c, 1); }

    inline ssize_t read_all(fd_t fd, void *buffer, size_t nbytes) {
        if (nbytes > intmax<ssize_t>::value) return ERR_invalid;

        size_t bytes_read = 0;
        while (bytes_read < nbytes) {
            ssize_t x = read(fd, (u8*)buffer + bytes_read, nbytes - bytes_read);
            if (x < 0)  return x;
            if (x == 0) return 0;
            bytes_read += x;
        }
        return bytes_read;
    }

    inline ssize_t write_all(fd_t fd, const void *buffer, size_t nbytes) {
        if (nbytes > intmax<ssize_t>::value) return ERR_invalid;

        size_t bytes_written = 0;
        while (bytes_written < nbytes) {
            ssize_t x = write(fd, (u8*)buffer + bytes_written, nbytes - bytes_written);
            if (x < 0) return x;
            bytes_written += x;
        }
        return bytes_written;
    }

    template<typename T, size_t N> ssize_t read (fd_t fd,       Array<T,N> &buffer) { return read_all (fd, buffer.data(), sizeof(buffer)); }
    template<typename T, size_t N> ssize_t write(fd_t fd, const Array<T,N> &buffer) { return write_all(fd, buffer.data(), sizeof(buffer)); }
    template<            size_t N> ssize_t read (fd_t fd,       String<N>  &buffer) { return read_all (fd, buffer.data()); }
    template<            size_t N> ssize_t write(fd_t fd, const String<N>  &buffer) { return write_all(fd, buffer.data()); }

    template<typename T> ssize_t read (fd_t fd,       T &val) { return read_all (fd, &val, sizeof(T)); }
    template<typename T> ssize_t write(fd_t fd, const T &val) { return write_all(fd, &val, sizeof(T)); }

    // }}}

    /// Attach a filesystem with the given name under '/'.
    errno_t mount(FileSystem::Fs &fs, StringView name);

    void init();
}
