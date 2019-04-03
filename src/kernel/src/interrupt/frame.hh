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

#include "../common.hh"
#include "../memory/gdt.hh"

/**
 * \file
 * Datastructures used for saving and restoring processor state during interrupts.
 */

namespace Interrupt {

    /**
     * x86 interrupt frame.
     *
     * When an interrupt fires, the CPU saves execution state on the stack, in
     * the form of an instruction pointer, a code segment register, CPU flags and
     * a stack pointer + segment.
     *
     * We create a C++ datastructure so that we can easily access that state
     * from C++ code.
     *
     * These fields are pushed back-to-front.
     *
     * When an interrupt is fired in user-mode code (as opposed to kernel-mode
     * code), the cpu switches to a safe kernel stack (linked in the TSS)
     * before calling an interrupt handler. In this case, user_esp and user_ss
     * contain the original user-mode stack pointer and stack segment,
     * respectively.
     *
     * Note that user_ss and user_esp only exist when the interrupt is fired
     * during execution of a user-mode process - in all other cases they refer
     * to unrelated data on the kernel stack and should not be touched.
     */
    struct cpu_interrupt_frame_t { u32 eip, cs, eflags, user_esp, user_ss; };

    /**
     * x86 register state.
     *
     * The cpu_interrupt_frame_t pushed by the CPU is of course not the
     * complete processor state. In order for our C++ interrupt handling code
     * to be able to return control to the interrupted program/kernel code, we
     * need to manually save more state - the other registers, lest they be
     * overwritten.
     */
    struct registers_t {
        // These are pushed by hand.
        u32 ss, gs, fs, es, ds;
        // These are pushed by the PUSHA instruction.
        u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    };

    /**
     * The unified interrupt frame.
     *
     * Finally, we create a datastructure that encompasses all the above, and
     * adds an interrupt number field. This field is used by our common
     * interrupt handler to determine which interrupt number fired.
     *
     * The error code field is pushed by the CPU for some exception types.
     *
     * The code that actually sets these frames up resides in handlers.cc
     */
    struct interrupt_frame_t {
        registers_t regs;
        u32 int_no;
        u32 error_code;
        cpu_interrupt_frame_t sys;
    };
}

namespace ostd::Format {

    /// Formatter for interrupt frames.
    template<typename F>
    constexpr int format(F &print, Flags, const Interrupt::interrupt_frame_t &frame) {

        int count = 0;

        // Helper functions.
        auto g = [&] (StringView name, u32 v) {
            count += fmt(print, "  {-3} {08x}", name, v);
        };
        auto nl = [&] { print('\n'); count++; };

        // Was the interrupted code running in kernel-mode?
        bool is_kernel = frame.sys.cs == Memory::Gdt::i_kernel_code << 3;

        count += fmt(print, "{} was interrupted by {} {#02x}\n\n"
                          , is_kernel ? "kernel" : "user-mode"
                          ,   frame.int_no < 0x20
                            ? "exception"
                            : frame.int_no < 0x40
                            ? "IRQ"
                            : "software interrupt"
                          , frame.int_no);

        g("eax", frame.regs.eax); g("cs", frame.sys.cs);  g("eip", frame.sys.eip);  nl();
        g("ebx", frame.regs.ebx); g("ss", frame.regs.ss); g("esp", frame.regs.esp); nl();
        g("ecx", frame.regs.ecx); g("ds", frame.regs.ds); g("ebp", frame.regs.ebp); nl();
        g("edx", frame.regs.edx); g("es", frame.regs.es); g("cr0", asm_cr0());      nl();
        g("esi", frame.regs.esi); g("fs", frame.regs.fs); g("cr2", asm_cr2());      nl();
        g("edi", frame.regs.edi); g("gs", frame.regs.gs); g("cr3", asm_cr3());      nl();

        g("eflags", frame.sys.eflags); nl();

        if (!is_kernel) {
            // User-mode stack pointer/segment are only valid if the
            // interrupted code was running in user mode, avoid printing it
            // otherwise.

            count += fmt(print, "\nuser-mode stack:\n");

            g("esp", frame.sys.user_esp); g("ss", frame.sys.user_ss); nl();
        }

        return count;
    }
}
