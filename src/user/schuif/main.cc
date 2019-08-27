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

// A tiny slideshow presentation viewer.
// Works on directories containing RLE-encoded images, named 00.rli to NN.rli.
// The tool used to generate/compress slides is not distributed in this repo at
// this time.
// See read_rli - a file is a flat list of packed  { u16 i; u32 val; } structs,
// denoting repetition count and 32-bit RGBA pixel value, respectively.
// Image size is always assumed to be 1920x1080.

using namespace ostd;

constexpr int max_slides = 100;

constexpr int screen_width  = 1920;
constexpr int screen_height = 1080;

/// Files.
fd_t video, keyboard;

/// A framebuffer
using buffer_t = Array<u32, screen_width*screen_height>;

/// The primary buffer.
buffer_t buffer;

/// Directory name of deck.
StringView dir;

int nslides        =  0;
int current_slide  = -1;

/// Get path to slide image from slide number.
constexpr String<max_path_length> slide_file_name(int n) {
    String<max_path_length> name;
    fmt(name, "{}/{02}.rli", dir, n);
    return name;
}

/// Write current buffer to screen.
static void flush() {
    seek(video, 0, 0);
    write(video, buffer.data(), buffer.size() * sizeof(buffer[0]));
}

/// Clear buffer.
static void clear() {
    for (auto &v : buffer)
        v = 0;
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
                print(stderr, "could not read from slide: {}\n", error_name(ret));
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

/// Load a slide into a buffer.
static void load_slide(int sliden, buffer_t &dest) {
    auto name = slide_file_name(sliden);

    fd_t fd = open(name, "r");
    if (fd < 0) {
        print(stderr, "could not open slide {}: {}\n", name, error_name(fd));
        sys_thread_delete(0);
    }

    read_rli(fd, dest);

    close(fd);
}

/// Change video mode.
static void mode_set(int w, int h) {
    fd_t mode = open("/dev/video-mode", "wt");
    String<64> s;
    fmt(s, "{}x{}\n", w, h);
    write(mode, s.data(), s.length());
    close(mode);
}

static int get_slide_count() {
    for (int n = 0; n < max_slides; ++n) {
        auto name = slide_file_name(n);
        fd_t fd = open(name, "r");
        if (fd < 0)
            return n;
        close(fd);
    }
    return 0;
}

/// Present a slide. Optionally shows transition.
static void show_slide(int n, bool fast = false) {
    if (n < 0) n = 0;
    if (n >= nslides) n = nslides - 1;

    if (n == current_slide)
        return;

    if (fast) {
        load_slide(n, buffer);
        flush();
        current_slide = n;
        return;
    }

    static buffer_t buffer_alt;

    constexpr int N = 4; // iterations (number of logical scanlines)
    constexpr int M = 2; // logical scanline height
    load_slide(n, buffer_alt);

    for (int i : range(N)) {
        for (int y : range(screen_height)) {
            if (y / M % N != i)
                continue;
            for (int x : range(screen_width))
                buffer[y*screen_width + x] = buffer_alt[y*screen_width + x];
        }
        flush();
    }
    current_slide = n;
}

static void run(int start_slide) {

    show_slide(start_slide);

    // Handle input.
    while (true) {
        ssize_t x = getchar(keyboard);
        switch (x) {
        case 'f':
        case 'n':
        case '\n':
        case '\r':
        case ' ':
        case '.':
        case '>':
            show_slide(current_slide + 1); break;
        case 'F':
            show_slide(current_slide + 1, true); break;
        case ascii_ctrl('f'):
            show_slide(current_slide + 4); break;
        case 'b':
        case 'p':
        case '\b':
        case ',':
        case '<':
            show_slide(current_slide - 1); break;
        case 'B':
            show_slide(current_slide - 1, true); break;
        case ascii_ctrl('b'):
            show_slide(current_slide - 4); break;
        case ascii_ctrl('a'):
            show_slide(0); break;
        case ascii_ctrl('e'):
            show_slide(nslides - 1); break;
        case 'q':
            return;

        default:
            // print("have: {08x} {}\n", x, x);
            break;
        }
    }
}

int main(int argc, const char **argv) {

    if (argc != 2 && argc != 3) {
        print(stderr, "usage: {} SLIDE_DIR [START]\n", argv[0]);
        return 1;
    }

    int start_slide = 0;
    dir = argv[1];

    if (argc == 3) {
        if (!string_to_num(argv[2], start_slide)) {
            print(stderr, "usage: {} SLIDE_DIR [START]\n", argv[0]);
            return 1;
        }
    }

    nslides = get_slide_count();
    if (!nslides) {
        print(stderr, "no slides (NN.rli files) found in {}\n", dir);
        return 1;
    }

    if (start_slide < 0 || start_slide >= nslides)
        start_slide = 0;

    mode_set(screen_width, screen_height);

    keyboard = open("/dev/keyboard", "r");
    if (keyboard < 0) {
        print("could not open keyboard: {}\n", error_name(keyboard));
        return 1;
    }

    video = open("/dev/video", "w");
    if (video < 0) {
        print("could not open video: {}\n", error_name(video));
        return 1;
    }

    clear();
    flush();

    run(start_slide);

    clear();
    flush();

    {
        // Temporary: Show the splash screen.
        mode_set(800, 600);
        fd_t fd = open("/disk0p1/splash.rli", "r");
        if (fd >= 0) {
            read_rli(fd, buffer);
            flush();
            close(fd);
        }
    }

    return 0;
}
