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
#include <os-std/ostd.hh>

// Reads a RLE-ish encoded image onto the monitor.

using namespace ostd;

constexpr int max_width  = 1920;
constexpr int max_height = 1080;

fd_t video;

using buffer_t = Array<u32, max_width*max_height>;

buffer_t buffer;

static void flush() {
    seek(video, 0, 0);
    write(video, buffer.data(), buffer.size() * sizeof(buffer[0]));
}

/// Read run-length encoded image.
/// Note: this is not the Utah RLE format.
///       Instead it's simply a RLE-encoded RGBA framebuffer.
static void read_rli(fd_t fd, buffer_t &buf) {

    struct rli_item_t { u16 i; u32 val; } __attribute__((packed));

    // Attempt to read up to 16K at a time.
    static Array<rli_item_t, 16*1024> tmp;

    size_t bufpos = 0;

    while(true) {
        // Read a rounded amount of bytes.
        ssize_t bytes = 0;
        while (!(bytes / sizeof(rli_item_t))
             || bytes  % sizeof(rli_item_t)) {
            ssize_t ret = read(fd
                              ,((u8*)tmp.data())+bytes
                              ,max(sizeof(rli_item_t) - bytes % sizeof(rli_item_t)
                                  ,tmp.size()*sizeof(rli_item_t) - bytes));
            if (ret < 0) {
                print(stderr, "could not read from image: {}\n", error_name(ret));
                sys_thread_delete(0);
            }

            bytes += ret;

            if (ret == 0)
                return;
        }

        // Process RLI entries.
        for (auto [i, item] : enumerate(tmp)) {
            if (i >= bytes/sizeof(rli_item_t))
                break;

            for (auto n : range(item.i)) {
                if (bufpos >= buf.size())
                    // Overflow?
                    return;
                buf[bufpos++] = item.val;
            }
        }
    }
}

static void mode_set(int w, int h) {
    fd_t mode = open("/dev/video-mode", "wt");
    String<64> s;
    fmt(s, "{}x{}\n", w, h);
    write(mode, s.data(), s.length());
    close(mode);
}

static void usage() {
    print(stderr, "usage: view [WIDTH] [HEIGHT] FILE\n");
}

int main(int argc, const char **argv) {

    int w = 1920;
    int h = 1080;
    StringView fname;

    if (argc == 4) {
        if (!string_to_num(argv[1], w)) { usage(); return 1; }
        if (!string_to_num(argv[2], h)) { usage(); return 1; }
        fname = argv[3];
    } else if (argc == 2) {
        fname = argv[1];
    } else {
        usage(); return 1;
    }

    mode_set(w, h);

    video = open("/dev/video", "w");
    if (video < 0) {
        print("could not open video: {}\n", error_name(video));
        return 1;
    }

    fd_t fd = open(fname, "r");
    if (fd < 0) {
        print(stderr, "could not open image {}: {}\n", fname, error_name(fd));
        return 1;
    }

    read_rli(fd, buffer);
    close(fd);

    flush();

    return 0;
}
