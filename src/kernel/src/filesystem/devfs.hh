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

#include "filesystem.hh"
#include "ipc/queue.hh"
#include "memory/region.hh"

// TODO: Documentation.

class DevFs : public FileSystem::Fs {

    file_name_t type_ = "devfs";
    file_name_t name_ = "devfs";

public:
    const file_name_t &type() const override { return type_; }
    const file_name_t &name() const override { return name_; }

    struct device_t {
        virtual ssize_t read (u64 offset,       void *buffer, size_t nbytes) = 0;
        virtual ssize_t write(u64 offset, const void *buffer, size_t nbytes) = 0;
        virtual s64     size() = 0;
    };

    struct partition_device_t : public device_t {
        device_t &dev;

        u64 part_offset;
        u64 part_size;

        ssize_t read (u64 offset,       void *buffer, size_t nbytes) override {
            if (offset >= part_size)
                return 0;
            nbytes = min(nbytes, u32(part_size - offset));
            return dev.read(part_offset + offset, buffer, nbytes);
        }
        ssize_t write(u64 offset, const void *buffer, size_t nbytes) override {
            if (offset >= part_size)
                return ERR_nospace;
            nbytes = min(nbytes, u32(part_size - offset));
            return dev.write(part_offset + offset, buffer, nbytes);
        }

        s64 size() override { return part_size; }

        partition_device_t(device_t &d, u64 offset, u64 size)
            : dev(d)
            , part_offset(offset)
            , part_size(size) { }
    };

    template<size_t N = 1_K>
    struct line_device_t : public device_t {
        String<N> string;
        String<N> input;

        virtual errno_t get(String<N> &str) = 0;
        virtual errno_t set(StringView v) = 0;

        ssize_t read (u64 offset, void *buffer, size_t nbytes) override {
            char *buf = (char*)buffer;
            errno_t err = get(string);
            if (err < 0) return err;
            if (offset >= string.length())
                return 0;
            size_t to_copy = min(nbytes, string.length() - u32(offset));
            for (size_t i : range(to_copy))
                buf[i] = string[offset + i];

            return to_copy;
        }
        ssize_t write(u64 offset, const void *buffer, size_t nbytes) override {
            // Assume the write is buffered correctly.

            (void)offset;

            input = "";
            char *buf = (char*)buffer;
            size_t to_copy = min(nbytes, N);
            for (size_t i : range(to_copy)) {
                if (buf[i] == '\n') {
                    break;
                } else {
                    input += buf[i];
                }
            }
            errno_t err = set(input);
            if (err) return err;
            else     return to_copy;
        }
        s64 size() override {
            return string.length();
        }
    };

    struct memory_device_t : public device_t {
        Memory::region_t region;

        ssize_t read (u64 offset,       void *buffer, size_t nbytes) override {
            if (offset >= region.size) {
                return 0;
            } else {
                // (now we also know offset fits in a u32)
                      u8 *buf = (u8*)buffer;
                const u8 *mem = (u8*)region.start;
                size_t to_copy = min(nbytes, region.size - u32(offset));
                memcpy(buf, mem+offset, nbytes);
                return to_copy;
            }
        }
        ssize_t write(u64 offset, const void *buffer, size_t nbytes) override {
            if (offset >= region.size) {
                return ERR_nospace;
            } else {
                const u8 *buf = (const u8*)buffer;
                      u8 *mem = (u8*)region.start;
                size_t to_copy = min(nbytes, region.size - u32(offset));
                memcpy(mem+offset, buf, nbytes);
                return to_copy;
            }
        }
        s64 size() override {
            return region.size;
        }

        memory_device_t(Memory::region_t x = { })
            : region(x) { }
    };

private:
    struct device_entry_t {
        file_name_t  name;
        device_t    &dev;
        perm_t       perm;

        device_entry_t(StringView x, device_t &y, perm_t z)
            : name(x), dev(y), perm(z) { }
    };

    static constexpr size_t max_devices = 64;
    Array<device_entry_t*, max_devices> devices;

    errno_t get_device_i(StringView name) const;

public:
    errno_t get_root_node(inode_t &node) override;
    errno_t lookup  (inode_t &inode, StringView name, inode_t &dest)                override;
    ssize_t read_dir(inode_t &inode, u64 &i, dir_entry_t &dest)                     override;
    ssize_t read    (inode_t &inode, u64 offset,       void *buffer, size_t nbytes) override;
    ssize_t write   (inode_t &inode, u64 offset, const void *buffer, size_t nbytes) override;

    errno_t register_device(StringView name, device_t &dev, perm_t perm);

     DevFs();
    ~DevFs();
};
