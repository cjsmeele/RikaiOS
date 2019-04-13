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
 * \namespace Memory::Heap
 *
 * Kernel Heap: A memory allocator for the kernel.
 *
 * This allocator is cheap, in the "you get what you pay for" sense. :^)
 */
namespace Memory::Heap {

    void dump_stats();
    void dump_all();

    [[nodiscard]]
    void *alloc(size_t size, size_t align);
    void  free(void *p);

    void init();
}
