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
#include "../memory/manager-virtual.hh"
#include "../interrupt/frame.hh"

/// Max length of a process name.
static constexpr size_t max_proc_name   = 32;

/// Max amount of semaphores a thread can wait on simultaneously.
//static constexpr size_t max_thread_sems = 32;

static constexpr size_t per_thread_kernel_stack_size = 8_KiB;

/*
 * Interactions of interrupt handling with scheduling:
 *
 * user thread           -> exc     -> [kill thread]         -> other thread
 * user thread           -> syscall -> kernel half           -> user  thread
 * user thread           -> syscall -> kernel half [block]   -> other thread
 * user thread           -> syscall -> kernel half [exit]    -> other thread
 * user thread           -> irq     -> kernel half           -> user  thread
 * user thread           -> irq     -> kernel half [preempt] -> other thread
 * kernel thread         -> irq     -> kernel thread         -> other thread
 * kernel thread         -> irq     -> kernel half [preempt] -> other thread
 * kernel thread [block] -> other thread
 * kernel thread [yield] -> other thread
 * kernel thread         -> exc     -> (panic)
 * kernel thread         -> exc PF  -> [alloc kheap]         -> kernel thread
 */

namespace Process {

    /// How many timer ticks a process is allowed to run before it is pre-empted.
    static constexpr size_t ticks_per_slice = 4;

    static constexpr size_t max_proc_files = 32;

    struct proc_t;
    struct thread_t {

        proc_t *proc             = nullptr; ///< The thread's process.
        tid_t   id               = 0;       ///< The thread's global ID.

        String<max_proc_name> name;         ///< The name of the thread (may be empty).

        u64 ticks_running        = 0;       ///< How many clock ticks this thread has been running.

        bool started             = false;   ///< Whether the thread was dispatched at least once.
        bool suspended_in_kernel = false;   ///< Whether the thread was suspended within kernel-mode.
        bool blocked             = false;   ///< Whether the thread is waiting on something.
        bool is_kernel_thread    = false;   ///< Whether this thread only runs in kernel-mode.

        thread_t *prev_in_proc   = nullptr; ///< Points to another thread within the same proc.
        thread_t *next_in_proc   = nullptr; ///< Points to another thread within the same proc.
        thread_t *next_ready     = nullptr; ///< Points to the next thread in the ready queue.

        Interrupt::interrupt_frame_t frame; ///< Stores thread state when it's interrupted.
        Memory::Virtual::PageDir *page_dir; ///< Stores the thread's address space.

        u32 *stack_bottom        = nullptr; ///< The lower bound of the thread's stack.

        /// A thread in blocking state (whether a user or kernel thread) will
        /// have saved kernel-mode register state at this location (which is
        /// always somewhere in the kernel_stack, see below).
        /// Popping from here in kernel mode will facilitate a resumption of the
        /// thread (straight into the kernel-mode code that called the block).
        ///
        /// \see Process::block()
        u32 *kernel_resume_esp   = nullptr;

        /**
         * Per-thread kernel stack.
         *
         * Since any amount of threads must be able to make (blocking) syscalls
         * simultaneously, we need a separate kernel stack for every one of
         * them.
         */
        Array<u32, per_thread_kernel_stack_size/sizeof(u32)> kernel_stack;
    };

    struct proc_t {

        pid_t id = 0;                     ///< The process' global ID.

        // This linked list contains all threads within this process.
        thread_t *first_thread = nullptr; ///< Points to a thread belonging to this process.
        thread_t  *last_thread = nullptr; ///< Points to a thread belonging to this process.

        // This linked list contains all processes.
        proc_t *prev = nullptr;           ///< Points to a different process.
        proc_t *next = nullptr;           ///< Points to a different process.

        String<max_proc_name> name;       ///< The name of this process (cannot be empty).

        /// Open files.
        // Array<fd_t, max_proc_files> files;
    };


    thread_t *current_thread(); ///< Gets the currently running thread.
    proc_t   *current_proc();   ///< Gets the currently running process.

    proc_t   *process_by_pid(pid_t pid); /// Find a process by its PID.
    thread_t * thread_by_tid(tid_t tid); /// Find a thread  by its TID.

    // For the following functions, `block` indicates whether the current
    // thread should be added to the ready queue or not.
    // Note that blocking without waiting on a semaphore (or the like) will
    // effectively freeze a thread forever.

    /// Yields current thread, resumes the saved frame on reschedule.
    /// This does not return to the caller on reschedule - it will return straight
    /// back to interrupted code (therefore this should only be used within an ISR).
    [[noreturn]]
    void yield_noreturn(bool block = false);

    /// Yields current thread, resumes the calling kernel function on reschedule.
    void yield(bool block = false);

    /// Block current thread, preventing reschedule until someone unblock()s it.
    inline void block() {
        yield(true);
    }

    /// Unblocks a thread, adding it to the ready queue.
    void unblock(thread_t &t);

    void dump_ready_queue();
    void dump_all();

    /**
     * Create a kernel thread.
     */
    thread_t *make_kernel_thread(function_ptr<void()> entrypoint, StringView name);

    bool scheduler_enabled();

    /**
     * Disable scheduling.
     *
     * This immediately dispatches the idle thread, disallowing task switches
     * until scheduling is resumed and effectively freezing the machine, while
     * still allowing kernel debug keys to work.
     */
    [[noreturn]]
    void pause();

    /// Re-enable scheduling.
    void resume();

    /// Save the given frame as the resumption frame of the current thread.
    void save_frame(const Interrupt::interrupt_frame_t &frame);

    /// Starts the scheduler, dispatches the idle thread.
    [[noreturn]]
    void run();

    void init();
}

namespace ostd::Format {

    /// Formatter for threads.
    template<typename F>
    constexpr int format(F &print, Flags, const Process::thread_t &thread) {
        return fmt(print, "{}:{}:{}.{}"
                  ,thread.proc->id
                  ,thread.id
                  ,thread.proc->name
                  ,thread.name);
    }

    /// Formatter for processes.
    template<typename F>
    constexpr int format(F &print, Flags, const Process::proc_t &proc) {
        return fmt(print, "{}:{}"
                  ,proc.id
                  ,proc.name);
    }
}
