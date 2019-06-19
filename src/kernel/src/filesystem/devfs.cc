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
#include "devfs.hh"


errno_t DevFs::get_root_node(inode_t &node) {
    node      = inode_t{};
    node.i    = 0;
    node.type = t_dir;
    node.perm = 0500;
    node.size = 0;
    node.fs   = this;

    return ERR_success;
}

errno_t DevFs::get_device_i(StringView name) const {
    for (auto [i, d] : enumerate(devices)) {
        if (d && d->name == name)
            return i;
    }

    return ERR_not_exists;
}

errno_t DevFs::lookup(inode_t &inode, StringView name, inode_t &dest) {
    assert(inode.i == 0, "invalid devfs lookup");
    errno_t dev_i = get_device_i(name);
    if (dev_i < 0) return dev_i;

    dest      = inode_t{};
    dest.i    = dev_i;
    dest.type = t_dev;
    dest.perm = devices[dev_i]->perm;
    dest.size = devices[dev_i]->dev.size();
    dest.fs   = this;

    return ERR_success;
}

ssize_t DevFs::read_dir(inode_t &inode, u64 &pos, dir_entry_t &dest) {

    if (inode.i == 0) {
        // This is the devfs root dir.
        size_t device_exist_i = 0;
        for (auto [i, device] : enumerate(devices)) {
            if (device) {
                if (device_exist_i == pos) {
                    dest.name       = device->name;
                    dest.inode      = inode_t {};
                    dest.inode.i    = i;
                    dest.inode.type = t_dev;
                    dest.inode.perm = device->perm;
                    dest.inode.size = device->dev.size();
                    dest.inode.fs   = this;

                    ++pos;

                    return 1;
                }
                device_exist_i++;
            }
        }
        return 0;

    } else {
        return ERR_not_exists;
    }
}

ssize_t DevFs::read(inode_t &inode, u64 offset, void *buffer, size_t nbytes) {
    assert(inode.i < max_devices, "invalid devfs inode");
    assert(devices[inode.i],      "invalid devfs inode");

    return devices[inode.i]->dev.read(offset, buffer, nbytes);
}

ssize_t DevFs::write(inode_t &inode, u64 offset, const void *buffer, size_t nbytes) {
    assert(inode.i < max_devices, "invalid devfs inode");
    assert(devices[inode.i],      "invalid devfs inode");

    return devices[inode.i]->dev.write(offset, buffer, nbytes);
}

errno_t DevFs::register_device(StringView name, device_t &dev, perm_t perm) {

    int device_i = ERR_limit;

    for (auto [i, entry] : enumerate(devices)) {
        if (i == 0)
            // Skip i 0, is used for root dir.
            continue;

        if (entry) {
            if (entry->name == name)
                return ERR_exists;
        } else if (device_i == ERR_limit) {
            device_i = i;
        }
    }

    if (device_i < 0)
        return device_i;

    devices[device_i] = new device_entry_t(name, dev, perm);
    if (!devices[device_i])
        return ERR_nomem;

    return ERR_success;
}

// Builtin special devices {{{

struct dev_null_t : public DevFs::device_t {
    ssize_t read (u64, void*, size_t) override {
        return 0;
    }
    ssize_t write(u64, const void*, size_t nbytes) override {
        // Ignore writes.
        return nbytes;
    }
    s64 size() override { return 0; }
};
static dev_null_t devnull;

struct dev_zero_t : public DevFs::device_t {
    ssize_t read (u64, void *buffer, size_t nbytes) override {
        memset(buffer, 0, nbytes);
        return nbytes;
    }
    ssize_t write(u64, const void*, size_t nbytes) override {
        // Ignore writes.
        return nbytes;
    }
    s64 size() override { return 0; }
};
static dev_zero_t devzero;

struct dev_full_t : public DevFs::device_t {
    ssize_t read (u64, void *buffer, size_t nbytes) override {
        // For the sake of convenience, this file can also produce 0xff when read.
        // (for NUL bytes, use /dev/zero instead)
        memset(buffer, (char)0xff, nbytes);
        return nbytes;
    }
    ssize_t write(u64, const void*, size_t) override {
        return ERR_nospace;
    }
    s64 size() override { return 0; }
};
static dev_full_t devfull;

struct dev_random_t : public DevFs::device_t {
    ssize_t read (u64, void *buffer, size_t nbytes) override {
        for (size_t i : range(nbytes))
            ((u8*)buffer)[i] = (rand() >> 16) % 256;
        return nbytes;
    }
    ssize_t write(u64, const void*, size_t) override {
        return ERR_nospace;
    }
    s64 size() override { return 0; }
};
static dev_random_t devrandom;

// }}}

DevFs::DevFs() {
    register_device("null",   devnull,   0600);
    register_device("zero",   devzero,   0600);
    register_device("full",   devfull,   0600);
    register_device("random", devrandom, 0400);
}

DevFs::~DevFs() {
}
