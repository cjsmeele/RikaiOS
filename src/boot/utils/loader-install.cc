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
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

// loader-install is responsible for installing a bootloader + kernel on a
// given disk, such that when the system is booted, the given kernel will be
// run.
//
// Prerequisites:
//
// - A bootsector (bootsect.asm) has been assembled into bootsect.bin
// - Stage2 has been compiled into stage2.bin
// - A kernel has been compiled into kernel.bin
//
// An example disk layout, after running loader-install:
//
//  Section               Block/Sector no.
// ┌────────────────────┐ 0
// │ Boot sector        │
// ├────────────────────┤ 1
// │ (unused space)     │
// ├────────────────────┤ 2048  (@ 1 MiB)  ─╮
// │ Stage 2 binary     │                   │
// ├────────────────────┤ 2048+n            ├─ Boot partition
// │ Kernel binary      │                   │
// ├────────────────────┤ 18432 (@ 9 MiB)  ─╯
// │                    │                  ─╮
// │                    │                   ├─ Other partitions
// └────────────────────┘                  ─╯


/// Partition id / FS type of the reserved space at the beginning of the disk
/// where we write bootloader information.
constexpr uint8_t reserved_fs_type = 0x7f;

/// The size of a block in bytes (the same as the size of a disk sector).
constexpr size_t  block_size       = 512;

// Byte positions in the bootsector where we will write information to tell the
// bootsector code how to load stage2 and the kernel.
constexpr size_t mbr_offset_stage2_start    =  5;
constexpr size_t mbr_offset_stage2_blocks   =  9;
constexpr size_t mbr_offset_kernel_start    = 10;
constexpr size_t mbr_offset_kernel_blocks   = 14;

/// Partition information structure, as contained in the boot sector of a
/// MBR-partitioned disk.
struct partition_entry {
    char     ignored1_[4]; ///< legacy stuff.
    uint8_t  partition_id; ///< indicates file system type.
    char     ignored2_[3]; ///< legacy stuff.
    uint32_t lba_start;    ///< start sector number.
    uint32_t block_count;  ///< sector count (assuming block size = 512 bytes).
} __attribute__((packed));

/// Read a file into a vector.
std::vector<uint8_t> slurp(std::fstream &fh, ssize_t max_size = -1) {
    fh.seekg(0, fh.end);
    size_t size = fh.tellg();
    fh.seekg(0);

    std::vector<uint8_t> vec;

    if (max_size >= 0 && size > size_t(max_size))
        size = max_size;

    vec.resize(size);
    fh.read((char*)vec.data(), size);

    return vec;
}

