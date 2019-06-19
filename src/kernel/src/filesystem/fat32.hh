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
#include "filesystem.hh"
#include "devfs.hh"

// TODO: -> into FileSystem namespace. Same with devfs.

// This filesystem implementation is currently read-only.
// Also, quite a bit of documentation is not yet written.

class Fat32 : public FileSystem::Fs {
    file_name_t type_ = "fat32";
    file_name_t name_ = "";

    DevFs::device_t &dev;

    // The only supported block size, for now.
    static constexpr size_t block_size = 512;

    using block_t     = Array<u8,  block_size>;
    using fat_block_t = Array<u32, block_size/sizeof(u32)>;

    // 1024 fat and data blocks: 2*512 KiB = 1M cache.
    static constexpr size_t cache_size = 1024;

    struct cache_entry_t {
        union {
            fat_block_t fat;
            block_t     data;
        };

        u32 lba = intmax<u32>::value;
        u32 hit = 0;
    };

    struct cache_t {
        Array<cache_entry_t, cache_size> entries;

        u32 last_hit = 0;
    };

    cache_t  fat_cache;
    cache_t data_cache;

    cache_entry_t &get_cache_entry(cache_t &c, u32 lba);

    mutex_t fat_lock;

    // Cached filesystem data.
    u32 block_count   = 0;
    u32 cluster_size  = 0;
    u32 fat_lba       = 0;
    u32 fat_size      = 0;
    u32 data_lba      = 0;
    u32 root_cluster  = 0;
    u32 cluster_count = 0;

    errno_t read_block (u32 lba,       block_t &buffer);
    errno_t write_block(u32 lba, const block_t &buffer);

    errno_t read_data_block (u32 block_i,       block_t &buffer);
    errno_t write_data_block(u32 block_i, const block_t &buffer);

    errno_t read_fat_block (u32 block_i,       fat_block_t &buffer);
    errno_t write_fat_block(u32 block_i, const fat_block_t &buffer);

    errno_t get_next_cluster_n(u32 cluster_n, u32 &next, u32 inc = 1);
    errno_t set_next_cluster_n(u32 cluster_n, u32  next);

    errno_t allocate_cluster(u32 cluster, u32 &next);

    errno_t get_fat_block(u32 block_i, fat_block_t *&block);
    errno_t get_data_block(u32 block_i, block_t *&block);

public:
    const file_name_t &type() const override { return type_; }
    const file_name_t &name() const override { return name_; }

    errno_t get_root_node(inode_t &inode) override;
    ssize_t read_dir(inode_t &inode, u64 &i, dir_entry_t &dest) override;

    // errno_t lookup(inode_t &inode, StringView name, inode_t &dest);
    ssize_t read  (inode_t &inode, u64 offset,       void *buffer, size_t nbytes) override;
    ssize_t write (inode_t &inode, u64 offset, const void *buffer, size_t nbytes) override;
    // errno_t unlink(inode_t &inode, StringView name);
    // errno_t create(inode_t &inode, const file_name_t &name, inode_t &dest);

    // errno_t mkdir (inode_t &inode, StringView name);
    // errno_t rmdir (inode_t &inode, StringView name);
    // errno_t rename(inode_t &src_inode, StringView src_name
    //                       ,inode_t &dst_inode, StringView dst_name);

    static Fat32 *try_mount(DevFs::device_t &dev, StringView devname);

private:
    // Make sure only the 'try_mount' function can construct this class.
    // This ensures that this FS can't be instantiated without a validated FAT boot record.
    Fat32(DevFs::device_t &d
         ,StringView fsname
         ,u32 block_count_
         ,u32 cluster_size_
         ,u32 fat_lba_
         ,u32 fat_size_
         ,u32 data_lba_
         ,u32 root_cluster_);
};
