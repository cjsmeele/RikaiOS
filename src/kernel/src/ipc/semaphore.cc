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
#include "semaphore.hh"
#include "interrupt/interrupt.hh"
#include "process/proc.hh"

using namespace Process;

// Note: Interrupts are disabled in kernel code. This makes our lives easier.

void signal(semaphore_t &sem) {

    // Thread id and struct of the thread that we are unblocking.
    tid_t     tid;
    thread_t *thread = nullptr;

    // Find a thread to unblock.
    do {
        if (!sem.waiting.dequeue(tid))
            break;
        thread = thread_by_tid(tid);
    } while (!thread);

    if (thread) {
        // Do we have a thread to unblock?
        unblock(*thread);
    } else {
        // No, mark availability.
        sem.i++;
    }
}

void signal_all(semaphore_t &sem) {

    tid_t     tid;
    thread_t *thread = nullptr;

    while (sem.waiting.dequeue(tid)) {
        thread = thread_by_tid(tid);
        if (thread)
            unblock(*thread);
    }
}

void wait(semaphore_t &sem) {

    if (sem.i > 0) {
        // Something's available right now, move on.
        sem.i--;

    } else {
        // We need to wait.
        if (!sem.waiting.enqueue(current_thread()->id))
            panic("semaphore waiting queue exceeded capacity while adding thread {}"
                 ,*current_thread());

        block();

        // Wait is done!
    }
}

bool try_wait(semaphore_t &sem) {

    if (sem.i > 0) {
        sem.i--;
        return true;
    } else {
        return false;
    }
}
