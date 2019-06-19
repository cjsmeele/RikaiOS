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

/**
 * \file
 * C++ wrappers for x86 assembler code.
 */

/**
 * \name Processor control.
 *
 * These are implemented as macros to prevent the compiler from generating
 * function calls for these.
 */
///@{
/// Wait for interrupts.
#define asm_hlt() asm volatile ("hlt")
/// Disable interrupts.
#define asm_cli() asm volatile ("cli")
/// Enable interrupts.
#define asm_sti() asm volatile ("sti")
/// Do nothing.
#define asm_nop() asm volatile ("nop")
///@}

/// Hang the CPU.
inline void asm_hang() { while(true) {asm_cli(); asm_hlt(); } }

/// Execute an invalid instruction, triggering an exception.
inline void crash() { asm volatile("ud2"); }

/// \name Functions for getting/setting special x86 registers.
///@{
inline u32  asm_esp()      { u32 x; asm volatile ("mov %%esp, %0"  : "=a"(x)); return x; }
inline u32  asm_eflags()   { u32 x; asm volatile ("pushf\n pop %0" : "=a"(x)); return x; }
inline u32  asm_cr0()      { u32 x; asm volatile ("mov %%cr0, %0"  : "=a"(x)); return x; }
inline u32  asm_cr2()      { u32 x; asm volatile ("mov %%cr2, %0"  : "=a"(x)); return x; }
inline u32  asm_cr3()      { u32 x; asm volatile ("mov %%cr3, %0"  : "=a"(x)); return x; }
inline u32  asm_cr4()      { u32 x; asm volatile ("mov %%cr4, %0"  : "=a"(x)); return x; }

inline void asm_cr0(u32 x) {        asm volatile ("mov %0, %%cr0" :: "b" (x) : "memory" ); }
inline void asm_cr3(u32 x) {        asm volatile ("mov %0, %%cr3" :: "b" (x) : "memory" ); }
inline void asm_cr4(u32 x) {        asm volatile ("mov %0, %%cr4" :: "b" (x) : "memory" ); }

inline u32  asm_cs()       { u32 x; asm volatile ("mov %%cs, %0"   : "=a"(x)); return x; }
inline u32  asm_ss()       { u32 x; asm volatile ("mov %%ds, %0"   : "=a"(x)); return x; }
inline u32  asm_ds()       { u32 x; asm volatile ("mov %%ds, %0"   : "=a"(x)); return x; }
inline u32  asm_es()       { u32 x; asm volatile ("mov %%ds, %0"   : "=a"(x)); return x; }
inline u32  asm_fs()       { u32 x; asm volatile ("mov %%ds, %0"   : "=a"(x)); return x; }
inline u32  asm_gs()       { u32 x; asm volatile ("mov %%ds, %0"   : "=a"(x)); return x; }
///@}

inline u64  asm_rdtsc()   { u32 hi, lo;
                            asm volatile ("rdtsc" : "=d" (hi), "=a"(lo));
                            return ((u64)hi << 32) | lo; }
inline u32  asm_rdtsc32() { u32 x;
                            asm volatile ("rdtsc" : "=a"(x) :: "rdx");
                            return x; }
inline u64  asm_rdtsc64() { return asm_rdtsc(); }

inline u32  asm_rdrand() {
    // (this unfortunately only works on fairly modern processors,
    //  qemu and bochs don't support it by default)
    u32 r;
    asm volatile ("1:  \
                \n rdrand %%eax \
                \n jnc 1"
                 :"=a" (r)
                ::"cc");
    return r;
}

inline void asm_invlpg(addr_t x) {
    asm volatile ("invlpg (%0)" :: "a" (x) : "memory");
}

/// Do nothing for n cycles (unreliable).
inline void spin(u64 n = 1) { for (u64 i = 0; i < n; ++i) asm volatile ("nop"); }

/// Functions for reading / writing to x86 IO ports.
namespace Io {
    inline void out_8 (u16 port,  u8 val) { asm volatile ("outb %0, %1" :: "a" (val),  "Nd" (port)); }
    inline void out_16(u16 port, u16 val) { asm volatile ("outw %0, %1" :: "a" (val),  "Nd" (port)); }
    inline void out_32(u16 port, u32 val) { asm volatile ("outl %0, %1" :: "a" (val),  "Nd" (port)); }

    inline  u8   in_8 (u16 port) {  u8 ret; asm volatile (" inb %1, %0" : "=a" (ret) : "Nd" (port)); return ret; }
    inline u16   in_16(u16 port) { u16 ret; asm volatile (" inw %1, %0" : "=a" (ret) : "Nd" (port)); return ret; }
    inline u32   in_32(u16 port) { u32 ret; asm volatile (" inl %1, %0" : "=a" (ret) : "Nd" (port)); return ret; }

    /// Performs a slow useless IO read operation to a achieve a few μs delay.
    inline void wait() { Io::in_8(0x80); }
    inline void wait(size_t n) {
        for (size_t i = 0; i < n; ++i)
            wait();
    }

    inline void out_8s (u16 port,  u8 val) { out_8 (port, val); wait(); }
    inline void out_16s(u16 port, u16 val) { out_16(port, val); wait(); }
    inline void out_32s(u16 port, u32 val) { out_32(port, val); wait(); }
}
