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
#include "fat32.hh"
#include "vfs.hh"

// This filesystem implementation is currently read-only.

Fat32::cache_entry_t &Fat32::get_cache_entry(Fat32::cache_t &c, u32 lba) {

    // kprint("{} CACHE {}?\n", &c == &fat_cache ? "FAT" : "DATA", lba);
    size_t lru_i = 0;
    for (auto [i, e] : enumerate(c.entries)) {
        if (e.hit < c.entries[lru_i].hit)
            lru_i = i;

        if (e.lba == lba) {
            e.hit = ++c.last_hit;
            // kprint("CACHE HIT {}\n", i);
            return e;
        }
    }
    // kprint("CACHE NO HIT {}\n", lru_i);

    c.entries[lru_i].hit = ++c.last_hit;
    return c.entries[lru_i];
}

errno_t Fat32::read_block (u32 lba, Array<u8, block_size> &buffer) {
    if (lba < block_count)
         // (throw away 'bytes read' information. will become 0 on success)
         return min(0, dev.read(lba*block_size, buffer.data(), block_size));
    else return ERR_io;
}

// XXX: Writes do not respect the cache currently.
errno_t Fat32::write_block(u32 lba, const Array<u8, block_size> &buffer) {
    if (lba < block_count)
         // (throw away 'bytes written' information. will become 0 on success)
         return min(0, dev.write(lba*block_size, buffer.data(), block_size));
    else return ERR_io;
}


errno_t Fat32::read_data_block (u32 block_i, block_t &buffer) {
    return read_block(data_lba + block_i, buffer);
}
errno_t Fat32::write_data_block(u32 block_i, const block_t &buffer) {
    return write_block(data_lba + block_i, buffer);
}

errno_t Fat32::read_fat_block (u32 block_i, fat_block_t &buffer) {
    return read_block(fat_lba + block_i, *(block_t*)&buffer); // eww.
}
errno_t Fat32::write_fat_block(u32 block_i, const fat_block_t &buffer) {
    return write_block(fat_lba + block_i, *(block_t*)&buffer); // eww.
}

errno_t Fat32::get_fat_block(u32 block_i, fat_block_t *&block) {

    u32 lba = fat_lba + block_i;
    cache_entry_t &e = get_cache_entry(fat_cache, lba);

    block = &e.fat;

    if (e.lba != lba) {
        e.lba = intmax<u32>::value;
        errno_t err = read_fat_block(block_i, e.fat);
        if (err) return err;
        e.lba = lba;
    }

    return ERR_success;
}

errno_t Fat32::get_data_block(u32 block_i, block_t *&block) {

    u32 lba = data_lba + block_i;
    cache_entry_t &e = get_cache_entry(data_cache, lba);

    block = &e.data;

    if (e.lba != lba) {
        e.lba = intmax<u32>::value;
        errno_t err = read_data_block(block_i, e.data);
        if (err) return err;
        e.lba = lba;
    }

    return ERR_success;
}

static constexpr bool is_eoc(u32 cluster_n) {
    return cluster_n < 2 || cluster_n >= 0x0fff'fff0;
}

errno_t Fat32::get_next_cluster_n(u32 cluster_n, u32 &next, u32 inc) {

    locked_within_scope _(fat_lock);

    next = cluster_n;

    for (u32 i : range(inc)) {
        if (is_eoc(next))
            return ERR_not_exists;

        fat_block_t *fat = nullptr;
        errno_t err = get_fat_block(next / fat->size(), fat);
        if (err < 0) return err;

        next = (*fat)[next % fat->size()];
    }
    return ERR_success;
}

errno_t Fat32::set_next_cluster_n(u32 cluster_n, u32 next) {

    locked_within_scope _(fat_lock);

    fat_block_t *fat = nullptr;
    errno_t err = get_fat_block(cluster_n / fat->size(), fat);
    if (err < 0) return err;

    (*fat)[cluster_n % fat->size()] = next;
    return write_fat_block(cluster_n / fat->size(), *fat);
}

errno_t Fat32::allocate_cluster(u32 cluster, u32 &next) {

    (void)cluster;
    (void)next;

    // (unimplemented)

    return ERR_nospace;
}

// FAT data structures {{{

/**
 * \brief Layout of the first sector in a FAT volume.
 */
struct boot_record_t {

    Array<u8,  3>  _jump;
    Array<char,8> oem_name;

