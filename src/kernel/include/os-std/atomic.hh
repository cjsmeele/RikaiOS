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

namespace ostd {

#ifndef __GNUC__
    #error "Need alternative atomic builtin implementations since this is not GCC/Clang"
#endif

    constexpr inline int atomic_default_order = __ATOMIC_SEQ_CST;

    inline void atomic_fence()  { __atomic_thread_fence(atomic_default_order); }
    // inline void mfence() { asm volatile ("mfence"); }

    template<typename T> constexpr T atomic_add_fetch(T &x, T y, int order = atomic_default_order) { return __atomic_add_fetch(&x, y, order); }
    template<typename T> constexpr T atomic_fetch_add(T &x, T y, int order = atomic_default_order) { return __atomic_fetch_add(&x, y, order); }
    template<typename T> constexpr T atomic_sub_fetch(T &x, T y, int order = atomic_default_order) { return __atomic_sub_fetch(&x, y, order); }
    template<typename T> constexpr T atomic_fetch_sub(T &x, T y, int order = atomic_default_order) { return __atomic_fetch_sub(&x, y, order); }
    template<typename T> constexpr T atomic_compare_exchange(T &x
                                                            ,T &expected
                                                            ,T y
                                                            ,int order_success = atomic_default_order
                                                            ,int order_fail    = atomic_default_order)
                                                            { return __atomic_compare_exchange(&x, &expected, &y, false, order_success, order_fail); }

    template<typename T> constexpr void atomic_load (const T &x, T &y, int order = atomic_default_order) { __atomic_load (&x, &y, order); }
    template<typename T> constexpr void atomic_store(T &x, const T &y, int order = atomic_default_order) { __atomic_store(&x, &y, order); }

    template<typename T>
    struct atomic {
        T val;

        static_assert(is_integral<T>::value || is_pointer<T>::value
                     ,"atomic<> currently only supported for intergral types and pointers");

        // enable_if<(is_integral<T>::value
        //         ||  is_pointer<T>::value), T>::type

        bool compare_exchange(T &expected, T desired) { return atomic_compare_exchange(val, expected, desired); }

        T load() const { T x; atomic_load (val, x); return x; }
        T store(T x)   {      atomic_store(val, x); return x; }

        T fetch_add(T x)  { return atomic_fetch_add(val, x); }
        T add_fetch(T x)  { return atomic_add_fetch(val, x); }
        T fetch_sub(T x)  { return atomic_fetch_sub(val, x); }
        T sub_fetch(T x)  { return atomic_sub_fetch(val, x); }

        T operator++()    { return atomic_add_fetch(val, T(1)); }
        T operator++(int) { return atomic_fetch_add(val, T(1)); }
        T operator--()    { return atomic_sub_fetch(val, T(1)); }
        T operator--(int) { return atomic_fetch_sub(val, T(1)); }

        T operator=(const atomic&) = delete;
        T operator=(T x)   { return store(x); }
        operator T() const { return load(); }

        atomic() = default;
        atomic(T v) : val(v) { }
        atomic(      atomic&&) = delete;
        atomic(const atomic& ) = delete;
    };
}
