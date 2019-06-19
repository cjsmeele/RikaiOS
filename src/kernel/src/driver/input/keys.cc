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

    /// Normal character values for all keys.
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

    /// Character values for keys when control is pressed.
    static constexpr Array<char, (size_t)Key::End_> key_chars_control {
        0,
        // ^A, ^B, etc.
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
        14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
        '0', '1',  0, '3', '4', '5', 30, '7', '8', '9',
        // 2 = ^@ -^         6 = ^^ -^

        '`', '-', '=',
        '\x1b', '\x1d', '\x1c',
        ';', '\'',
        ',', '.', '\x1f', // ( 'C-/' is used to send ^_ in my terminals, for some reason)

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

    /// Character values for keys when shift is pressed.
    static constexpr Array<char, (size_t)Key::End_> key_chars_shift {
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


    // Rudimentary keymap for a dvorak layout.
    // This only needs to change the alphabet and some symbol-only keys.
    //
    // Just for my convenience, this also remaps CapsLock to LControl.
    //
    // (this is a hack to make typing bearable for me until we have proper keymaps)
    //
    KeyMap keymap_dvorak = {
        Key::None, Key::a, Key::x, Key::j, Key::e, Key::Period, Key::u, Key::i, Key::d, Key::c, Key::h, Key::t, Key::n, Key::m, Key::b, Key::r, Key::l, Key::Quote, Key::p, Key::o, Key::y, Key::g, Key::k, Key::Comma, Key::q, Key::f, Key::Semicolon, Key::n0, Key::n1, Key::n2, Key::n3, Key::n4, Key::n5, Key::n6, Key::n7, Key::n8, Key::n9, Key::Backtick, Key::BracketLeft, Key::BracketRight, Key::Slash, Key::Equals, Key::Backslash, Key::s, Key::Minus, Key::w, Key::v, Key::z, Key::Esc, Key::F1, Key::F2, Key::F3, Key::F4, Key::F5, Key::F6, Key::F7, Key::F8, Key::F9, Key::F10, Key::F11, Key::F12, Key::PrintScreen, Key::AltSysRq, Key::Pause, Key::Break, Key::Home, Key::End, Key::PageUp, Key::PageDown, Key::Insert, Key::Delete, Key::Tab, Key::Space, Key::Backspace, Key::Enter, Key::Up, Key::Down, Key::Left, Key::Right, Key::Menu, Key::LShift, Key::RShift, Key::LControl, Key::RControl, Key::LAlt, Key::RAlt, Key::LSuper, Key::RSuper, Key::LHyper, Key::RHyper, Key::LMeta, Key::RMeta, Key::NumLock, Key::LControl, Key::ScrollLock, Key::kpPeriod, Key::kpPlus, Key::kpMinus, Key::kpAsterisk, Key::kpSlash, Key::kp0, Key::kp1, Key::kp2, Key::kp3, Key::kp4, Key::kp5, Key::kp6, Key::kp7, Key::kp8, Key::kp9, Key::kpEnter, Key::Power, Key::Sleep, Key::Wake, Key::End_
    };

    char key_to_char(Key  key
                    ,bool shift
                    ,bool control
                    ,bool alt) {

        (void)alt; // (don't care)

        if ((size_t)key > key_chars.size())
            return 0;

             if (control) return key_chars_control[(size_t)key];
        else if (shift)   return key_chars_shift  [(size_t)key];
        else              return key_chars        [(size_t)key];

             // return key_chars_shifted[(size_t)qwerty_to_dvorak[(size_t)key]];
        // else return key_chars        [(size_t)qwerty_to_dvorak[(size_t)key]];
    }

    StringView key_to_name(Key key) {
        if ((size_t)key > key_names.size())
             return 0;
        else return key_names[(size_t)key];
    }
}
