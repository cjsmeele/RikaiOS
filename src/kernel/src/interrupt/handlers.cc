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
#include "handlers.hh"
#include "controller.hh"
#include "page-fault.hh"
#include "../memory/gdt.hh"
#include "../debug-keys.hh"
#include "../process/proc.hh"
#include "syscall.hh"

/**
 * State save/restore.
 *
 * All handlers jump to this block after pushing their interrupt number.
 *
 * Here we save execution state, call the common interrupt handler, and restore state.
 */
extern "C" [[gnu::naked]] void isr_common();
extern "C" [[gnu::naked]] void isr_common() {
    asm ("/* Save state */                           \
       \n pusha                                      \
       \n push %%ds                                  \
       \n push %%es                                  \
       \n push %%fs                                  \
       \n push %%gs                                  \
       \n push %%ss                                  \
       \n /* Set kernel data segment */              \
       \n mov %0, %%eax                              \
       \n mov %%eax, %%ds                            \
       \n mov %%eax, %%es                            \
       \n mov %%eax, %%fs                            \
       \n mov %%eax, %%gs                            \
       \n /* Push address of registers struct */     \
       \n push %%esp                                 \
       \n call common_interrupt_handler              \
       \n add $4, %%esp                              \
       \n /* Restore state */                        \
       \n pop %%ss                                   \
       \n pop %%gs                                   \
       \n pop %%fs                                   \
       \n pop %%es                                   \
       \n pop %%ds                                   \
       \n popa                                       \
       \n /* Pop error code and interrupt number. */ \
       \n add $8, %%esp                              \
       \n iret"
       ::"i" (Memory::Gdt::i_kernel_data*8));
}

constexpr Array<StringView, 0x20> exception_names
    /* 0x00 #DE */ { "Divide Error Exception"
    /* 0x01 #DB */ , "Debug Exception"
    /* 0x02     */ , "Non-Maskable Interrupt"
    /* 0x03 #BP */ , "Breakpoint Exception"
    /* 0x04 #OF */ , "Overflow Exception"
    /* 0x05 #BR */ , "BOUND Range Exceeded Exception"
    /* 0x06 #UD */ , "Invalid Opcode Exception"
    /* 0x07 #NM */ , "Device Not Available Exception"
    /* 0x08 #DF */ , "Double Fault Exception"
    /* 0x09     */ , "Coprocessor Segment Overrun"
    /* 0x0a #TS */ , "Invalid TSS Exception"
    /* 0x0b #NP */ , "Segment Not Present"
    /* 0x0c #SS */ , "Stack Fault Exception"
    /* 0x0d #GP */ , "General Protection Exception"
    /* 0x0e #PF */ , "Page-Fault Exception"
    /* 0x0f     */ , "Unknown Exception"
    /* 0x10 #MF */ , "FPU Floating-Point Error"
    /* 0x11 #AC */ , "Alignment Check Exception"
    /* 0x12 #MC */ , "Machine-Check Exception"
    /* 0x13 #XM */ , "SIMD Floating-Point Exception"
    /* 0x14     */ , "Unknown Exception"
    /* 0x15     */ , "Unknown Exception"
    /* 0x16     */ , "Unknown Exception"
    /* 0x17     */ , "Unknown Exception"
    /* 0x18     */ , "Unknown Exception"
    /* 0x19     */ , "Unknown Exception"
    /* 0x1a     */ , "Unknown Exception"
    /* 0x1b     */ , "Unknown Exception"
    /* 0x1c     */ , "Unknown Exception"
    /* 0x1d     */ , "Unknown Exception"
    /* 0x1e     */ , "Unknown Exception"
    /* 0x1f     */ , "Unknown Exception" };

namespace Interrupt::Handler {

    static Array<irq_handler_t,16> irq_handlers;

    void register_irq_handler(u8 irq, irq_handler_t handler) {
        if (irq >= irq_handlers.size())
            panic("tried to register isr for irq{} (>{})"
                 ,irq, irq_handlers.size()-1);

        if (irq_handlers[irq])
            // We do not support IRQ sharing.
            panic("tried to register 2 handlers for irq{}:\n"
                  " @{}, @{}"
                 ,irq_handlers[irq]
                 ,handler
                 ,irq);

        irq_handlers[irq] = handler;
    }

