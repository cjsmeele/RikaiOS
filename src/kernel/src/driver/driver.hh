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
 * All supported drivers.
 *
 * Normally, we would scan extension buses (PCI, USB, etc.) to detect installed
 * hardware and load drivers accordingly. However, because of the limited
 * hardware that we need to support, we take a simpler approach: We initialize
 * all supported drivers, and let them figure out themselves if they are
 * needed.
 */
namespace Driver {

    /// Load all required drivers.
    void init();
}

#define DRIVER_NAME(x) constexpr static StringView driver_name_ = (x)

#define dprint(...) kprint_from(driver_name_, __VA_ARGS__)
