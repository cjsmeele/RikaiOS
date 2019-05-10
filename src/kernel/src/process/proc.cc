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
#include "proc.hh"
#include "idle.hh"
#include "../interrupt/interrupt.hh"
#include "../interrupt/frame.hh"
#include "../memory/manager-virtual.hh"

// Assembly functions that assist in saving and restoring register & stack
// state for threads waiting in kernel-mode.
extern "C" [[gnu::naked]]           void suspend_in_kernel();
extern "C" [[            noreturn]] void suspend_2        (u32 *esp);
extern "C" [[gnu::naked, noreturn]] void resume_in_kernel (u32 *esp);

// Other assembly functions.
extern "C" [[noreturn]] void delete_running_thread();
extern "C" [[noreturn]] void delete_running_thread_2();

namespace Process {

    // When disabled, only the idle thread is allowed to run.
    bool scheduler_enabled_ = false;
    bool scheduler_enabled() { return scheduler_enabled_; }

    /// The current thread - will never be null once the first thread is dispatched.
    thread_t *current_thread_ = nullptr;
    thread_t *current_thread() { return current_thread_; }
    proc_t   *current_proc()   { return current_thread_
                                      ? current_thread_->proc
                                      : nullptr; }

    /// The kernel idle thread (TID 0).
    thread_t *idle_thread = nullptr;

    /// Process (PID 0) for all kernel threads.
    proc_t    kernel_proc;

    /// The front of the ready queue (may be null when there is nothing to do but idle).
    thread_t *ready_first = nullptr;
    /// The back  of the ready queue (may be null when there is nothing to do but idle).
    thread_t *ready_last  = nullptr;

    /// When userspace is paused (only kthreads can run), userspace threads are
    /// split off the ready queue into this queue.
    thread_t *paused_userspace_queue = nullptr;

    bool userspace_paused = false;

    atomic<size_t> thread_count  = 0;
    atomic<size_t> process_count = 0;

    /// Checks whether the thread's saved state indicates that it runs in kernel mode.
    static bool is_thread_in_kernel_mode(const thread_t &t) {
        // Check whether the code segment of the thread is in ring 0.
        return (t.frame.sys.cs & 0x3) == 0;
    }

    /// (inefficiently) find a process by its PID.
    proc_t   *process_by_pid(pid_t pid) {
        for (proc_t *p = &kernel_proc
            ;p
            ;p = p->next) {

            if (p->id == pid)
                return p;
        }
        return nullptr;
    }

    /// (inefficiently) find a thread by its TID.
    thread_t *thread_by_tid(tid_t tid) {

        for (proc_t *p = &kernel_proc
            ;p
            ;p = p->next) {

            for (thread_t *t = p->first_thread
                ;t
                ;t = t->next_in_proc) {

                if (t->id == tid)
                    return t;
            }
        }

        return nullptr;
    }

    /// Pop the next thread from the ready queue.
    /// If no such thread exists, returns null.
    static thread_t *dequeue() {

        if (ready_first) {
            // There is a thread in the ready queue.
            thread_t *next = ready_first;

            ready_first = next->next_ready;
            if (ready_first)
                ready_first->prev_ready = nullptr;

            // Break the link.
            next->prev_ready = nullptr;
            next->next_ready = nullptr;

            // Update ready_last.
            if (ready_last == next)
                ready_last = nullptr;

            return next;
        } else {
            return nullptr;
        }
    }

    /// Add a thread to the ready queue.
    static void enqueue(thread_t &t) {

        enter_critical_section();

        if (userspace_paused && !t.is_kernel_thread) {
            // userspace is paused, add this thread to the paused queue instead.
            if (paused_userspace_queue) {
                t.next_ready = paused_userspace_queue;
                t.prev_ready = nullptr;
                paused_userspace_queue->prev_ready = &t;
            } else {
                t.next_ready = nullptr;
                t.prev_ready = nullptr;
            }
            paused_userspace_queue = &t;
        } else {
            t.prev_ready = ready_last;

            if (ready_last)   ready_last->next_ready = &t;
            if (!ready_first) ready_first            = &t;

            ready_last  = &t;
        }

        t.blocked = false;
        leave_critical_section();
    }

