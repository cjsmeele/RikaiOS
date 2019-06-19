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
#include "elf.hh"
#include "filesystem/vfs.hh"
#include "memory/manager-virtual.hh"
#include "memory/layout.hh"

namespace Elf {

    enum elf_class_t : u8 {
        ELF_32BIT = 1,
        ELF_64BIT = 2,
    };
    enum elf_endianness_t : u8 {
        ELF_LE = 1,
        ELF_BE = 2,
    };

    // ELF32 types.
    using elf32_addr_t   = u32;
    using elf32_half_t   = u16;
    using elf32_off_t    = u32;
    using elf32_word_t   = u32;
    using elf32_sword_t  = s32;

    struct elf_ident_t {
        Array<char,4> magic;
        elf_class_t      elf_class;  ///< 1 = 32-bit, 2 = 64-bit.
        elf_endianness_t endianness; ///< 1 = little-endian, 2 = big-endian.
        u8   version;
        u8   _reserved1[9];
    } __attribute__((packed));

    struct elf32_header_t {
        elf_ident_t  ident;

        elf32_half_t type;
        elf32_half_t machine;
        elf32_word_t version;
        elf32_addr_t entry;
        elf32_off_t  ph_off; ///< Program Header offset.
        elf32_off_t  sh_off; ///< Section Header offset.
        elf32_word_t flags;
        elf32_half_t eh_size;
        elf32_half_t ph_ent_size;
        elf32_half_t ph_num;
        elf32_half_t sh_ent_size;
        elf32_half_t sh_ent_num;
        elf32_half_t sh_str_index;
    } __attribute__((packed));

    enum elf_type_t : u32 {
        ELF_TYPE_NONE = 0, ///< No type.
        ELF_TYPE_REL  = 1, ///< Relocatable object.
        ELF_TYPE_EXEC = 2, ///< Executable.
        ELF_TYPE_DYN  = 3, ///< Shared object.
        ELF_TYPE_CORE = 4, ///< Core file.
    };

    enum elf_ph_entry_type : u8 {
        // (other entry types are not interesting to the ELF loader)
        ELF_PT_LOAD = 1,
    };

    struct elf32_ph_entry_t {
        elf32_word_t type;
        elf32_off_t  offset;    ///< Offset in this file.
        elf32_addr_t v_addr;    ///< Virtual address destination.
        elf32_addr_t p_addr;    ///< Physical address destination.
        elf32_word_t size_file; ///< Segment size within the ELF binary.
        elf32_word_t size_mem;  ///< Segment size in memory.
        elf32_word_t flags;
        elf32_word_t align;
    } __attribute__((packed));

    static bool validate_header(const elf32_header_t &header) {

        // hex_dump(&header, sizeof(header));
        // kprint("header: <{02ux}>\n", header.ident.magic);

        if (header.ident.magic       != Array { '\x7f', 'E', 'L', 'F' }) return false;
        if (header.ident.elf_class   != ELF_32BIT    )                   return false;
        if (header.ident.endianness  != ELF_LE       )                   return false;
        if (header.type              != ELF_TYPE_EXEC)                   return false;
        if (header.ph_num            >  16)                              return false;

        return true;
    }

