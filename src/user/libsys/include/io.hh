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
#include <os-std/fmt.hh>
#include <os-std/fs-types.hh>

extern fd_t stdin;
extern fd_t stdout;
extern fd_t stderr;

struct dir_entry_t {
    file_name_t name;

    u64     inode_i;
    ftype_t type;
    perm_t  perm;
    u64     size;
};

/**
 * Open a file.
 *
 * \param[in] fd   (optional) a file descriptor number.
 * \param[in] path the path to the file to open.
 * \param[in] mode whether to open for reading, writing or both.
 *
 * fd should be left out (see the other open() function) unless you want to
 * assign a specific file number.
 * fd can be used to e.g. replace the stdin/out/err files.
 *
 * mode is a string of characters with the following meanings:
 *
 * - 'r': open for reading
 * - 'w': open for writing
 * - 'd': open a directory (implies "r")
 *
 * Directories can only be opened with "d", while regular files cannot be
 * opened with "d".
 * To read from a directory, use the read_dir() function instead of normal read().
 *
 * \return on failure, an error code below 0.
 *
 * A return value >= 0 indicates success.
 *
 * Example:
 *
 *     fd_t fd = open(8, "bla.txt", "r");
 *     if (fd < 0) {
 *         print("error: {}\n", error_name(fd));
 *         return 1;
 *     }
 *     close(fd);
 */
fd_t open(fd_t fd, ostd::StringView path, ostd::StringView mode = "r");

/**
 * Open a file.
 *
 * \param[in] path the path to the file to open.
 * \param[in] mode whether to open for reading, writing or both.
 *
 * mode is a string of characters with the following meanings:
 *
 * - 'r': open for reading
 * - 'w': open for writing
 *
 * \return on failure, an error code below 0.
 *
 * A return value >= 0 indicates success.
 *
 * Example:
 *
 *     fd_t fd = open("bla.txt", "r");
 *     if (fd < 0) {
 *         print("error: {}\n", error_name(fd));
 *         return 1;
 *     }
 *     close(fd);
 */
fd_t open(ostd::StringView path, ostd::StringView mode = "r");

/**
 * Close a file.
 *
 * \param[in] fd     the file to close
 */
errno_t close(fd_t fd);

/**
 * Duplicate a file descriptor.
 *
 * \param[in] dest   the destination file number
 * \param[in] fd     the descriptor to duplicate
 */
fd_t duplicate_fd(fd_t dest, fd_t fd);

/**
 * Create a pipe.
 *
 * \param[out] in     the `in`  part, can be read from
 * \param[out] out    the `out` part, can be written to
 */
errno_t pipe(fd_t &in, fd_t &out);

/**
 * Duplicate a file descriptor.
 *
 * Returns a new file descriptor.
 *
 * \param[in] fd     the descriptor to duplicate
 */
inline fd_t duplicate_fd(fd_t fd) { return duplicate_fd(-1, fd); }

/**
 * Read from a file into a buffer.
 *
 * This should normally not be used directly.
 * Instead, look below for the read function that reads into an Array, String,
 * or other datatypes.
 *
 * \param[in] fd     the file to read from
 * \param[in] p      a pointer to a buffer
 * \param[in] nbytes the amount of bytes to read.
 *
 * \return the amount of bytes that were read (may be lower than nbytes!) or a
 *         value below zero if an error occurred.
 *
 * A return value of 0 indicates that the end-of-file was reached, and no
 * bytes could be read.
 *
 * The amount of bytes that are actually read may be lower than nbytes for
 * various reasons. For example, reading from the keyboard device will always
 * return early when a newline (or carriage return) is encountered.
 */
ssize_t read(fd_t fd, void *p, size_t nbytes);


/**
 * Read a directory entry.
 *
 * \param[in]  fd    the directory to read from
 * \param[out] d     a dir_entry struct
 *
 * A return value of 0 indicates the end of the directory.
 *
 * A return value below 0 indicates an error.
 *
 * A return value above 0 indicates that a directory entry was read into `d`.
 */
ssize_t read_dir(fd_t fd, dir_entry_t &d);

/**
 * Write from a buffer into a file.
 *
 * This should normally not be used directly.
 * Instead, look below for the read function that writes from an Array, String,
 * or other datatypes.
 *
 * \param[in] fd     the file to write to
 * \param[in] p      a pointer to a buffer
 * \param[in] nbytes the amount of bytes to write.
 *
 * \return the amount of bytes that were written (may be lower than nbytes!) or
 *         a value below zero if an error occurred.
 *
 * The amount of bytes that are actually written may be lower than nbytes for
 * various reasons.
 */
ssize_t write(fd_t fd, const void *p, size_t nbytes);

/**
 * Change the current offset into the file where data is written to or read from.
 *
 * \param[in] fd     the file to write to
 * \param[in] whence the type of seek (0 = absolute, 1 is from end, 2 is from current).
 * \param[in] off    teh amount to seek (may be negative)
 *
 * \return 0 on success, a negative value on error
 */