    /// Add a thread to the front of the ready queue.
    static void enqueue_front(thread_t &t) {

        enter_critical_section();

        if (!t.is_kernel_thread && userspace_paused) {
            if (paused_userspace_queue) {
                t.next_ready = paused_userspace_queue;
                t.prev_ready = nullptr;
                paused_userspace_queue->prev_ready = &t;
            } else {
                t.next_ready = nullptr;
                t.prev_ready = nullptr;
            }
            paused_userspace_queue = &t;
        } else {
            t.next_ready = ready_first;

            if (ready_first) ready_first->prev_ready = &t;
            if (!ready_last) ready_last = &t;

            ready_first  = &t;
        }

        t.blocked = false;
        leave_critical_section();
    }

    /// Start running / resume a thread.
    [[noreturn]]
    static void dispatch(thread_t &thread) {

        // We definitely do not want to be interrupted during dispatch.
        enter_critical_section();

        // kprint("dispatch to thread {} (current: {})\n", thread, current_thread_ ? current_thread_->name : "NONE");
        // kprint("dispatch to thread {} @{} with frame@{}: {}\n"
        //        , thread
        //        ,&thread
        //        ,&thread.frame
        //        ,thread.frame);

        if (&Memory::Virtual::current_dir() != thread.page_dir) {
            // This thread lives in a different address space than the current
            // thread, se we switch to it first.

            // Kernel threads are excluded - they are mapped in all address spaces.
            if (!thread.is_kernel_thread)
                Memory::Virtual::switch_address_space(*thread.page_dir);
        }

        thread_t *old_thread = current_thread_;

        // Update active thread.
        current_thread_ = &thread;
        current_thread_->ticks_running++;

        if (old_thread
        && !old_thread->blocked
        &&  old_thread != &thread
        &&  old_thread != idle_thread) {

            // We are switching away from a thread that isn't blocked.
            // Make sure to re-enter it in the ready queue.
            enqueue(*old_thread);
        }

        // // Force-disable interrupts (for testing).
        // thread.frame.sys.eflags &= ~(1 << 9);

        // Was the thread suspended using yield() or block() within kernel code?

        if (thread.suspended_in_kernel) {
            thread.suspended_in_kernel = false;

            // YES: Easy mode: We only need to act as if we are returning from
            //                 that function call.

            // kprint("RESUME esp {}\n", thread.kernel_resume_esp);
            // hex_dump(thread.kernel_resume_esp, 64, true);

            resume_in_kernel(thread.kernel_resume_esp);

            UNREACHABLE

        } else {
            // NO: Hard mode: The thread must be resumed using an interrupt frame.
        }

        // We will now decide where on the thread's kernel-mode stack we place
        // the interrupt frame, and then enter it using an IRET (return from
        // interrupt).

        if (thread.started) {

            // The thread was interrupted through a software or hardware interrupt.

            // Strategy: We re-use the stack space that was eaten by the ISR that
            // ran on the thread previously. We copy the frame into this space
            // and return to that state.
            //
            // frame.regs.esp is the stack pointer that was saved half-way into the ISR:
            // after the CPU has pushed the EIP, CS, EFLAGS, error_code and
            // possibly user-ESP, user-SS; and after the ISR has pushed an int_no.
            // (right before the PUSHA instruction)
            //
            // First, we pop off this partial frame:

            thread.frame.regs.esp += 5*4;

            // For user-mode threads, pop off user-esp and user-ss as well.
            if (!is_thread_in_kernel_mode(thread))
                thread.frame.regs.esp += Interrupt::frame_size_user
                                       - Interrupt::frame_size_kernel;

        } else {
            // This is the first dispatch for this thread!
            // There's no partial interrupt frame to pop off, so we simply push
            // our frame at the top of the stack.
            thread.started = true;
        }

        // Now we copy the frame onto the thread's stack and IRET into it.

        size_t frame_size = is_thread_in_kernel_mode(thread)
                          ? Interrupt::frame_size_kernel
                          : Interrupt::frame_size_user;

        asm volatile ("/* Switch to the thread's stack at isr time */              \
                    \n mov %0, %%esp                                               \
                    \n mov %0, %%edi                                               \
                    \n sub %3, %%edi                                               \
                    \n mov %1, %%esi                                               \
                    \n /* Copy the frame onto the thread's stack */                \
                    \n rep movsd                                                   \
                    \n sub %3, %%esp                                               \
                    \n /* Perform a return from interrupt on the thread's stack */ \
                    \n pop %%ss                                                    \
                    \n pop %%gs                                                    \
                    \n pop %%fs                                                    \
                    \n pop %%es                                                    \
                    \n pop %%ds                                                    \
                    \n /* Note: this popa does *not* restore esp */                \
                    \n popal                                                       \
                    \n /* Skip over int_no and error_code */                       \
                    \n add $8, %%esp                                               \
                    \n iret"
                   ::"d" (thread.frame.regs.esp)
                    ,"b" (&thread.frame)
                    ,"c" (frame_size / 4)
                    ,"a" (frame_size));

        UNREACHABLE
    }