    /**
     * \brief FAT Bios Parameter Block.
     */
    struct bpb_t {
        u16 block_size;   ///< In bytes.
        u8  cluster_size; ///< In blocks.
        u16 reserved_blocks;
        u8  fat_count;
        u16 root_dir_entry_count;
        u16 block_count_short; // Use the 32-bit blockCount field if zero.
        u8  media_descriptor;
        u16 fat_size_short;    ///< In blocks. Use the 32-bit EBPB fatSize field if zero.
        u16 sectors_per_track; // CHS stuff, do not use.
        u16 head_count;        // .
        u32 hidden_blocks;
        u32 block_count;
    } __attribute__((packed)) bpb;

    /**
     * \brief FAT32 EBPB.
     */
    struct ebpb_t {
        u32 fat_size; ///< In blocks.
        u16 flags1;
        u16 version;
        u32 root_cluster;
        u16 fs_info_block;
        u16 fat_copy_block; ///< 3 blocks.
        u8  _reserved1[12];
        u8  drive_number;
        u8  _reserved2;
        u8  extended_boot_signature; ///< 0x29 if the following fields are valid, 0x28 otherwise.
        u32 volume_id;

        Array<char,11> volume_label;
        Array<char, 8> fs_type;

        Array<u8,420> _boot_code;

    } __attribute__((packed)) ebpb;

    u16 signature; // 0xaa55.

} __attribute__((packed));

// This should be exactly one sector.
static_assert(sizeof(boot_record_t) == 512);

/**
 * \brief FAT directory entry.
 */
struct fat_dir_entry_t {
    Array<char,8> name;      ///< Padded with spaces.
    Array<char,3> extension; ///< Padded with spaces.

    // Attribute byte 1.
    bool attr_read_only    : 1;
    bool attr_hidden       : 1;
    bool attr_system       : 1;
    bool attr_volume_label : 1;
    bool attr_directory    : 1;
    bool attr_archive      : 1;
    bool attr_disk         : 1;
    bool _attr_reserved    : 1;

    u8  attributes2; ///< This byte has lots of different interpretations and we can safely ignore it.

    u8  create_time_hi_res;
    u16 create_time;
    u16 create_date;
    u16 access_date;
    u16 cluster_no_high;
    u16 modify_time;
    u16 modify_date;
    u16 cluster_no_low;
    u32 file_size; ///< In bytes.

} __attribute__((packed));

// }}}

/// Replace trailing spaces in name fields with NUL bytes.
template<size_t N>
static void trim_name(Array<char, N> &str) {
    for (ssize_t i : range((ssize_t)str.size()-1, -1, -1)) {
             if (str[i] == ' ') str[i] = '\0';
        else if (str[i])        break;
    }
}

static u64 &inode_parent_cluster(inode_t &inode) {
    return inode.context1;
}
static u64 &inode_parent_dirent_i(inode_t &inode) {
    return inode.context2;
}

errno_t Fat32::get_root_node(inode_t &inode) {
    inode      = inode_t{};
    inode.i    = root_cluster;
    inode.type = t_dir;
    inode.perm = 0700;
    inode.size = 0;
    inode.fs   = this;

    inode_parent_cluster(inode)  = 0;
    inode_parent_dirent_i(inode) = 0;

    return ERR_success;
}