errno_t seek (fd_t fd, int whence, ssize_t off);

/**
 * Shorthand to keep reading till completion.
 *
 * In contrast to the normal read() function, this version keeps trying to read
 * more bytes until nbytes have been read succesfully.
 *
 * \param[in] fd     the file to read from
 * \param[in] p      a pointer to a buffer
 * \param[in] nbytes the amount of bytes to read
 *
 * \return the amount of bytes that were read or a value below zero if an error
 *         occurred.
 */
inline ssize_t read_all (fd_t fd, void *p, size_t nbytes) {

    if (nbytes > ostd::intmax<ssize_t>::value) return ostd::ERR_invalid;

    size_t bytes_read = 0;
    while (bytes_read < nbytes) {
        ssize_t x = read(fd, (u8*)p + bytes_read, nbytes - bytes_read);
        if (x <= 0) return x;
        bytes_read += x;
    }
    return bytes_read;
}

/**
 * Shorthand to keep writing till completion.
 *
 * In contrast to the normal write() function, this version keeps trying to
 * write more bytes until nbytes have been written succesfully.
 *
 * \param[in] fd     the file to write to
 * \param[in] p      a pointer to a buffer
 * \param[in] nbytes the amount of bytes to write
 *
 * \return the amount of bytes that were written or a value below zero if an
 *         error occurred.
 */
inline ssize_t write_all(fd_t fd, const void *p, size_t nbytes) {

    if (nbytes > ostd::intmax<ssize_t>::value) return ostd::ERR_invalid;

    size_t bytes_written = 0;
    while (bytes_written < nbytes) {
        ssize_t x = write(fd, (u8*)p + bytes_written, nbytes - bytes_written);
        if (x < 0) return x;
        bytes_written += x;
    }
    return bytes_written;
}

/// Shorthand to read any object from a file.
template<typename T> ssize_t read (fd_t fd,       T &v) { return read_all (fd, &v, sizeof(v)); }

/// Shorthand to write any object to a file.
template<typename T> ssize_t write(fd_t fd, const T& v) { return write_all(fd, &v, sizeof(v)); }

/// Shorthand to read a string from a file.
template<size_t N> ssize_t read (fd_t fd, ostd::String<N> &v) {
    v.length_ = 0;
    ssize_t n = read_all(fd, v.data(), v.size());
    if (n < 0) return n;
    v[n] = 0;
    v.length = n;
    return n;
}

/// Shorthand to write a string to a file.
inline ssize_t write(fd_t fd, ostd::StringView &v) { return write_all(fd, v.data(), v.length()); }

/// Shorthand to write a string to a file.
template<size_t N> ssize_t write(fd_t &fd, const ostd::String<N> &v) { return write_all(fd, v.data(), v.length()); }


/**
 * Reads a '\n'-terminated line from a file into a string.
 *
 * \return If a complete line was read, returns the length of the line
 *         including the newline character.
 *         If the line is longer than fits in the string, N+1 is returned.
 *         If an error occured, a value lower than 0.
 */
template<size_t N>
ssize_t getline(fd_t fd, ostd::String<N> &str) {
    str.length_ = 0;
    while (str.length() < str.size()) {
        //ssize_t n = sys_read(fd, str.data() + str.length_, str.size() - str.length_);
        // Note: /dev/keyboard and uart are line-buffered magically within the kernel,
        // so for regular files the above does not work (we read too much).
        // This is an inefficient workaround while we do not yet have a proper TTY driver:
        // We read 1 byte at a time.
        ssize_t n = sys_read(fd, str.data() + str.length_, 1);
        if (n <= 0) return n;
        str.length_ += n;
        for (auto [i, c] : enumerate(str)) {
            if (c == '\n' || c == '\r') {
                str[i+1] = 0;
                str.length_ = i+1;
                return str.length();
            }
        }
    }
    return N+1;
}

template<size_t N> ssize_t getline(ostd::String<N> &str) { return getline(stdin, str); }

inline ssize_t getchar(fd_t fd = stdin) {
    char c;
    ssize_t err = read(fd, c);
         if (err < 0)  return err;
    else if (err == 0) return ostd::ERR_eof;
    else             { return c; }
}

inline errno_t putchar(fd_t fd, char c) { return write  (fd,     c); }
inline ssize_t putchar(         char c) { return putchar(stdout, c); }

template<typename... Args>
ssize_t print(fd_t fd, ostd::StringView s, const Args&... args) {
    ssize_t bytes_written = 0;
    errno_t err = 0;
    ostd::fmt([&](char c) {
        if (err < 0) return;
        err = putchar(fd, c);
        if (err >= 0) bytes_written++;
    }, s, args...);

    return err < 0 ? err : bytes_written;
}

template<typename... Args>
ssize_t print(ostd::StringView s, const Args&... args) {
    return print(stdout, s, args...);
}
