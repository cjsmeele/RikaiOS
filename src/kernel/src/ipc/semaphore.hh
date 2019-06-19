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

// TODO: -> limits.hh
static constexpr size_t semaphore_max_waiting = 32;

struct semaphore_t;

void signal    (semaphore_t &sem); ///< Increment.
void signal_all(semaphore_t &sem); ///< Increment until 0.
void wait      (semaphore_t &sem); ///< Decrement or block.
bool try_wait  (semaphore_t &sem); ///< Decrement or return false.

/**
 * Semaphore.
 *
 * (TODO: documentation)
 */
struct semaphore_t {

    int i = 0;

    Queue<tid_t, semaphore_max_waiting> waiting { };

    void signal()     {        ::signal    (*this); }
    void signal_all() {        ::signal_all(*this); }
    void wait  ()     {        ::wait      (*this); }
    bool try_wait()   { return ::try_wait  (*this); }
};

/// A mutex is a binary semaphore.
struct mutex_t {
    semaphore_t sem { 1, {} };

    void     lock() {            wait(sem); }
    bool try_lock() { return try_wait(sem); }
    void   unlock() {          signal(sem); }
};

inline void mutex_lock    (mutex_t &mut) {            wait(mut.sem); }
inline bool mutex_try_lock(mutex_t &mut) { return try_wait(mut.sem); }
inline void mutex_unlock  (mutex_t &mut) {          signal(mut.sem); }

/// Use RAII to automatically unlock a mutex on scope exit.
struct locked_within_scope {
    mutex_t &mut;

    locked_within_scope(const locked_within_scope& ) = delete;
    locked_within_scope(      locked_within_scope&&) = delete;

     locked_within_scope(mutex_t &m) : mut(m) { mut.lock();   }
    ~locked_within_scope()                    { mut.unlock(); }
};