ssize_t Fat32::read_dir(inode_t &inode, u64 &i, dir_entry_t &dest) {

    bool found = false;
    errno_t err;

    // Iterate over clusters.
    while (true) {
        u32 dir_cluster_i = i * sizeof(fat_dir_entry_t) / block_size / cluster_size;
        u32 cluster = inode.i;

        if (dir_cluster_i != 0) {
            err = get_next_cluster_n(inode.i, cluster, dir_cluster_i);
            // kprint("cluster next of {08x} -> {} ({08x})\n"
            //       ,inode.i, error_name(err), cluster);
            if (is_eoc(cluster))       return 0;
            if (err == ERR_not_exists) return 0;
            if (err)                   return err;
        }

        // Iterate over blocks.

        for (u32 block_i = i * sizeof(fat_dir_entry_t) / block_size % cluster_size
            ;block_i < cluster_size
            ;++block_i) {

            block_t *block;
            err = get_data_block((cluster-2) * cluster_size + block_i, block);
            // kprint("block {} -> {}\n", cluster * cluster_size + block_i, error_name(err));
            if (err) return err;

            // hex_dump(block->data(), 512);

            const auto &dirents
                = *(Array<fat_dir_entry_t, block_size / sizeof(fat_dir_entry_t)>*) block;

            // Iterate over dirents.
            for (u32 entry_i : range(i % (block_size / sizeof(fat_dir_entry_t)), dirents.size())) {

                const fat_dir_entry_t &entry = dirents[entry_i];

                if (!entry.name[0]) {
                    // A name field starting with a NUL byte indicates directory EOF.
                    return 0;
                }

                if (!entry.attr_disk
                 && !entry.attr_volume_label
                 && (u8)entry.name[0] != 0xe5) { // Indicates deleted file.

                    dest = {};

                    for (char c : entry.name)
                        dest.name += c;
                    dest.name.rtrim();

                    if (entry.extension[0] != ' ') {
                        dest.name += '.';
                        for (char c : entry.extension)
                            dest.name += c;
                    }
                    dest.name.rtrim();
                    dest.name.to_lower();

                    // kprint("have: {} {} <{}>\n", i, entry_i, dest.name);

                    if (dest.name.length()
                     && dest.name[0] != '.'
                     && dest.name.char_pos('/') == -1) {

                        dest.inode.fs   = this;
                        dest.inode.i    = (u32)entry.cluster_no_high << 16
                                        |      entry.cluster_no_low;
                        dest.inode.type = entry.attr_directory ? t_dir : t_regular;
                        dest.inode.perm = 0700;
                        dest.inode.size = entry.file_size;

                        inode_parent_cluster(dest.inode)  = inode.i;
                        inode_parent_dirent_i(dest.inode) = i;

                        ++i;
                        // kprint("OK????? i -> {}\n", i);

                        return 1;
                    }
                }

                ++i;
            }
        }
    }

    return 0;
}

ssize_t Fat32::read(inode_t &inode, u64 offset, void *buffer, size_t nbytes) {

    if (offset >= inode.size)
        return 0;

    errno_t err;

    // Iterate over clusters.

    u32 cluster_i      = inode.i;

    size_t bytes_read = 0;
    while (bytes_read < nbytes) {

        if (offset + bytes_read >= inode.size)
            return bytes_read;

        u32 file_cluster_i = (offset + bytes_read) / block_size / cluster_size;

        if (file_cluster_i != 0) {
            err = get_next_cluster_n(inode.i, cluster_i, file_cluster_i);
            // kprint("cluster next of {08x} -> {} ({08x})\n"
            //       ,inode.i, error_name(err), cluster);
            if (is_eoc(cluster_i)) return 0;
            if (err) return err;
        }

        // Iterate over blocks.

        for (u32 block_i = (offset + bytes_read) / block_size % cluster_size
            ;block_i < cluster_size
            ;++block_i) {

            // kprint("READ BLOCK {}:{} OF FILE (INODE {}) CSZ {}\n"
            //       , cluster_i, block_i, inode.i, cluster_size);

            block_t *block;
            err = get_data_block((cluster_i-2) * cluster_size + block_i, block);
            // kprint("block {} -> {}\n", cluster_i * cluster_size + block_i, error_name(err));
            if (err) return err;

            size_t to_copy = min(u32(min(u64(nbytes) - bytes_read
                                    ,inode.size - (offset + bytes_read)))
                                ,block_size - u32((offset + bytes_read) % block_size));

            memcpy(((u8*)buffer)+bytes_read
                  ,block->data() + (offset+bytes_read) % block_size
                  ,to_copy);

            bytes_read += to_copy;
        }
    }

    return bytes_read;
}

