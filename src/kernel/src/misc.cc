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
#include <os-std/types.hh>
#include "common/new-delete.hh"
#include "memory/kernel-heap.hh"

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

/// This isn't necessary since the kernel never exits.
extern "C" void __cxa_atexit(void (*)(void*), void*, void*);
extern "C" void __cxa_atexit(void (*)(void*), void*, void*) { }

/// \name These are required by compiler builtins.
///@{
extern "C" void *memcpy(void *__restrict dst, const void *__restrict src, malloc_size_t n) {
    for (size_t i = 0; i < n; ++i)
        ((u8*)dst)[i] = ((u8*)src)[i];
    return dst;
}

extern "C" void *memset(void *s, int c, malloc_size_t n) {
    for (size_t i = 0; i < n; ++i)
        ((char*)s)[i] = c;
    return s;
}
///@}

/// \name C++ memory allocation operators.
///@{

// Basic new and new[].
void *operator new      (malloc_size_t size) { return Memory::Heap::alloc(size, 4); }
void *operator new[]    (malloc_size_t size) { return Memory::Heap::alloc(size, 4); }

// Basic delete and delete[].
void  operator delete  (void *ptr)                { Memory::Heap::free(ptr); }
void  operator delete[](void *ptr)                { Memory::Heap::free(ptr); }

// "sized" delete, for possibly more efficient deallocations.
// We don't care - we keep track of the allocated size ourselves.
void  operator delete  (void *ptr, malloc_size_t) { Memory::Heap::free(ptr); }
void  operator delete[](void *ptr, malloc_size_t) { Memory::Heap::free(ptr); }

// Aligned new. Returns pointers of the given alignment.
// (currently only supports chaotic neutral alignment)
void *operator new(malloc_size_t size, std::align_val_t align) {
    return Memory::Heap::alloc(size, static_cast<malloc_size_t>(align));
}

// Placement new and delete.
// These construct objects in-place without allocating memory.
void *operator new      (malloc_size_t, void *p) { return p; }
void *operator new[]    (malloc_size_t, void *p) { return p; }
void  operator delete   (void*, void*) { }
void  operator delete[] (void*, void*) { }

///@}
