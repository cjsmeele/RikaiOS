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
#include <io.hh>
#include <proc.hh>
#include <os-std/ostd.hh>

using namespace ostd;

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
    if (argv[argc].length_)
        argc++;

    return { argv, argc };
}

String<max_path_length+16> prompt;
String<max_path_length>    cwd;

/// Shell builtins.
/// returns true if the command was handled, false if it is not a builtin.
static bool handle_builtin(const args_t &args) {

    size_t argc         = args.argc;
    args_t::argv_t argv = args.argv;

    if (argv[0] == "cd") {
        if (argc != 2) {
            print(stderr, "usage: cd <path>\n");
            return true;
        }
        errno_t err = set_cwd(argv[1]);
        if (err < 0) {
            print(stderr, "error: could not change working directory: {}\n"
                 ,error_name(err));
            return true;
        }

        // Get the canonicalised absolute path (resolves e.g. .. and .)
        get_cwd(cwd);

        return true;

    } else if (argv[0] == "pwd") {
        if (argc != 1) {
            print(stderr, "usage: pwd\n");
            return true;
        }
        print("{}\n", cwd);
        return true;

    } else if (argv[0] == "put") {
        if (argc < 2) {
            print(stderr, "usage: put <file> [text]...\n");
            return true;
        }

        fd_t fd = open(argv[1], "w");
        if (fd < 0) {
            print(stderr, "open failed: {}\n", error_name(fd));
            return true;
        }

        for (size_t i : range(2, argc)) {
            ssize_t err = write(fd, argv[i]);
            if (err < 0) {
                print(stderr, "write failed: {}\n", error_name(err));
                break;
            }
        }
        write(fd, '\n');
        close(fd);
        return true;

    } else if (argv[0] == "test") {

        // fd_t in, out;
        // errno_t err = pipe(in, out);
        // if (err < 0) {
        //     print(stderr, "pipe failed: {}\n", error_name(err));
        //     return true;
        // }

        fd_t kb = open("/dev/keyboard", "r");
        if (kb < 0) {
            print(stderr, "could not open keyboard: {}\n", error_name(kb));
            return true;
        }

        Array<StringView, 1> args_ { "shell" };
        spawn("shell.elf", args_, true, { kb, spawn_fd_inherit, spawn_fd_inherit });

        // close(in);
        // close(out);

        return true;
    }

    return false;
}

int main(int, const char**) {
    // (arguments are ignored)

    print("Hello this is shell\n");

    cmdline_t cmdline;
    get_cwd(cwd);

    while (true) {
        prompt = "";
        fmt(prompt, "{} % ", cwd);
        print(prompt);

        ssize_t n = getline(cmdline);
        if (n <  0) return 1;
        if (n == 0) break; // EOF.

        cmdline.rtrim();
        if (cmdline.length()) {

            args_t args = split_args(cmdline);
            const auto& [argv, argc] = args;

            if (!handle_builtin(args)) {
                errno_t err = spawn(argv[0], argv, true);

                if (err == ERR_not_exists) {
                    // File does not exist? Append ".elf" and try again.
                    String<max_path_length> path = argv[0];
                    path += ".elf";
                    err = spawn(path, argv, true);
                }

                if (err < 0)
                    print("cannot run <{}>: {}\n", argv[0], error_name(err));
            }
        }
    }
    print("\nshell exit (EOF)\n");

    return 0;
}
