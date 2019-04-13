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

        T &container;

        /// Iterator type of the underlying container.
        using it_t = decltype(ostd::begin(container));

        /// Return type of iterator's deref operator.
        /// (usually a reference to an element)
        using el_t = decltype(*declval<it_t>());

        /// The {index,value} pair that is returned on each iteration.
        struct El {
            size_t i;
            el_t val;
        };

        /// Enumerator Iterator type.
        /// Keeps track of an index and the iterator of the underlying container.
        struct It {
            size_t i;
            it_t  it;

            constexpr It &operator++() {
                ++it, ++i;
                return *this;
            }
            constexpr El operator*()       { return El { i, *it }; }
            constexpr El operator*() const { return El { i, *it }; }
            constexpr bool operator==(const It &o) const { return it == o.it; }
            constexpr bool operator!=(const It &o) const { return it != o.it; }
        };

        constexpr It begin() const { return It { 0, ostd::begin(container) }; }
        constexpr It end()   const { return It { 0,   ostd::end(container) }; }

        constexpr Enumerator(T &x) : container(x) { }
    };

    /// \see Enumerator
    template<typename T> constexpr Enumerator<T> enumerate(T &x) { return Enumerator(x); }

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
        T1 &container1;
        T2 &container2;

        /// Iterator type of the underlying container.
        using it1_t = decltype(ostd::begin(container1));
        using it2_t = decltype(ostd::begin(container2));

        /// Return type of iterator's deref operator.
        /// (usually a reference to an element)
        using el1_t = decltype(*declval<it1_t>());
        using el2_t = decltype(*declval<it2_t>());

        struct El {
            el1_t x;
            el2_t y;
        };

        struct end_marker_t {};

        struct It {
            const T1 &container1;
            const T2 &container2;
            it1_t it1;
            it2_t it2;

            constexpr It &operator++() {
                ++it1, ++it2;
                return *this;
            }
            constexpr El operator*()       { return El { *it1, *it2 }; }
            constexpr El operator*() const { return El { *it1, *it2 }; }
            constexpr bool operator==(const It &o) const {
                return it1 == o.it1 && it2 == o.it2;
            }
            constexpr bool operator!=(const It &o) const { return !(*this == o); }
            constexpr bool operator==(end_marker_t) const {
                return it1 == ostd::end(container1)
                    || it2 == ostd::end(container2);
            }
            constexpr bool operator!=(end_marker_t) const { return !(*this == end_marker_t{}); }
        };

        constexpr It         begin() const { return It { container1
                                                       , container2
                                                       , ostd::begin(container1)
                                                       , ostd::begin(container2) }; }
        constexpr end_marker_t end() const { return {}; }

        constexpr Zipper(T1 &x, T2 &y)
            : container1(x)
            , container2(y) { }
    };

    /// \see Zipper
    template<typename T1, typename T2>
    constexpr Zipper<T1,T2> zip(T1 &x, T2 &y) { return Zipper(x, y); }
}
