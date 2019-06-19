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

#include <os-std/types.hh>

// Standard new and delete.

void *operator new  (malloc_size_t size);
void *operator new[](malloc_size_t size);

void  operator delete  (void*);
void  operator delete[](void*);

// New and delete with alignment.

namespace std { enum class align_val_t : malloc_size_t {}; }
void *operator new(malloc_size_t size, std::align_val_t align);
void  operator delete  (void*, malloc_size_t);
void  operator delete[](void*, malloc_size_t);

// Placement new and delete (does not allocate).

void *operator new      (malloc_size_t, void*);
void *operator new[]    (malloc_size_t, void*);
void  operator delete   (void*, void*);
void  operator delete[] (void*, void*);
