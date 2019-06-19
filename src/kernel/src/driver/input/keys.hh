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
#pragma once

#include "common.hh"

/**
 * \namespace Driver::Input
 *
 * Functions and data related to input devices.
 */
namespace Driver::Input {

    /**
     * All possible keyboard buttons.
     *
     * This would also include many international keys if we were that
     * ambitious. Currently we assume US-style keyboard keys.
     */
    enum class Key : s32 {
        None = 0,                                          // 0

        a, b, c, d, e, f, g, h, i, j, k, l, m,             // 13
        n, o, p, q, r, s, t, u, v, w, x, y, z,             // 26

        // Number keys.
        n0, n1, n2, n3, n4, n5, n6, n7, n8, n9,            // 36

        Backtick, Minus, Equals,                           // 39
        BracketLeft, BracketRight, Backslash,              // 42
        Semicolon, Quote,                                  // 44
        Comma, Period, Slash,                              // 47

        Esc,                                               // 48
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, // 60
        PrintScreen, AltSysRq, Pause, Break,               // 64
        Home, End, PageUp, PageDown, Insert, Delete,       // 70

        Tab, Space, Backspace, Enter,                      // 74

        Up, Down, Left, Right,                             // 78

        Menu,                                              // 79
        LShift,   RShift,                                  // 81
        LControl, RControl,                                // 83
        LAlt,     RAlt,                                    // 85
        LSuper,   RSuper,                                  // 87

        // Let's throw in some space cadet keys while we're at it.
        LHyper,   RHyper,                                  // 89
        LMeta,    RMeta,                                   // 91

        NumLock, CapsLock, ScrollLock,                     // 94

        kpPeriod, kpPlus, kpMinus, kpAsterisk, kpSlash,    // 99
        kp0, kp1, kp2, kp3, kp4, kp5, kp6, kp7, kp8, kp9,  // 109
        kpEnter,                                           // 110

        Power, Sleep, Wake,                                // 113
        End_
    };

    /**
     * Key map type.
     *
     * This could be used to map keys from one layout to another (a quick and
     * dirty solution for supporting e.g. the dvorak layout, which normally has
     * the same shift states on the same keys compared to qwerty).
     */
    using KeyMap = Array<Key, 256>;

    extern KeyMap keymap_dvorak;

    /**
     * Get the ASCII character corresponding to the given key.
     *
     * shifted = whether the shift key is pressed.
     */
    char key_to_char(Key  key
                    ,bool shift   = false
                    ,bool control = false
                    ,bool alt     = false);

    /// Get the human-readable name of the given key.
    StringView key_to_name(Key key);
}
