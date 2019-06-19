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
#include "kshell.hh"

#include "process/proc.hh"
#include "memory/manager-physical.hh"
#include "memory/kernel-heap.hh"
#include "driver/disk/ata.hh"
#include "driver/vga.hh"
#include "ipc/semaphore.hh"
#include "filesystem/vfs.hh"
#include "process/elf.hh"

/**
 * A built-in kernel shell for debugging purposes.
 */
namespace Kshell {

    /**
     * The input queue.
     *
     * When kshell is enabled, all keyboard and uart input is redirected into
     * this queue.
     * kshell input completely bypasses the virtual filesystem (we do not call
     * read() for input). This means kshell may be usable when other parts of
     * the OS refuse to work.
     */
    KQueue<char, 64> input;

    static bool enabled_       = false;
    static bool input_enabled_ = false;

    /// A lock used to enable (unblock) kshell.
    semaphore_t lock {0, {}};

    /// (to be called from an isr or another thread)
    void enable() {
        if (!enabled_) {
            Process::pause_userspace();
            enabled_ = true;
            signal(lock);
        }
    }

    /// (to be called from the kshell thread)
    static void disable() {
        if (enabled_) {
            enabled_ = false;
            Process::resume_userspace();
            wait(lock);
        }
    }

    bool enabled()        { return enabled_;       }
    bool want_all_input() { return input_enabled_; }

    static constexpr size_t max_cmdline = 256;

    using cmdline_t = String<max_cmdline>;

    /// Does not contain the arguments itself.
    /// Instead, contains indices (StringViews) into a command line string.
    struct args_t {
        using argv_t = Array<StringView, max_args>;
        argv_t argv;
        size_t argc;
    };

    /// Split a commandline string into separate argument strings.
    /// The original string is modified in place: The return value contains
    /// StringViews that point into the cmdline string.
    static args_t split_args(cmdline_t &cmdline) {

        args_t::argv_t argv;
        size_t argc = 0;

        bool escaped = false;

        size_t escapes = 0;

        for (size_t i : range(cmdline.length())) {
redo:
            char &src = cmdline[i + escapes];
            char &dst = cmdline[i];

            if (i + escapes >= cmdline.length()) {
                dst = 0;
                // cmdline[i+1] = 0;
                break;
            }

            if (escaped) {
                if (!argv[argc].length_)
                    argv[argc].data_ = &dst;

                // Allow for simple escape codes.
                     if (src == 'n') dst = '\n';
                else if (src == 'r') dst = '\r';
                else if (src == 'e') dst = '\x1b';
                else if (src == 'f') dst = '\f';
                else                 dst = src;
                argv[argc].length_++;
                escaped = false;
            } else {
                if (src == ' ') {
                    dst = 0;
                    if (argv[argc].length_)
                        argc++;
                } else {
                    if (src == '\\') {
                        dst = 0;
                        escaped = true;
                        escapes++;
                        goto redo;
                    } else {
                        if (!argv[argc].length_)
                            argv[argc].data_ = &dst;
                        dst = src;
                        argv[argc].length_++;
                    }
                }
            }
        }
        if (argv[argc].length_) {
            // argv[argc].data_[argv[argc].length_] = 0;
            argc++;
        }

        return { argv, argc };
    }

    /// Print a recursive directory listing.
    static void tree(StringView dir_name, int depth = 0) {

        fd_t fd = Vfs::open(dir_name, o_dir);
        if (fd < 0) {
            kprint("could not open dir <{}>: {}\n", dir_name, error_name(fd));
            return;
        }

        if (depth == 0) {
            path_t path = Vfs::canonicalise_path(dir_name);
            if (path != "/") path += '/';
            kprint("{}\n", path);
            ++depth;
        }

        dir_entry_t entry;
        while (true) {
            int n = Vfs::read_dir(fd, entry);
            if (n > 0) {
                for (int i : range(depth*4))
                    kprint(" ");
                if (entry.inode.type == t_dir) {
                    kprint("{}/\n", entry.name);
                    path_t path = dir_name;
                    path += '/';
                    path += entry.name;
                    tree(path, depth+1);
                } else {
                    kprint("{}\n", entry.name);
                }
            } else {
                if (n != 0)
                    kprint("read_dir error: {}\n", error_name(n));
                break;
            }
        }

        Vfs::close(fd);
    }

