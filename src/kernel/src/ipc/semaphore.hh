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
#include "process/proc.hh"

static constexpr size_t semaphore_max_waiting = 32;

struct semaphore_t;

void signal  (semaphore_t &sem); ///< Increment.
void wait    (semaphore_t &sem); ///< Decrement or block.
bool try_wait(semaphore_t &sem); ///< Decrement or return false.

struct semaphore_t {

    atomic<int> i = 0;

    Queue<tid_t, semaphore_max_waiting> waiting { };

    void signal()   {        ::signal  (*this); }
    void wait  ()   {        ::wait    (*this); }
    bool try_wait() { return ::try_wait(*this); }
};

struct mutex_t {
    // A mutex is a binary semaphore.
    semaphore_t sem { 1, {} };

    void   lock() {   wait(sem); }
    void unlock() { signal(sem); }
};

inline void mutex_lock  (mutex_t &mut) { wait  (mut.sem); }
inline void mutex_unlock(mutex_t &mut) { signal(mut.sem); }