int main(int argc, const char **argv) {

    if (argc != 5) {
        std::cerr << "usage: " << argv[0] << " <disk> <mbr_file> <stage2_file> <kernel_file>\n";
        return 0;
    }

    const char *path_disk   = argv[1];
    const char *path_mbr    = argv[2];
    const char *path_stage2 = argv[3];
    const char *path_kernel = argv[4];

    // The program aborts automatically when I/O errors occur.

    std::fstream    file_mbr;    file_mbr.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    std::fstream file_stage2; file_stage2.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    std::fstream file_kernel; file_kernel.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    std::fstream   file_disk;   file_disk.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    // Open files.

    try {    file_mbr.open(path_mbr,       file_mbr.in |    file_mbr.binary);                 } catch (...) { std::cerr << argv[0] << ": could not open mbr\n";    return 1; }
    try { file_stage2.open(path_stage2, file_stage2.in | file_stage2.binary);                 } catch (...) { std::cerr << argv[0] << ": could not open stage2\n"; return 1; }
    try { file_kernel.open(path_kernel, file_kernel.in | file_kernel.binary);                 } catch (...) { std::cerr << argv[0] << ": could not open kernel\n"; return 1; }
    try {   file_disk.open(path_disk,     file_disk.in |   file_disk.binary | file_disk.out); } catch (...) { std::cerr << argv[0] << ": could not open disk\n";   return 1; }

    // Get file sizes.

       file_mbr.seekg(0,    file_mbr.end); size_t    mbr_size =    file_mbr.tellg();    file_mbr.seekg(0);
    file_stage2.seekg(0, file_stage2.end); size_t stage2_size = file_stage2.tellg(); file_stage2.seekg(0);
    file_kernel.seekg(0, file_kernel.end); size_t kernel_size = file_kernel.tellg(); file_kernel.seekg(0);
      file_disk.seekp(0,   file_disk.end); size_t   disk_size =   file_disk.tellp();   file_disk.seekp(0);

    // Some sanity checks.

    if (disk_size < 16 * 1024*1024)   { std::cerr << argv[0] << ": disk too small (should be at least 16M)\n"; return 1; }
    if (mbr_size != 512)              { std::cerr << argv[0] << ": mbr is not 512 bytes long\n";               return 1; }
    if (stage2_size > 0x10000-0x7e00) { std::cerr << argv[0] << ": stage2 too big (must be max 32K)\n";        return 1; }

    // Read disk's MBR.
    auto disk_mbr        = slurp(file_disk, 512);
    auto partition_table = (partition_entry*)(disk_mbr.data() + 0x1be);

    // Find the boot partition.
    // The boot partition is identified by a partition id (filesystem type) of 0x7f.
    // Additionally, the boot partition must be a primary partition.
    const auto *boot_partition = &partition_table[0];
    for (int i = 0
        ;i < 4 && boot_partition->partition_id != 0x7f
        ;boot_partition = &partition_table[++i]);

    // Did we find the boot partition?
    if (boot_partition->partition_id != reserved_fs_type) {
        std::cerr << argv[0] << ": disk does not have a boot partition (partition id 0x7f)\n";
        return 1;
    }

    // Make sure stage2 + kernel fit in the boot partition.
    if (   boot_partition->block_count < stage2_size / block_size + 1
        || boot_partition->block_count < kernel_size / block_size + 1
        || boot_partition->block_count < kernel_size / block_size + 2
                                       + stage2_size / block_size + 2) {

        std::cerr << argv[0] << ": boot partition is too small to fit both stage2 and the kernel\n";
        return 1;
    }

    auto mbr    = slurp(file_mbr);
    auto stage2 = slurp(file_stage2);
    auto kernel = slurp(file_kernel);

    // -----------------------------------------------------------------
    // Sanity-check the bootsector.

    if (   (mbr[510] != 0x55)
        || (mbr[511] != 0xaa)) {
        std::cerr << argv[0] << ": MBR does not have a '55aa' signature\n";
        return 1;
    }

    // Make sure the MBR does not contain code where the partition table is supposed to go.
    for (size_t i = 0x1be; i <= 0x1fd; ++i) {
        if (mbr[i] != 0) {
            std::cerr << argv[0] << ": MBR contains non-null bytes after byte 0x1bd (is the bootsector code too large?)\n";
            return 1;
        }
    }

    // -----------------------------------------------------------------
    // Install stage2.

    size_t stage2_start = boot_partition->lba_start * block_size;
    file_disk.seekp(stage2_start); // Go to the first sector of the boot partition.
    file_disk.write((char*)stage2.data(), stage2.size());

    // -----------------------------------------------------------------
    // Install the kernel.
    // Make sure we start the kernel on a sector-aligned address.

    size_t kernel_start = size_t(file_disk.tellp()) + block_size;
    kernel_start = kernel_start - (kernel_start % block_size);
    file_disk.seekp(kernel_start);

    file_disk.write((char*)kernel.data(), kernel.size());

    // -----------------------------------------------------------------
    // Install the bootsector.

    file_disk.seekp(0);
    for (size_t i = 0; i < 0x1bd; ++i) {

        // Helper function for writing a little-endion 32-bit number at the
        // current disk position.
        auto write_u32le = [&](uint32_t v) {
            file_disk.put((uint8_t)(v      ));
            file_disk.put((uint8_t)(v >>  8)); i++;
            file_disk.put((uint8_t)(v >> 16)); i++;
            file_disk.put((uint8_t)(v >> 24)); i++;
        };

        // Overwrite certain MBR positions with stage2/kernel information.
        if (i == mbr_offset_stage2_blocks) {
            // Stage2 block count must be <= 64 (<= 32KiB), so we only need one byte here.
            file_disk.put(((uint8_t)(stage2.size() / block_size + 1)));
        } else if (i == mbr_offset_stage2_start)  { write_u32le(stage2_start  / block_size);
        } else if (i == mbr_offset_kernel_start)  { write_u32le(kernel_start  / block_size);
        } else if (i == mbr_offset_kernel_blocks) { write_u32le(kernel.size() / block_size + 1);
        } else {
            file_disk.put(mbr[i]);
        }
    }

    // Write bootsector signature.
    file_disk.seekp(0x1fe);
    file_disk.put((char)0x55);
    file_disk.put((char)0xaa);

    return 0;
}
