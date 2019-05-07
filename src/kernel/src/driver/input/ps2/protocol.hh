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
#include "../../driver.hh"

/**
 * \namespace Driver::Input::Ps2::Protocol
 *
 * These functions handle communication with PS/2 devices.
 *
 * The PS/2 controller commonly has 2 connections: The first one connecting a
 * keyboard, the second one a mouse (also called the AUX device).
 */
namespace Driver::Input::Ps2::Protocol {

    /// Tries to read a data byte (such as a scancode, or part of a mouse packet).
    /// Returns false if no data is available.
    bool read_data(u8 &byte);

    /// Returns true if data is available to be read.
    bool output_full();

    /// Flushes all pending input data.
    void flush();

    struct init_result_t {
        bool have_keyboard;
        bool have_mouse;
        u8   mouse_type; ///< Indicates mouse features such as the scrollwheel.
    };

    /// Initialises the PS/2 controller and tries to initialise attached devices.
    init_result_t init_controller();
}
