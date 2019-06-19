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

#include <os-std/array.hh>

namespace ostd {

    /**
     * Single-reader single-writer lock-free fixed-capacity queue.
     */
    template<typename T, size_t N>
    struct Queue {

        struct null_item_t { char _; };
        union Item {
            null_item_t _;
            T data;

            constexpr  Item() : _{} {}
            ~Item() { }
        };

        Array<Item,N> items;

        /// The amount of (non-null) items we currently have.
        size_t length_ = 0;

        /// Where the next item is written
        /// (only accessed by the writer).
        size_t i_writer_ = 0;

        /// From where the next item is read
        /// (only accessed by the reader).
        size_t i_reader_ = 0;

        /// Increment an index, taking care of wrap-around.
        constexpr static void increment(size_t &i) {
            if (i == N - 1)
                 i = 0;
            else i = i + 1;
        };

        constexpr size_t size()   const { return N; }
        constexpr size_t length() const { return length_; }

        constexpr void enqueue_(const T &item) {
            // Assume length_ < N.

            items[i_writer_].data = item;
            increment(i_writer_);

            ++length_;
        }

        constexpr void enqueue_(T &&item) {
            // Assume length_ < N.

            items[i_writer_].data = move(item);
            increment(i_writer_);

            ++length_;
        }

        template<typename U>
        constexpr bool enqueue(U &&x) {
            if (length_ == N) {
                return false;
            } else {
                enqueue_(forward(x));
                return true;
            }
        }

        constexpr T dequeue_() {
            // Assume length_ > 0.

            T item = move(items[i_reader_].data);

            items[i_reader_].data.~T();
            items[i_reader_]._ = null_item_t { }; // For good measure.

            increment(i_reader_);

            --length_;

            return move(item);
        }

        constexpr bool dequeue(T &x) {
            if (length_ == 0) {
                return false;
            } else {
                x = dequeue_();
                return true;
            }
        }

        constexpr Queue() = default;

        ~Queue() {
            while (length_) dequeue_();
        }
    };
}
