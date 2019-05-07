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
#include "keys.hh"

namespace Driver::Input {

    static constexpr Array<StringView, (size_t)Key::End_> key_names {
        "None",

        "A","B","C","D","E","F","G","H","I","J","K","L","M",
        "N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
        "0","1","2","3","4","5","6","7","8","9",

        "Backtick", "Minus", "Equals",
        "BracketLeft", "BracketRight", "Backslash",
        "Semicolon", "Quote",
        "Comma", "Period", "Slash",

        "Esc",
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
        "PrintScreen", "AltSysRq", "Pause", "Break",
        "Home", "End", "PageUp", "PageDown", "Insert", "Delete",

        "Tab", "Space", "Backspace", "Enter",

        "Up", "Down", "Left", "Right",

        "Menu",
        "LShift",   "RShift",
        "LControl", "RControl",
        "LAlt",     "RAlt",
        "LSuper",   "RSuper",
        "LHyper",   "RHyper",
        "LMeta",    "RMeta",

        "NumLock", "CapsLock", "ScrollLock",

        "kpPeriod", "kpPlus", "kpMinus", "kpAsterisk", "kpSlash",
        "KP0", "KP1", "KP2", "KP3", "KP4", "KP5", "KP6", "KP7", "KP8", "KP9",
        "KPEnter",

        "Power", "Sleep", "Wake",
    };

    static constexpr Array<char, (size_t)Key::End_> key_chars {
        0,
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

        '`', '-', '=',
        '[', ']', '\\',
        ';', '\'',
        ',', '.', '/',

        '\x1b', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0, 0, '\x7f',

        '\t', ' ', '\b', '\r',

        0, 0, 0, 0,

        0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,

        0, 0, 0,

        '.', '+', '-', '*', '/',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '\r',

        0, 0, 0,
    };

    static constexpr Array<char, (size_t)Key::End_> key_chars_shifted {
        0,
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        ')', '!', '@', '#', '$', '%', '^', '&', '*', '(',

        '~', '_', '+',
        '{', '}', '|',
        ':', '"',
        '<', '>', '?',

        '\x1b', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0, 0, '\x7f',

        '\t', ' ', '\b', '\r',

        0, 0, 0, 0,

        0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,

        0, 0, 0,

        '.', '+', '-', '*', '/',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '\r',

        0, 0, 0,
    };

    char key_to_char(Key key, bool shifted) {

        if ((size_t)key > key_chars.size())
            return 0;
        else if (shifted)
             return key_chars_shifted[(size_t)key];
        else return key_chars        [(size_t)key];
    }

    StringView key_to_name(Key key) {
        if ((size_t)key > key_names.size())
             return 0;
        else return key_names[(size_t)key];
    }
}