    /**
     * Exception handler.
     *
     * Called by the common interrupt handler for exceptions.
     * We don't really handle much yet, so most exception crash the machine
     * with a nice register dump.
     */
    static void handle_exception(Interrupt::interrupt_frame_t &frame) {
        using namespace Interrupt;

        static constexpr Array quotes {
            StringView("Aaaaaand it's gone.")
                      ,"Into the trash it goes..."
                      ,"Your rubber ducky needs a word with you."
                      ,"`M-x doctor' is there for you if you need help."
                      ,"I'm sorry, Dave. I'm afraid I can't do that."
                      ,"PC LOAD LETTER"
                      ,"Abort, Retry, Fail?"
                      ,"Set phasers to kill -9."
                      ,"You've been terminated."
                      ,"Je kan altijd nog BIM doen."
                      ,"Have you tried turning it off and on again?"
                      ,"Try putting `sudo' in front."
                      ,"This is fine..."
                      ,"'Tis but a scratch"
                      ,"It's just a flesh wound"
        };

        // See if we can handle page faults in a non-fatal way.
        if (frame.int_no == 0x0e && handle_pagefault(frame))
            // Ok!
            return;

        // In any other case, the running thread is killed.
        // If kernel code caused the problem, it's time to panic.

        bool is_kernel = frame.sys.cs == Memory::Gdt::i_kernel_code << 3;

        if (is_kernel) {
            // The kernel did an oopsie: Panic with a register dump.
            panic("{#02x}:{#08x} - {}\n\n{}\n{}\n"
                 ,frame.int_no
                 ,frame.error_code
                 ,frame.int_no < exception_names.size()
                    ? exception_names[frame.int_no]
                    : "???"
                 ,frame
                 ,quotes[rand()%quotes.size()]);
        } else {
            // A userland process violated the law. Kill the offender.
            // (note: should kill the entire process, not just the thread)
            kprint("\n*** VIOLATION ({#02x}:{#08x} - {})\n    by {}\nthread frame: {}\n"
                  ,frame.int_no
                  ,frame.error_code
                  ,frame.int_no < exception_names.size()
                    ? exception_names[frame.int_no]
                    : "???"
                  ,*Process::current_thread()
                  ,frame);

            Process::delete_thread(Process::current_thread());
        }

        UNREACHABLE
    }

    /**
     * Hardware interrupt handler.
     *
     * Called by the common interrupt handler for hardware interrupts.
     */
    static void handle_irq(Interrupt::interrupt_frame_t &frame) {
        using namespace Interrupt;

        // IRQ numbers are mapped starting at 0x20.
        u8 irq = frame.int_no - 0x20;

        // Hardware interrupts must be acknowledged.
        Controller::acknowledge_interrupt(frame.int_no);

        if (irq_handlers[irq]) {
            irq_handlers[irq](frame);
        } else {
            // Unknown interrupt number, inform the user.
            kprint("!!unk.irq{} ", irq);
        }
    }
}

/**
 * Common interrupt handler.
 *
 * All interrupts and exceptions pass through here.
 */
extern "C" void common_interrupt_handler(Interrupt::interrupt_frame_t &frame);
extern "C" void common_interrupt_handler(Interrupt::interrupt_frame_t &frame) {
    using namespace Interrupt;

    if (frame.int_no < 0x20) {
        // Check & handle exceptions first.
        Handler::handle_exception(frame);

        // If we are still here, then the exception was non-fatal to the
        // running thread. Return immediately.
        return;
    }

    // Other handlers may cause context switches.
    // Save the interrupt frame so that we can return to this thread.
    Process::save_frame(frame);

    if (frame.int_no < 0x30) {
        Handler::handle_irq(frame);
    } else if (frame.int_no == 0xca) {
        Syscall::handle_syscall(Process::current_thread()->frame);
        Process::dispatch(*Process::current_thread());

        UNREACHABLE
    }
}

namespace Interrupt::Handler {

    // Interrupt service routine definitions.
    //
    // We get rid of the boilerplate by using C macros.
    //
    // One tricky part is that some exceptions also push an error code.
    // In order to still be able to use a common interrupt handler routine, we
    // make all other handlers push a dummy error code.

    // For interrupts and exceptions.
#define I(n)                          \
    [[gnu::naked]] void isr_##n() {   \
        /* Push a dummy error code */ \
        asm ("push $0                 \
           \n push %0                 \
           \n jmp isr_common"         \
           ::"i" (n));                \
    }

    // For exceptions that push an error code.
