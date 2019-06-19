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

/// Calls a function on object destruction.
template<typename F> struct scope_guard_t {
    F f;
    ~scope_guard_t()    { f(); }
     scope_guard_t(F x) : f(x) { }
};

#define ON_RETURN_JOIN2(x, y) x##y
#define ON_RETURN_JOIN1(x, y) ON_RETURN_JOIN2(x, y)
#define ON_RETURN(x) scope_guard_t ON_RETURN_JOIN1(scope_guard_, __LINE__) {[&]() { x; }};