ssize_t Fat32::write(inode_t &inode, u64 offset, const void *buffer, size_t nbytes) {

    if (offset >= inode.size)
        return ERR_nospace;

    errno_t err;

    // Iterate over clusters.

    size_t bytes_written = 0;
    while (bytes_written < nbytes) {

        if (offset + bytes_written >= inode.size)
            return ERR_nospace; // XXX
            // return bytes_written;

        u32 file_cluster_i = offset / block_size / cluster_size;
        u32 cluster_i      = inode.i;

        if (file_cluster_i != 0) {
            err = get_next_cluster_n(inode.i, cluster_i, file_cluster_i);
            // kprint("cluster next of {08x} -> {} ({08x})\n"
            //       ,inode.i, error_name(err), cluster);
            if (is_eoc(cluster_i)) {
                // Allocate.
                return ERR_nospace; // XXX
            }
            if (err) return err;
        }

        // Iterate over blocks.

        for (u32 block_i = (offset + bytes_written) / block_size % cluster_size
            ;block_i < cluster_size
            ;++block_i) {

            block_t *block;
            err = get_data_block((cluster_i-2) * cluster_size + block_i, block);
            // kprint("block {} -> {}\n", cluster_i * cluster_size + block_i, error_name(err));
            if (err) return err;

            size_t to_copy = min(u32(min(u64(nbytes) - bytes_written
                                    ,inode.size - (offset + bytes_written)))
                                ,block_size - u32((offset + bytes_written) % block_size));

            memcpy(block->data() + (offset+bytes_written) % block_size
                  ,((u8*)buffer)+bytes_written
                  ,to_copy);

            err = write_data_block((cluster_i-2) * cluster_size + block_i, *block);

            bytes_written += to_copy;
        }
    }

    return bytes_written;
}
// errno_t unlink(inode_t &inode, StringView name);
// errno_t create(inode_t &inode, const file_name_t &name, inode_t &dest);

// errno_t mkdir (inode_t &inode, StringView name);
// errno_t rmdir (inode_t &inode, StringView name);
// errno_t rename(inode_t &src_inode, StringView src_name
//                       ,inode_t &dst_inode, StringView dst_name);

Fat32 *Fat32::try_mount(DevFs::device_t &dev, StringView devname) {

    Array<u8, 512> buffer;

    ssize_t n = dev.read(0, buffer.data(), block_size);
    if (n != block_size) return nullptr;

    const boot_record_t &br = *(boot_record_t*)buffer.data();

    // Sanity-check the boot record - make sure that this is actually a FAT32 partition. {{{

    if (br.signature != 0xaa55) return nullptr;

    // We do not currently support block sizes other than 512.
    if (br.bpb.block_size != block_size) return nullptr;

    // Check against partition bounds.
    if (br.bpb.block_count > dev.size()/block_size) return nullptr;

    // These must all be at least 1.
    if (!br.bpb.reserved_blocks) return nullptr; // (for the boot record)
    if (!br.bpb.cluster_size)    return nullptr;
    if (!br.bpb.block_count)     return nullptr;
    if (!br.bpb.fat_count)       return nullptr;

    // These would indicate that the FS is FAT12 or FAT16 instead of FAT32.
    if (br.bpb.root_dir_entry_count) return nullptr;
    if (br.bpb.fat_size_short)       return nullptr;

    if (br.ebpb.fat_size > br.bpb.block_count) return nullptr;

    u32 fat_lba  = br.bpb.reserved_blocks;
    u32 fat_size = br.ebpb.fat_size;
    u32 data_lba = fat_lba + fat_size * br.bpb.fat_count; // FIXME: may overflow.

    if (data_lba >= br.bpb.block_count) return nullptr;

    u32 cluster_size  = br.bpb.cluster_size;
    u32 data_size     = br.bpb.block_count - data_lba;
    u32 data_clusters = data_size / cluster_size;

    if (!data_clusters) return nullptr;
    if (!br.ebpb.root_cluster
      || br.ebpb.root_cluster >= data_clusters) return nullptr;

    // }}}

    // Everything seems to be in order!

    Fat32 *fs = new Fat32(dev
                         ,devname
                         ,br.bpb.block_count
                         ,cluster_size
                         ,fat_lba
                         ,fat_size
                         ,data_lba
                         ,br.ebpb.root_cluster);

    if (!fs) return nullptr;

    // kprint("fat volume name: {}\n", StringView(br.ebpb.volume_label.data()
    //                                           ,br.ebpb.volume_label.size()));

    if (Vfs::mount(*fs, devname) == ERR_success)
         return fs;
    else return nullptr;

    return nullptr;
}

Fat32::Fat32(DevFs::device_t &d
            ,StringView fsname
            ,u32 block_count_
            ,u32 cluster_size_
            ,u32 fat_lba_
            ,u32 fat_size_
            ,u32 data_lba_
            ,u32 root_cluster_)
    : name_      (fsname)
    , dev        (d)
    ,block_count (block_count_)
    ,cluster_size(cluster_size_)
    ,fat_lba     (fat_lba_)
    ,fat_size    (fat_size_)
    ,data_lba    (data_lba_)
    ,root_cluster(root_cluster_)
    ,cluster_count(fat_size / 4)
{ }
