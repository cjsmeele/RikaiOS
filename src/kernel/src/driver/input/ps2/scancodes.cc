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
#include "scancodes.hh"
#include "../keys.hh"

namespace Driver::Input::Ps2 {

    /**
     * \name Tables that map PS/2 keyboard scancodes to Key numbers.
     */
    ///@{

    /// The scancodes for these keys are prefixed with 0xf0 on key release.
    const Array<Key, 256> keys_normal {
                   Key::None
        /* 01 */ , Key::F9
                 , Key::None
        /* 03 */ , Key::F5
        /* 04 */ , Key::F3
        /* 05 */ , Key::F1
        /* 06 */ , Key::F2
        /* 07 */ , Key::F12
                 , Key::None
        /* 09 */ , Key::F10
        /* 0a */ , Key::F8
        /* 0b */ , Key::F6
        /* 0c */ , Key::F4
        /* 0d */ , Key::Tab
        /* 0e */ , Key::Backtick
                 , Key::None
                 , Key::None
        /* 11 */ , Key::LAlt
        /* 12 */ , Key::LShift
                 , Key::None
        /* 14 */ , Key::LControl
        /* 15 */ , Key::q
        /* 16 */ , Key::n1
                 , Key::None
                 , Key::None
                 , Key::None
        /* 1a */ , Key::z
        /* 1b */ , Key::s
        /* 1c */ , Key::a
        /* 1d */ , Key::w
        /* 1e */ , Key::n2
                 , Key::None
                 , Key::None
        /* 21 */ , Key::c
        /* 22 */ , Key::x
        /* 23 */ , Key::d
        /* 24 */ , Key::e
        /* 25 */ , Key::n4
        /* 26 */ , Key::n3
                 , Key::None
                 , Key::None
        /* 29 */ , Key::Space
        /* 2a */ , Key::v
        /* 2b */ , Key::f
        /* 2c */ , Key::t
        /* 2d */ , Key::r
        /* 2e */ , Key::n5
                 , Key::None
                 , Key::None
        /* 31 */ , Key::n
        /* 32 */ , Key::b
        /* 33 */ , Key::h
        /* 34 */ , Key::g
        /* 35 */ , Key::y
        /* 36 */ , Key::n6
                 , Key::None
                 , Key::None
                 , Key::None
        /* 3a */ , Key::m
        /* 3b */ , Key::j
        /* 3c */ , Key::u
        /* 3d */ , Key::n7
        /* 3e */ , Key::n8
                 , Key::None
                 , Key::None
        /* 41 */ , Key::Comma
        /* 42 */ , Key::k
        /* 43 */ , Key::i
        /* 44 */ , Key::o
        /* 45 */ , Key::n0
        /* 46 */ , Key::n9
                 , Key::None
                 , Key::None
        /* 49 */ , Key::Period
        /* 4a */ , Key::Slash
        /* 4b */ , Key::l
        /* 4c */ , Key::Semicolon
        /* 4d */ , Key::p
        /* 4e */ , Key::Minus
                 , Key::None
                 , Key::None
                 , Key::None
        /* 52 */ , Key::Quote
                 , Key::None
        /* 54 */ , Key::BracketLeft
        /* 55 */ , Key::Equals
                 , Key::None
                 , Key::None
        /* 58 */ , Key::CapsLock
        /* 59 */ , Key::RShift
        /* 5a */ , Key::Enter
        /* 5b */ , Key::BracketRight
                 , Key::None
        /* 5d */ , Key::Backslash
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
        /* 66 */ , Key::Backspace
                 , Key::None
                 , Key::None
        /* 69 */ , Key::kp1
                 , Key::None
        /* 6b */ , Key::kp4
        /* 6c */ , Key::kp7
                 , Key::None
                 , Key::None
                 , Key::None
        /* 70 */ , Key::kp0
        /* 71 */ , Key::kpPeriod
        /* 72 */ , Key::kp2
        /* 73 */ , Key::kp5
        /* 74 */ , Key::kp6
        /* 75 */ , Key::kp8
        /* 76 */ , Key::Esc
        /* 77 */ , Key::NumLock
        /* 78 */ , Key::F11
        /* 79 */ , Key::kpPlus
        /* 7a */ , Key::kp3
        /* 7b */ , Key::kpMinus
        /* 7c */ , Key::kpAsterisk
        /* 7d */ , Key::kp9
        /* 7e */ , Key::ScrollLock
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
        /* 83 */ , Key::F7
        /* 84 */ , Key::AltSysRq
                 , Key::None
    };

    /// The scancodes for these keys are prefixed with 0xe0 on keypress, and
    /// 0xe0 0xf0 on release.
    const Array<Key, 256> keys_escaped {
                   Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
        /* 11 */ , Key::RAlt
                 , Key::None
                 , Key::None
        /* 14 */ , Key::RControl
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
        /* 1f */ , Key::LSuper
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
        /* 27 */ , Key::RSuper
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
        /* 2f */ , Key::Menu
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
        /* 37 */ , Key::Power
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
        /* 3f */ , Key::Sleep
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
        /* 4a */ , Key::kpSlash
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
        /* 5a */ , Key::kpEnter
                 , Key::None
                 , Key::None
                 , Key::None
        /* 5e */ , Key::Wake
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
        /* 69 */ , Key::End
                 , Key::None
        /* 6b */ , Key::Left
        /* 6c */ , Key::Home
                 , Key::None
                 , Key::None
                 , Key::None
        /* 70 */ , Key::Insert
        /* 71 */ , Key::Delete
        /* 72 */ , Key::Down
                 , Key::None
        /* 74 */ , Key::Right
        /* 75 */ , Key::Up
                 , Key::None
                 , Key::None
                 , Key::None
                 , Key::None
        /* 7a */ , Key::PageDown
                 , Key::None
        /* 7c */ , Key::PrintScreen
        /* 7d */ , Key::PageUp
        /* 7e */ , Key::Break
                 , Key::None
    };
    ///@}

    // The pause key is weird, let's forget it exists.
    // scancode: e1 14 77  Pause
}