    static void run_command(const args_t &args) {

        // Pause the kinput buffer so that input can again be read from
        // /dev/keyboard and /dev/uart0.
        input_enabled_ = false;

        if (!args.argc)
            return;

        size_t argc  = args.argc;
        auto  &argv  = args.argv;
        StringView s = argv[0];

               if (s == "bmap") {
            Memory::Physical::dump_bitmap();
        } else if (s == "brick") {
            alignas(512) Array<u8,512> buffer;
            errno_t err = Driver::Disk::Ata::read(0, 0, buffer.data(), 1);
            if (err) {
                kprint("error: {}\n", error_name(err));
            } else {
                // Remove boot signature.
                buffer[510] = 0;
                buffer[511] = 0;
                err = Driver::Disk::Ata::write(0, 0, buffer.data(), 1);
                if (err) {
                    kprint("error: {}\n", error_name(err));
                }
            }
        } else if (s == "cat" || s == "xd") {
            if (argc >= 2) {
                for (size_t i : range(1, argc)) {
                    auto path = argv[i];

                    fd_t fd = Vfs::open(path, o_read);

                    if (fd < 0) {
                        kprint("open failed: {}\n", error_name(fd));
                        return;
                    }

                    size_t total = 0;

                    alignas(512) Array<char, 512> buffer;
                    while (true) {
                        int n = Vfs::read(fd, buffer.data(), buffer.size());
                        if (n > 0) {
                            if (s == "cat") {
                                for (size_t j : range(n))
                                    kprint("{}", buffer[j]);
                            } else {
                                kprint("file offset {08x}\n", total);
                                hex_dump(buffer.data(), n);
                            }
                            total += n;
                        } else {
                            if (n != 0)
                                kprint("read failed: {}\n", error_name(n));
                            break;
                        }
                    }

                    Vfs::close(fd);
                }
            } else {
                kprint("usage: cat <path>\n");
            }
        } else if (s == "cd") {
            if (argc == 2) {
                fd_t fd = Vfs::open(argv[1], o_dir);
                if (fd < 0) {
                    kprint("open error: {}\n", error_name(fd));
                    return;
                }
                Process::current_proc()->working_directory = Vfs::canonicalise_path(argv[1]);
                Vfs::close(fd);
            } else {
                kprint("usage: cd <path>\n");
            }
        } else if (s == "clear") {
            kprint("\n\x1b[2J\x1b[1;1H");
        } else if (s == "close") {
            fd_t fd;
            if (argc == 2 && string_to_num(argv[1], fd)) {
                errno_t err = Vfs::close(fd);
                if (err < 0)
                    kprint("close failed: {}\n", error_name(err));
            } else {
                kprint("usage: close <fd>\n");
            }
        } else if (s == "cp") {
            if (argc != 3) {
                kprint("usage: cp <src> <dst>\n");
                return;
            }

            fd_t src = Vfs::open(argv[1], o_read);
            if (src < 0) {
                kprint("could not open src for reading: {}\n", error_name(src));
                return;
            }
            fd_t dst = Vfs::open(argv[2], o_create | o_write);
            if (dst < 0) {
                Vfs::close(src);
                kprint("could not open dst for writing: {}\n", error_name(dst));
                return;
            }

            alignas(512) Array<char, 512> buffer;
            while (true) {
                int n = Vfs::read(src, buffer);
                if (n <= 0) {
                    if (n != 0)
                        kprint("read failed: {}\n", error_name(n));
                    break;
                }
                n = Vfs::write(dst, buffer.data(), n);
                if (n < 0) {
                    kprint("write failed: {}\n", error_name(n));
                    break;
                }
            }
            Vfs::close(dst);
            Vfs::close(src);

        } else if (s == "die") {
            crash();
        } else if (s == "echo") {
            for (auto i : range(1, argc))
                kprint("{} ", argv[i]);
            kprint("\n");
        } else if (s == "exit") {
            kprint("** disabled kshell **\n");
            disable();
            kprint("\n** enabled kshell **\n");

        } else if (s == "halt") {
            // Try to initiate a shutdown by banging known emulator ports.
            // (this may cause hardware to catch fire on real machines)
            //
            // Correctly implementing shutdown in a cross-platform manner would
            // require us to implement APM or ACPI support, which is beyond our scope.

            kprint("trying to initiate shutdown...\n");

            // source: https://wiki.osdev.org/Shutdown#Emulator-specific_methods

            Io::out_16(0xB004, 0x2000);
            Io::out_16(0x0604, 0x2000);
            Io::out_16(0x4004, 0x3400);

            Io::wait(500_K);

            kprint("\nso apparently that failed!\n");
            kprint("freezing the cpu instead...\nzzz");

            asm_hang();

        } else if (s == "head" || s == "headx") {
            u32 nbytes;
            if (argc == 3 && string_to_num(argv[1], nbytes)) {
                fd_t fd = Vfs::open(argv[2], o_read);

                if (fd < 0) {
                    kprint("open failed: {}\n", error_name(fd));
                    return;
                }

                alignas(512) Array<char, 512> buffer;
                size_t bytes_read = 0;
                while (bytes_read < nbytes) {
                    int n = Vfs::read(fd, buffer.data(), min(buffer.size(), nbytes - bytes_read));
                    if (n >= 0) {
                        if (s == "head") {
                            for (size_t j : range(n))
                                kprint("{}", buffer[j]);
                        } else {
                            hex_dump(buffer.data(), n);
                        }
                        bytes_read += n;
                    } else {
                        if (n != 0)
                            kprint("read failed: {}\n", error_name(n));
                        break;
                    }
                }

                Vfs::close(fd);
            } else {
                kprint("usage: head <nbytes> <path>\n");
            }
        } else if (s == "heap") {
            Memory::Heap::dump_stats();
            Memory::Heap::dump_all();
        } else if (s == "hello") {
            kprint("Hello, world!\n");
        } else if (s == "help") {
            kprint("\n  {-22} {}" , "bmap"                 , "print physical memory usage bitmap"    );
            kprint("\n  {-22} {}" , "brick"                , "render the system unbootable"          );
            kprint("\n  {-22} {}" , "cat <path>..."        , "print the contents of a file"          );
            kprint("\n  {-22} {}" , "cd <path>"            , "change the working directory"          );
            kprint("\n  {-22} {}" , "clear"                , "clear the screen"                      );
            kprint("\n  {-22} {}" , "close <fd>"           , "try to close the given file nr (debug)");
            kprint("\n  {-22} {}" , "cp <path> <path>"     , "copy one file to another"              );
            kprint("\n  {-22} {}" , "die"                  , "crash and burn"                        );
            kprint("\n  {-22} {}" , "echo <text>..."       , "print a string of text"                );
            kprint("\n  {-22} {}" , "exit"                 , "disable kshell, resume userspace"      );
            kprint("\n  {-22} {}" , "halt"                 , "try to shutdown the machine"           );
            kprint("\n  {-22} {}" , "head <nbytes> <path>" , "print the first N bytes of a file"     );
            kprint("\n  {-22} {}" , "headx <nbytes> <path>", "hexdump the first N bytes of a file"   );
            kprint("\n  {-22} {}" , "heap"                 , "print heap statistics"                 );
            kprint("\n  {-22} {}" , "hello"                , "print 'Hello, World!'"                 );
            kprint("\n  {-22} {}" , "help"                 , "print this text"                       );
            kprint("\n  {-22} {}" , "kill <tid>"           , "kill the thread with the given ID"     );
            kprint("\n  {-22} {}" , "ls [path]"            , "print the contents of a directory"     );
            kprint("\n  {-22} {}" , "lsof"                 , "list open files of all processes"      );
            kprint("\n  {-22} {}" , "mbr"                  , "read and print the master boot record" );
            kprint("\n  {-22} {}" , "mem"                  , "print physical memory statistics"      );
            kprint("\n  {-22} {}" , "mount"                , "print mountpoints"                     );
            kprint("\n  {-22} {}" , "open <r|w> <path>"    , "open a file (debug)"                   );
            kprint("\n  {-22} {}" , "opendir <path>"       , "open a directory (debug)"              );
            kprint("\n  {-22} {}" , "ps"                   , "print process information"             );
            kprint("\n  {-22} {}" , "psr"                  , "print ready queue"                     );
            kprint("\n  {-22} {}" , "put <path> <text>"    , "put text in file"                      );
            kprint("\n  {-22} {}" , "pwd"                  , "print working directory"               );
            kprint("\n  {-22} {}" , "reboot"               , "reboot the machine"                    );
            kprint("\n  {-22} {}" , "rm <path>..."         , "remove a file"                         );
            kprint("\n  {-22} {}" , "tree [path]"          , "print a recursive directory listing"   );
            kprint("\n  {-22} {}" , "vgatest <w> <h>"      , "test video modes"                      );
            kprint("\n  {-22} {}" , "xd <path>..."         , "print a file in hexadecimal"           );
            kprint("\n\n");
        } else if (s == "kill") {
            pid_t tid;
            if (argc == 2 && string_to_num(argv[1], tid)) {
                Process::thread_t *t = Process::thread_by_tid(tid);
                if (t)
                     Process::delete_thread(t);
                else kprint("no thread with exists with tid {}\n", tid);
            } else {
                kprint("usage: kill <tid>\n");
            }
        } else if (s == "ls") {

            if (argc > 2)
                kprint("usage: ls [path]\n");

            StringView path = argc >= 2 ? argv[1] : ".";

            fd_t fd = Vfs::open(path, o_dir);
            if (fd < 0) {
                kprint("open error: {}\n", error_name(fd));
                return;
            }

            if (fd >= 0) {
                kprint("  {-6} {-10} {-6} {}\n", "INODE", "MODE", "SIZE", "NAME");

                dir_entry_t entry;
                while (true) {
                    int n = Vfs::read_dir(fd, entry);
                    if (n > 0) {
                        if (entry.inode.type == t_dir) {
                            kprint("  {6} {} {6 } {}/\n"
                                   ,entry.inode.i
                                   ,format_inode_mode(entry.inode)
                                   ,""
                                   ,entry.name);
                        } else {
                            kprint("  {6} {} {4S} {}\n"
                                   ,entry.inode.i
                                   ,format_inode_mode(entry.inode)
                                   ,entry.inode.size
                                   ,entry.name);
                        }
                    } else {
                        if (n != 0)
                            kprint("read_dir error: {}\n", error_name(n));
                        break;
                    }
                }
            }

            Vfs::close(fd);

        } else if (s == "lsof") {
            Vfs::dump_open_files();

        } else if (s == "mbr") {
            alignas(512) Array<u8,512> buffer;
            Driver::Disk::Ata::test(buffer);
            errno_t err = Driver::Disk::Ata::read(0, 0, buffer.data(), 1);
            if (err) {
                kprint("error: {}\n", error_name(err));
            } else {
                hex_dump(buffer.data(), 512);
            }
        } else if (s == "mem") {
            Memory::Physical::dump_stats();
        } else if (s == "mount") {
            Vfs::dump_mounts();
        } else if (s == "open") {
            if (argc == 3 && (argv[1] == "r" || argv[1] == "w")) {
                errno_t err = Vfs::open(argv[2], argv[1] == "w" ? o_write : o_read);
                if (err < 0)
                    kprint("open failed: {}\n", error_name(err));
            } else {
                kprint("usage: open <r|w> <path>\n");
            }
        } else if (s == "opendir") {
            if (argc == 2) {
                errno_t err = Vfs::open(argv[1], o_dir);
                if (err < 0)
                    kprint("open failed: {}\n", error_name(err));
            } else {
                kprint("usage: opendir <path>\n");
            }
        } else if (s == "ps") {
            Process::dump_all();
        } else if (s == "psr") {
            Process::dump_ready_queue();
        } else if (s == "put") {
            if (argc >= 2) {
                auto path = argv[1];
                fd_t fd = Vfs::open(path, o_write);
                if (fd < 0) {
                    kprint("open failed: {}\n", error_name(fd));
                    return;
                }

                for (size_t i : range(2, argc)) {
                    ssize_t err = Vfs::write(fd, argv[i].data(), argv[i].length());
                    if (err < 0) {
                        kprint("write failed: {}\n", error_name(err));
                        break;
                    }
                }
                Vfs::close(fd);

            } else {
                kprint("usage: put <path> <text>\n");
            }
        } else if (s == "pwd") {
            kprint("{}\n", Process::current_proc()->working_directory);
        } else if (s == "rm") {
            if (argc >= 2) {
                for (size_t i : range(1, argc)) {
                    errno_t err = Vfs::unlink(argv[i]);
                    if (err < 0)
                        kprint("unlink failed: {}\n", error_name(err));
                }
            } else {
                kprint("usage: rm <path>...\n");
            }
        } else if (s == "reboot") {
            kprint("\n** system reset **\n");
            // Io::wait(1_M);
            Io::out_8(0x64, 0xfe);
        } else if (s == "tree") {
            if (argc == 2) {
                tree(argv[1]);
            } else {
                tree(".");
            }
        } else if (s == "vgatest") {
            u32 w, h;
            if (argc == 3
             && string_to_num(argv[1], w)
             && string_to_num(argv[2], h)) {

                Driver::Vga::test(w, h);
            } else {
                kprint("usage: vgatest <w> <h>\n");
            }
        } else {
            path_t path = s;
            if (!path.ends_with(".elf"))
                path += ".elf";

            fd_t fd = Vfs::open(path, o_read);
            if (fd >= 0) {
                Vfs::close(fd);

                Process::proc_t *proc;
                Process::proc_arg_spec_t args_spec;

                for (size_t i : range(argc))
                    args_spec[i] = argv[i];

                errno_t err = Elf::load_elf(path, args_spec, proc);
                if (err == ERR_success) {
                    kprint("elf loaded (press esc to return)\n");
                    disable();
                } else {
                    kprint("not an executable file: '{}' ({})\n", path, error_name(err));
                }
            } else {
                kprint("no such file or command '{}'\n", s);
            }
        }
    }