#define E(n)                          \
    [[gnu::naked]] void isr_##n() {   \
        asm ("push %0                 \
           \n jmp isr_common"         \
           ::"i" (n));                \
    }

    // Exception handlers.

        I(0x00) I(0x01) I(0x02) I(0x03) I(0x04) I(0x05) I(0x06) I(0x07)
        E(0x08) I(0x09) E(0x0a) E(0x0b) E(0x0c) E(0x0d) E(0x0e) I(0x0f)
        I(0x10) E(0x11) I(0x12) I(0x13) I(0x14) I(0x15) I(0x16) I(0x17)
        I(0x18) I(0x19) I(0x1a) I(0x1b) I(0x1c) I(0x1d) I(0x1e) I(0x1f)

    // Interrupt handlers.

        I(0x20) I(0x21) I(0x22) I(0x23) I(0x24) I(0x25) I(0x26) I(0x27)
        I(0x28) I(0x29) I(0x2a) I(0x2b) I(0x2c) I(0x2d) I(0x2e) I(0x2f)
        I(0x30) I(0x31) I(0x32) I(0x33) I(0x34) I(0x35) I(0x36) I(0x37)
        I(0x38) I(0x39) I(0x3a) I(0x3b) I(0x3c) I(0x3d) I(0x3e) I(0x3f)
        I(0x40) I(0x41) I(0x42) I(0x43) I(0x44) I(0x45) I(0x46) I(0x47)
        I(0x48) I(0x49) I(0x4a) I(0x4b) I(0x4c) I(0x4d) I(0x4e) I(0x4f)
        I(0x50) I(0x51) I(0x52) I(0x53) I(0x54) I(0x55) I(0x56) I(0x57)
        I(0x58) I(0x59) I(0x5a) I(0x5b) I(0x5c) I(0x5d) I(0x5e) I(0x5f)
        I(0x60) I(0x61) I(0x62) I(0x63) I(0x64) I(0x65) I(0x66) I(0x67)
        I(0x68) I(0x69) I(0x6a) I(0x6b) I(0x6c) I(0x6d) I(0x6e) I(0x6f)
        I(0x70) I(0x71) I(0x72) I(0x73) I(0x74) I(0x75) I(0x76) I(0x77)
        I(0x78) I(0x79) I(0x7a) I(0x7b) I(0x7c) I(0x7d) I(0x7e) I(0x7f)
        I(0x80) I(0x81) I(0x82) I(0x83) I(0x84) I(0x85) I(0x86) I(0x87)
        I(0x88) I(0x89) I(0x8a) I(0x8b) I(0x8c) I(0x8d) I(0x8e) I(0x8f)
        I(0x90) I(0x91) I(0x92) I(0x93) I(0x94) I(0x95) I(0x96) I(0x97)
        I(0x98) I(0x99) I(0x9a) I(0x9b) I(0x9c) I(0x9d) I(0x9e) I(0x9f)
        I(0xa0) I(0xa1) I(0xa2) I(0xa3) I(0xa4) I(0xa5) I(0xa6) I(0xa7)
        I(0xa8) I(0xa9) I(0xaa) I(0xab) I(0xac) I(0xad) I(0xae) I(0xaf)
        I(0xb0) I(0xb1) I(0xb2) I(0xb3) I(0xb4) I(0xb5) I(0xb6) I(0xb7)
        I(0xb8) I(0xb9) I(0xba) I(0xbb) I(0xbc) I(0xbd) I(0xbe) I(0xbf)
        I(0xc0) I(0xc1) I(0xc2) I(0xc3) I(0xc4) I(0xc5) I(0xc6) I(0xc7)
        I(0xc8) I(0xc9) I(0xca) I(0xcb) I(0xcc) I(0xcd) I(0xce) I(0xcf)
        I(0xd0) I(0xd1) I(0xd2) I(0xd3) I(0xd4) I(0xd5) I(0xd6) I(0xd7)
        I(0xd8) I(0xd9) I(0xda) I(0xdb) I(0xdc) I(0xdd) I(0xde) I(0xdf)
        I(0xe0) I(0xe1) I(0xe2) I(0xe3) I(0xe4) I(0xe5) I(0xe6) I(0xe7)
        I(0xe8) I(0xe9) I(0xea) I(0xeb) I(0xec) I(0xed) I(0xee) I(0xef)
        I(0xf0) I(0xf1) I(0xf2) I(0xf3) I(0xf4) I(0xf5) I(0xf6) I(0xf7)
        I(0xf8) I(0xf9) I(0xfa) I(0xfb) I(0xfc) I(0xfd) I(0xfe) I(0xff)

#undef I
#undef E
}
