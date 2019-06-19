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

/** \file
 * Functions that are required to be present by the compiler / builtins.
 */

void *__dso_handle = nullptr;

/// Handler for pure virtual function calls.
extern "C" void __cxa_pure_virtual();
extern "C" void __cxa_pure_virtual() {
    // Simply hang the machine, this shouldn't happen anyway.
    asm volatile ("cli \n hlt");
}

// FIXME: Maybe not required in the kernel, but certainly needed in userland!
//        This should register a function to be run on program exit.
extern "C" void __cxa_atexit(void (*)(void*), void*, void*);
extern "C" void __cxa_atexit(void (*)(void*), void*, void*) { }