    void kshell() {

        // kprint("\nto enable the kernel shell, enter Esc (0x1b) in the serial terminal or press F1 on your keyboard\n\n");
        {
            path_t rootfs = "/";

            int fd = Vfs::open("/", o_dir);
            assert(fd > 0, "could not open vfs root");
            while (true) {
                dir_entry_t entry;
                int n = Vfs::read_dir(fd, entry);
                if (n > 0) {
                    if (entry.name.starts_with("disk")) {
                        rootfs += entry.name;
                        break;
                    }
                } else if (n == 0) {
                    break;
                } else {
                    kprint("couldn't read root directory: {}\n", error_name(n));
                }
            }
            Vfs::close(fd);

            if (rootfs != "/") {
                Process::current_proc()->working_directory = rootfs;

                path_t rcfile = rootfs;
                rcfile += "/autoexec.bat"; // :D
                fd = Vfs::open(rcfile, o_read);

                if (fd >= 0) {
                    cmdline_t line = "";
                    Array<char, 2> buffer;
                    while (true) {
                        int n = Vfs::read(fd, buffer);
                        if (n <= 0) {
                            if (n != 0)
                                kprint("read failed: {}\n", error_name(n));
                            break;
                        }

                        for (int i : range(n)) {
                            if (buffer[i] == '\n') {
                                // kprint("line: <{}>\n", line);
                                args_t args = split_args(line);
                                // kprint("args: <{},{}>\n", args.argc, args.argv);
                                run_command(args);
                                line = "";
                            } else {
                                line += buffer[i];
                            }
                        }
                    }
                    Vfs::close(fd);
                }
            }
        }

        wait(lock);

        kprint("\n");
        kprint("** enabled kshell **\n");
        kprint("\n");

        cmdline_t last_command = "";

        while (true) {

            String<max_path_length+10> prompt;
            fmt(prompt, "[kernel {}]# ", Process::current_proc()->working_directory);
            kprint("{}_\b", prompt);

            cmdline_t line = "";

            input_enabled_ = true;

            while (true) {
                char ch = input.dequeue();

                if (ch == '\b' || ch == ascii_ctrl('?')) { // ^?, or 0x7f, is the DEL character.
                    if (line.length()) {
                        line[line.length()-1] = 0;
                        line.length_--;

                        // Since we cannot currently manipulate the hardware VGA cursor,
                        // we manually output an underscore at the prompt.
                        kprint(" \b\b_\b");
                    }
                } else if (ch == ascii_ctrl('W')) {
                    // Delete word.

                    while (line.length()) {
                        line.pop();
                        kprint(" \b\b_\b");
                        if (line.length() && is_space(line[line.length()-1]))
                            break;
                    }
                } else if (ch == ascii_ctrl('U') || ch == ascii_ctrl('C')) {
                    // Delete line.

                    while (line.length()) {
                        line.pop();
                        kprint(" \b\b_\b");
                    }
                } else if (ch == '\t' || ch == ascii_ctrl('P') || ch == ascii_ctrl('N')) {
                    // Toggle history.
                    // (swaps between current and previous command)
                    auto tmp = line;
                    while (line.length()) {
                        line.pop();
                        kprint(" \b\b_\b");
                    }
                    line         = last_command;
                    last_command = tmp;

                    for (char c : line)
                        kprint("{}", c);
                    kprint("_\b");

                } else if (ch == '\n' || ch == '\r') {
                    if (line == "")
                        {kprint(" \n"); break;} //line = last_command;
                    else last_command = line;

                    kprint(" \n");

                    args_t args = split_args(line);
                    // kprint("args: <{},{}>\n", args.argc, args.argv);

                    run_command(args);

                    break;

                } else if (line == "" && ch == ascii_ctrl('D')) {
                    kprint("\nbye!\n");

                    // Magical shutdown sequence (see "halt" command).
                    Io::out_16(0xB004, 0x2000);
                    Io::out_16(0x0604, 0x2000);
                    Io::out_16(0x4004, 0x3400);

                    // (eof)
                    args_t args = {{"exit"}, 1};

                    run_command(args);
                    break;

                } else if (ch == '\f') {
                    // Clear the screen on ^L.
                    // (does not work in the VGA console)
                    kprint("\n\x1b[2J\x1b[1;1H{}{}_\b", prompt, line);

                } else if (is_print(ch) && line.length() < line.size()) {
                    line += ch;
                    kprint("{}_\b", ch);
                }
            }
        }
    }
}