    errno_t load_elf(StringView path
                    ,const Process::proc_arg_spec_t &args
                    ,Process::proc_t *&proc) {

        proc = nullptr;

        // Open the ELF file.
        fd_t fd = Vfs::open(path, o_read);
        if (fd < 0) return fd;

        // There's a lot of places in this function where things can go wrong
        // and we need to return early.
        // To make sure we clean up the right resources (memory, files, etc.)
        // regardless of whether we return with success or with an error, we
        // register cleanup actions with the ON_RETURN macro (see kernel/include/os-std/misc.hh).
        //
        // C++ guarantees that these cleanup functions are run on scope exit.
        // (in the C world, this is typically achieved with goto statements)
        //
        ON_RETURN(Vfs::close(fd));

        // Read the header.
        elf32_header_t header;
        errno_t err = 0;

        err = Vfs::read(fd, header);
        if (err < 0) return err;

        err = ERR_success;

        if (!validate_header(header)) return ERR_invalid;

        // The ELF header is valid. We can start to load segments into memory.

        // Strategy:
        //
        // We need to create an address space for the new process and read
        // code+data from the ELF file into that address space.
        //
        // We cannot easily do this while the current process (in which we
        // called load_elf) its address space is active: We would need to
        // create many temporary memory mappings within the kernel address
        // space, which may also interfere with other processes' syscalls
        // unless we apply locking.
        //
        // Switching to a new address space and then reading into it is also
        // not desirable: A context switch may happen during a Vfs::read(), and
        // once we are re-scheduled, we are no longer in the new address space.
        //
        // Our approach then is to read each segment in chunks into a temporary
        // buffer, and for each chunk switch to the new address space only
        // temporarily to copy from the buffer to the destination.
        // It's not very efficient, but it's safe and relatively simple to implement.
        //
        // (alternatively, we could load the entire ELF into a global kernel
        // buffer and copy everything in one go, but then we would need to
        // apply locking and add a maximum ELF size limit)

        // Keep track of the address space of the current process.
        Memory::Virtual::PageDir &old_dir = Memory::Virtual::current_dir();

        // Note: This chunk must fit in the per-thread kernel stack.
        static constexpr size_t max_chunk_size = 8_KiB;
        Array<u8, max_chunk_size> chunk;

        // Create an address space for the new process.
        Memory::Virtual::PageDir *pd = Memory::Virtual::make_address_space();
        if (!pd) return ERR_nomem;

        // Make sure we cleanup the address space if we return with an error.
        ON_RETURN({ if (err < 0) {
                        Memory::Virtual::switch_address_space(old_dir);
                        Memory::Virtual::delete_address_space(pd);
                  }});

        // The ELF's program header entries specify what we need to load into memory.
        // Iterate over them.

        for (size_t i : range(header.ph_num)) {
            err = Vfs::seek(fd, seek_set, header.ph_off + i * sizeof(elf32_ph_entry_t));
            if (err < 0) return err;

            // Read it.
            elf32_ph_entry_t entry;
            err = Vfs::read(fd, entry);
            if (err < 0) return err;

            // Ignore entries that do not ask us to load stuff in memory.
            // These may contain debug symbols, for example.
            if (entry.type != ELF_PT_LOAD || !entry.size_mem)
                continue;

            // kprint("PT_LOAD: {08x} offset<{08x}> va<{}> file<{6S}> mem<{6S}>\n"
            //       ,entry.type, entry.offset, (void*)entry.v_addr, entry.size_file, entry.size_mem);

            Memory::region_t mem { entry.v_addr
                                 , align_up(entry.size_mem, page_size) };

            // Validate destination address + size.
            if (!region_valid(mem) || entry.size_file > entry.size_mem) return ERR_invalid;
            if (!region_contains(Memory::Layout::user(), mem))          return ERR_invalid;

            // Entries have a file and a memory size. The file size may be
            // smaller than the memory size: In this case, the remaining memory
            // size is zeroed. The .bss section typically has zero file size, for example.

            size_t file_bytes_copied = 0;

            if (entry.size_file) {
                err = Vfs::seek(fd, seek_set, entry.offset);
                if (err < 0) return err;
            }

            // Map the segment.
            {
                // Temporarily switch to the new address space.
                Memory::Virtual::switch_address_space(*pd);

                err = Memory::Virtual::map(mem.start
                                          ,0
                                          ,mem.size
                                          ,Memory::Virtual::flag_writable
                                          |Memory::Virtual::flag_user);
                if (err < 0) return err;

                // Switch back.
                Memory::Virtual::switch_address_space(old_dir);
            }

            // Read and copy the segment in chunks.
            while (file_bytes_copied < entry.size_file) {

                size_t to_copy = min(entry.size_file - file_bytes_copied
                                    ,chunk.size());

                err = Vfs::read_all(fd, chunk.data(), to_copy);
                if (err < 0) return err;

                // Temporarily switch to the new address space.
                {
                    Memory::Virtual::switch_address_space(*pd);

                    memcpy((u8*)mem.start + file_bytes_copied
                          ,chunk.data()
                          ,to_copy);

                    Memory::Virtual::switch_address_space(old_dir);
                }

                file_bytes_copied += to_copy;
            }

            // Zero the remaining memory portion, if any.
            if (entry.size_mem > entry.size_file) {
                Memory::Virtual::switch_address_space(*pd);

                memset((u8*)mem.start + entry.size_file
                      ,0
                      ,mem.size - entry.size_file);

                Memory::Virtual::switch_address_space(old_dir);
            }
        }

        // Copy over process arguments.
        {
            Memory::Virtual::switch_address_space(*pd);

            err = Memory::Virtual::map(Memory::Layout::user_args().start
                                      ,0
                                      ,max((size_t)16_K
                                          ,(size_t)(sizeof(int)
                                                   + max_args*sizeof(char*)
                                                   + max_args_chars))
                                      ,Memory::Virtual::flag_writable
                                      |Memory::Virtual::flag_user);
            if (err < 0) return err;

            // The destination values.
            int   &argc = *(int*)Memory::Layout::user_args().start;
            char **argv = (char**)(&argc + 1);

            char  *p    = (char*)(argv + max_args);
            for (StringView a : args) {
                if (!a.length()) break;
                argv[argc++] = p;
                memcpy(p, a.data(), a.length());
                p   += a.length();
                *p++ = 0;
            }

            Memory::Virtual::switch_address_space(old_dir);
        }

        // Memory::Virtual::swtch_address_space(*pd);
        // hex_dump((char*)0x40100000, 8_K);
        // Memory::Virtual::switch_address_space(old_dir);

        // Note: The process name (`path` here) may be longer than the max process name.
        // It is automatically truncated.
        proc = Process::make_proc(pd
                                 ,(function_ptr<void()>)header.entry
                                 ,path);

        return ERR_success;
    }
}