    void unblock(thread_t &t) {

        enter_critical_section();

        if (t.blocked) {
            // This thread probably has something important to do, so push it
            // to the front of the queue.
            enqueue_front(t);
        }

        leave_critical_section();
    }

    [[noreturn]]
    static void dispatch_next_thread() {

        enter_critical_section();

        if (!scheduler_enabled_)
            // Disregard the ready queue, run the idle thread.
            dispatch(*idle_thread);

        thread_t *next = dequeue();

        if (next) {
            // There is a thread in the ready queue, run it.
            dispatch(*next);

        } else {
            // No ready thread - keep running the current if possible,
            // otherwise, run the idle thread.
            if (current_thread_ && !current_thread_->blocked)
                 dispatch(*current_thread_);
            else dispatch(*idle_thread);
        }
    }

    void pause_userspace() {

        enter_critical_section();

        if (userspace_paused) {
            leave_critical_section();
            return;
        }

        userspace_paused       = true;
        paused_userspace_queue = nullptr;

        thread_t *last_paused  = nullptr;

        if (!current_thread_) {
            leave_critical_section();
            return;
        }

        for (thread_t *t = ready_first
            ;t
            ;t = t->next_ready) {

            if (!t->is_kernel_thread)
                UNIMPLEMENTED
        }

        leave_critical_section();
    }

    void resume_userspace() {

        enter_critical_section();

        if (userspace_paused) {

            userspace_paused = false;

            if (paused_userspace_queue) {
                // Move threads from the paused_userspace_queue back to the ready queue.
                enqueue(*paused_userspace_queue);
                paused_userspace_queue = nullptr;
            }
        }

        leave_critical_section();
    }

    void save_frame(const Interrupt::interrupt_frame_t &frame) {
        if (current_thread_) current_thread_->frame = frame;
    }

    [[noreturn]]
    void yield_noreturn(bool block) {

        // When this thread is resumed, make it return directly to the
        // interrupted code instead of returning to yield's caller.

        enter_critical_section();

        if (block) current_thread_->blocked = true;

        dispatch_next_thread();

        UNREACHABLE
    }

    void yield(bool block) {

        enter_critical_section();

        if (block) current_thread_->blocked = true;

        // Enter suspend.
        suspend_in_kernel();

        // We've been resumed

        leave_critical_section();
    }

    static tid_t genereate_thread_id() {
        // XXX: This is silly, but it works as long as we have fewer than 2^31
        //      threads over the running time of the OS.
        //      We definitely need to fix this if we want the OS to be able to
        //      run for longer amounts of time.
        return thread_count++;
    }

    static pid_t genereate_process_id() {
        // XXX: See above.
        return process_count++;
    }

