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
#include "textmode.hh"

namespace Console::TextMode {

    /**
     * Text-mode video memory.
     *
     * Addresses from `0xb8000` map to memory that is used by the VGA to draw
     * text onto the screen in text mode.
     *
     * The text-mode screen is 80x25 characters, represented in memory by an
     * array of 80*25 16-bit values. The lower 8 bits contain the character,
     * while the upper 8 bits contain a color attribute (lower 4-bits
     * foreground, higher 4 bits background).
     */
    auto vram = (volatile u16*)0xb8000;

    static constexpr int screen_cols = 80;
    static constexpr int screen_rows = 25;

    static int cursor_x = 0;
    static int cursor_y = 0;

    /// Determines text color.
    static constexpr u8 attribute = 0x0f;

    constexpr static u16 char_to_cell(char c) {
        return u16(attribute) << 8 | c;
    }

    static void scroll() {
        // Move all rows up by one.
        for (int y = 1; y < screen_rows; ++y) {
            for (int x = 0; x < screen_cols; ++x) {
                vram[(y-1)*screen_cols+x]
              = vram[(y-0)*screen_cols+x];
            }
        }
        // Clear the empty line at the bottom.
        for (int x = 0; x < screen_cols; ++x) {
            vram[(screen_rows-1)*screen_cols+x]
                = char_to_cell(' ');
        }
        cursor_y--;
    }

    static void newline() {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= screen_rows)
            scroll();
    }

    static void clear_screen() {
        for (int y = 0; y < 25; ++y) {
            for (int x = 0; x < 80; ++x) {
                ((volatile u16*)0xb8000)[y*80+x] = 0x0720;
            }
        }

        cursor_x = 0;
        cursor_y = 0;
    }

    void print_char(char c) {

               if (c == '\n') { newline();
        } else if (c == '\r') { cursor_x = 0;   // carriage return
        } else if (c == '\b') { if (cursor_x > 0) cursor_x--;
        } else if (c == 0x0c) { clear_screen(); // ^L (form feed)
        } else {
            vram[cursor_y*screen_cols + cursor_x++]
                = char_to_cell(c);

            if (cursor_x >= screen_cols)
                newline();
        }
    }

    void init() {
        clear_screen();
    }
}
