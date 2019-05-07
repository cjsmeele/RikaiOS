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

    /**
     * \name Global iterator begin/end functions.
     *
     * These are used for range-based for loops (`for (auto x : y)`).
     */
    ///@{
    template<typename T, size_t N> constexpr   T *begin(T (&ar)[N]) { return ar;         }
    template<typename T, size_t N> constexpr   T *end  (T (&ar)[N]) { return ar+N;       }
    template<typename T>           constexpr auto begin(T  &ar)     { return ar.begin(); }
    template<typename T>           constexpr auto end  (T  &ar)     { return ar.end();   }
    ///@}

    /**
     * Enumerator container adapter.
     *
     * This allows looping over a container with an index, without resorting to
     * old-style for loops.
     *
     * (comparable with Haskell `zip [0..]`, or Python `enumerate()`)
     *
     * Synopsis:
     *
     *     Array foo {'b', 'l', 'a'};
     *
     *     for (auto [i, val] : enumerate(foo))
     *         kprint("{}: {}\n", i, val);
     *
     * Prints:
     *
     *     0: b
     *     1: l
     *     2: a
     *
     * (if foo's iterator provides references, val should be a reference too)
     */
    template<typename T>
    struct Enumerator {

        /// Iterator type of the underlying container.
        ///@{
        using itb_t = decltype(ostd::begin(*(T*)0));
        using ite_t = decltype(ostd::end  (*(T*)0));
        ///@}

        itb_t begin_;
        ite_t end_;

        /// Return type of iterator's deref operator.
        /// (usually a reference to an element)
        using el_t = decltype(*declval<itb_t>());

        /// The {index,value} pair that is returned on each iteration.
        struct El {
            size_t i;
            el_t val;
        };

        struct end_marker_t {};

        /// Enumerator Iterator type.
        /// Keeps track of an index and the iterator of the underlying container.
        struct It {
            size_t i;
            itb_t  it;
            ite_t  end_;

            constexpr It &operator++() {
                ++it, ++i;
                return *this;
            }
            constexpr El operator*()       { return El { i, *it }; }
            constexpr El operator*() const { return El { i, *it }; }
            constexpr bool operator==(const It &o) const { return it == o.it; }
            constexpr bool operator!=(const It &o) const { return it != o.it; }

            constexpr bool operator==(end_marker_t) const { return it == end_; }
            constexpr bool operator!=(end_marker_t) const { return !(*this == end_marker_t{}); }
        };

        constexpr It begin() const { return It { 0, begin_, end_ }; }
        // constexpr It end()   const { return It { 0,   end_ }; }
        constexpr end_marker_t end() const { return {}; }

        constexpr Enumerator(T  &x) : begin_(ostd::begin(x)), end_(ostd::end(x)) { }
        constexpr Enumerator(T &&x) : begin_(ostd::begin(x)), end_(ostd::end(x)) { }
    };

    /// \see Enumerator
    template<typename T> constexpr Enumerator<T> enumerate(T  &x) { return Enumerator(x); }
    /// \see Enumerator
    template<typename T> constexpr Enumerator<T> enumerate(T &&x) { return Enumerator(x); }

    /**
     * Zipper container adapter.
     *
     * This allows looping over two containers at once, without resorting to
     * old-style for loops.
     *
     * Any excess elements in the larger container are ignored.
     *
     * Synopsis:
     *
     *     Array  foo {1, 2, 3};
     *     String bar = "blah";
     *
     *     for (auto [x, y] : zip(foo, bar))
     *         kprint("({},{})\n", x, y);
     *
     * Prints:
     *
     *     (1,b)
     *     (2,l)
     *     (3,a)
     *
     * (if either iterator provides references, their respective x/y value
     * should be a reference too)
     */
    template<typename T1, typename T2>
    struct Zipper {
        /// Iterator types of the underlying containers.
        ///@{
        using it1b_t = decltype(ostd::begin(*(T1*)0));
        using it1e_t = decltype(ostd::end  (*(T1*)0));

        using it2b_t = decltype(ostd::begin(*(T2*)0));
        using it2e_t = decltype(ostd::end  (*(T2*)0));
        ///@}

        it1b_t begin1_; it1e_t end1_;
        it2b_t begin2_; it2e_t end2_;

        /// Return type of iterator's deref operator.
        /// (usually a reference to an element)
        ///@{
        using el1_t = decltype(*declval<it1b_t>());
        using el2_t = decltype(*declval<it2b_t>());
        ///@}

        struct El {
            el1_t x;
            el2_t y;
        };

        struct end_marker_t {};

        struct It {
            it1b_t it1;
            it2b_t it2;
            it1e_t end1_;
            it2e_t end2_;

            constexpr It &operator++() {
                ++it1, ++it2;
                return *this;
            }
            constexpr El operator*()       { return El { *it1, *it2 }; }
            constexpr El operator*() const { return El { *it1, *it2 }; }
            constexpr bool operator==(const It &o) const {
                return it1 == o.it1 && it2 == o.it2;
            }
            constexpr bool operator!=(const It &o)  const { return !(*this == o); }
            constexpr bool operator==(end_marker_t) const {
                return it1 == end1_
                    || it2 == end2_;
            }
            constexpr bool operator!=(end_marker_t) const { return !(*this == end_marker_t{}); }
        };

        constexpr It         begin() const { return It { begin1_
                                                       , begin2_
                                                       , end1_
                                                       , end2_ }; }
        constexpr end_marker_t end() const { return {}; }

        constexpr Zipper(T1 &x, T2 &y)
            : begin1_(ostd::begin(x)), end1_(ostd::end(x))
            , begin2_(ostd::begin(y)), end2_(ostd::end(y))
            { }
        constexpr Zipper(T1 &&x, T2 &&y)
            : begin1_(ostd::begin(x)), end1_(ostd::end(x))
            , begin2_(ostd::begin(y)), end2_(ostd::end(y))
            { }
    };

    /// \see Zipper
    template<typename T1, typename T2>
    constexpr Zipper<T1,T2> zip(T1 &x, T2 &y) { return Zipper(x, y); }
    template<typename T1, typename T2>
    constexpr Zipper<T1,T2> zip(T1 &&x, T2 &&y) { return Zipper(x, y); }

    /**
     * Range iterator.
     *
     * This allows for Python-style iteration:
     *
     *     for (int i : range(3))
     *         kprint("{}\n", i);
     *
     *     => 0
     *     => 1
     *     => 2
     *
     *     for (int i : range(-1, 3))
     *         kprint("{}\n", i);
     *
     *     => -1
     *     => 0
     *     => 1
     *     => 2
     */
    template<typename T = size_t>
    struct Range {

        T start;
        T step;
        T stop;

        struct It {
            T val;
            T step;
            T stop;

            constexpr It &operator++() {
                val += step;
                return *this;
            }
            constexpr T operator*() const { return val; }
            constexpr bool operator==(const It &o) const {
                if (o.val == stop)
                     return val >= stop;
                else return val == o.val;
            }
            constexpr bool operator!=(const It &o) const { return !(*this == o); }
        };

        constexpr It begin() const { return { start, step, stop }; }
        constexpr It end()   const { return { stop,  step, stop }; }

        constexpr Range(T start_, T stop_, T step_ = 1)
            : start(start_)
            , step (step_ )
            , stop(stop_) { }
    };

    /// \see Range
    template<typename T, typename T2>
    constexpr Range<T> range(T start, T2 stop, T step = 1) {
        return { start, T(stop), step };
    }

    /// \see Range
    template<typename T = size_t>
    constexpr Range<T> range(T count) { return range(T(0), count); }
}