    /// Entrypoint for all newly created threads.
    [[gnu::naked]]
    static void threadpoline() {
        asm volatile ("/* Save thread pointer */                              \
                    \n push %%ecx                                             \
                    \n /* Call entrypoint with arg */                         \
                    \n push %%ebx                                             \
                    \n call *%%eax                                            \
                    \n add $4, %%esp                                          \
                    \n xchg %%bx, %%bx                                        \
                    \n /* Call delete_thread with the saved thread pointer */ \
                    \n call (%0)"
                    :: "m" (delete_thread));
    }

    thread_t *make_kernel_thread(function_ptr<void()> entrypoint, StringView name) {
        // eww.
        return make_kernel_thread(reinterpret_cast<function_ptr<void(int)>>(entrypoint), name, 0);
    }

    thread_t *make_kernel_thread(function_ptr<void(int)> entrypoint, StringView name, int arg) {

        enter_critical_section();

        kprint("proc: spawning kernel thread '{}'\n", name);

        thread_t *t         = new thread_t;
        assert(t, "could not allocate kernel thread");

        t->id               = genereate_thread_id();
        t->name             = name;
        t->proc             = &kernel_proc;
        t->is_kernel_thread = true;

        t->stack_bottom     = t->kernel_stack.data();
        t->frame.regs.esp   = (addr_t)(t->kernel_stack.data() + t->kernel_stack.size());

        // Enable interrupts for the new kernel thread.
        // (without touching reserved flags)
        t->frame.sys.eflags = (asm_eflags() & 0xffc0'802a) | 1 << 9;

        // Set segment registers.
        t->frame.sys. cs    = asm_cs();
        t->frame.regs.ss    = asm_ss();
        t->frame.regs.ds    = asm_ds();
        t->frame.regs.es    = asm_es();
        t->frame.regs.fs    = asm_fs();
        t->frame.regs.gs    = asm_gs();

        // Give context to the threadpoline.
        t->frame.regs.eax   = (addr_t)entrypoint;
        t->frame.regs.ebx   = arg;
        t->frame.regs.ecx   = (addr_t)t;
        t->frame.sys.eip    = (addr_t)threadpoline;

        if (kernel_proc.last_thread) {
            kernel_proc.last_thread->next_in_proc = t;
            t->prev_in_proc = kernel_proc.last_thread;
            kernel_proc.last_thread = t;

        } else {
            kernel_proc.first_thread = t;
            kernel_proc. last_thread = t;
        }

        t->started    = false;
        t->next_ready = nullptr;

        enqueue(*t);

        leave_critical_section();

        return t;
    }

    void delete_thread(thread_t *t) {

        enter_critical_section();

        if (t == idle_thread)
            panic("someone tried to kill the idle thread - please don't, i need that!");

        if (!t->next_in_proc && !t->prev_in_proc) {

            // Delete an entire process.
            UNIMPLEMENTED
        }

        // Update linked lists.
        if (t->next_ready)   t->next_ready  ->prev_ready   = t->prev_ready;
        if (t->prev_ready)   t->prev_ready  ->next_ready   = t->next_ready;
        if (t->next_in_proc) t->next_in_proc->prev_in_proc = t->prev_in_proc;
        if (t->prev_in_proc) t->prev_in_proc->next_in_proc = t->next_in_proc;

        if (ready_last  == t) ready_last  = t->prev_ready;
        if (ready_first == t) ready_first = t->next_ready;

        if (t == current_thread_) {
            // We are deleting the currently running thread.
            // This is a bit more involved.
            delete_running_thread();

            UNREACHABLE

        } else {
            // Simply de-allocate and move on.
            delete t; // *poof*
        }

        leave_critical_section();
    }

    void resume() {
        scheduler_enabled_ = true;
    }

    [[noreturn]]
    void pause() {

        enter_critical_section();

        scheduler_enabled_ = false;

        // Re-enter idle thread immediately.
        dispatch(*idle_thread);
    }

    void dump_ready_queue() {
        kprint("scheduler ready queue:\n");
        kprint("  {-4} {-4} {1} {8} {}\n", "PID", "TID", "S", "TICKS", "NAME");

        for (thread_t *t = ready_first
            ;t
            ;t = t->next_ready) {

            kprint("  {4} {4} {} {8} {}{}{}\n"
                  ,t->proc->id
                  ,t->id
                  ,t == current_thread_
                    ? 'R'
                    : t->blocked
                    ? 'B'
                    : t->suspended_in_kernel
                    ? 's'
                    : 'S'
                  ,t->ticks_running
                  ,t->proc->name
                  ,t->name.length() ? "." : ""
                  ,t->name);
        }
    }

    void dump_all() {
        kprint("  {-4} {-4} {1} {8} {}\n", "PID", "TID", "S", "TICKS", "NAME");
        for (proc_t *p = &kernel_proc
            ;p
            ;p = p->next) {

            for (thread_t *t = p->first_thread
                ;t
                ;t = t->next_in_proc) {

                kprint("  {4} {4} {1} {8} {}{}{}\n"
                      ,p->id
                      ,t->id
                      ,t == current_thread_
                        ? 'R'
                        : t->blocked
                        ? 'B'
                        : t->suspended_in_kernel
                        ? 's'
                        : 'S'
                      ,t->ticks_running
                      ,p->name
                      ,t->name.length() ? "." : ""
                      ,t->name);
            }
        }
    }

    [[noreturn]]
    void run() {
        scheduler_enabled_ = true;
        dispatch(*idle_thread);
    }

    void init() {
        kernel_proc.id   = 0;
        kernel_proc.name = "kernel";

        idle_thread = make_kernel_thread(idle, "idle");
    }
}

/// Suspends a thread running in kernel-mode.
/// Returns to the caller on resumption.
extern "C" [[gnu::naked]]
void suspend_in_kernel() {
    asm volatile ("cli                             \
                \n pushfl                          \
                \n pushal                          \
                \n push %%ds                       \
                \n push %%es                       \
                \n push %%fs                       \
                \n push %%gs                       \
                \n push %%esp                      \
                \n call suspend_2"
                :: "m" (suspend_2));
}

/// Second part of suspend_in_kernel().
extern "C" [[noreturn]]
void suspend_2(u32 *kernel_resume_esp) {
    // kprint("suspend esp {}\n", kernel_resume_esp);
    // hex_dump(kernel_resume_esp, 64, true);

    Process::current_thread_->kernel_resume_esp   = kernel_resume_esp;
    Process::current_thread_->suspended_in_kernel = true;
    Process::dispatch_next_thread();
}

/// Return from a suspend_in_kernel().
extern "C" [[gnu::naked, noreturn]]
void resume_in_kernel(u32*) {
    asm volatile ("cli                                                        \
                \n /* Switch to the stack that was provided as an argument */ \
                \n mov 4(%%esp), %%esp                                        \
                \n pop %%gs                                                   \
                \n pop %%fs                                                   \
                \n pop %%es                                                   \
                \n pop %%ds                                                   \
                \n popal                                                      \
                \n popfl                                                      \
                \n ret" ::);
}

extern "C" [[noreturn]]
void delete_running_thread() {
    // Destroying the running thread including its stack while we are still
    // running on it probably works, but is a little irresponsible.
    //
    // Instead we create a (static) stack that we switch to right
    // before deleting ourselves, for the single purpose of dispatching
    // the next thread.
    //
    // Switching stacks in the middle of a function is a bit too tricky to do
    // in pure C++, unfortunately.

    static Array<u8, 256> temp_stack;

    asm volatile ("mov %0, %%esp \
                \n call delete_running_thread_2"
                :: "r" (temp_stack.data() + temp_stack.size()));

    UNREACHABLE
}

extern "C" [[noreturn]]
void delete_running_thread_2() {
    delete Process::current_thread_;
    Process::current_thread_ = nullptr;

    // It may be too late to write a testament, but we can still
    // appoint a thread to inherit the CPU from us.
    Process::dispatch_next_thread();
}
