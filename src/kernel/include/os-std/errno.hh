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

#include "types.hh"
#include "string.hh"

namespace ostd {
    static constexpr errno_t ERR_success       =   0;
    static constexpr errno_t ERR_misc          =  -1;
    static constexpr errno_t ERR_invalid       =  -2;
    static constexpr errno_t ERR_limit         =  -3;
    static constexpr errno_t ERR_bad_fd        =  -4;
    static constexpr errno_t ERR_nospace       =  -5;
    static constexpr errno_t ERR_nomem         =  -6;
    static constexpr errno_t ERR_io            =  -7;
    static constexpr errno_t ERR_exists        =  -8;
    static constexpr errno_t ERR_not_exists    =  -9;
    static constexpr errno_t ERR_type          = -10;
    static constexpr errno_t ERR_timeout       = -11;
    static constexpr errno_t ERR_perm          = -12;
    static constexpr errno_t ERR_not_supported = -13;
    static constexpr errno_t ERR_in_use        = -14;
    static constexpr errno_t ERR_eof           = errno_t(1ULL << (sizeof(errno_t)*8-1));

    constexpr StringView error_name(errno_t err) {
        constexpr Array error_names
            { StringView("success")
            ,            "unknown error"
            ,            "invalid argument"
            ,            "limit reached"
            ,            "bad file descriptor"
            ,            "no space left on device"
            ,            "no free memory left"
            ,            "i/o error"
            ,            "file exists"
            ,            "file does not exist"
            ,            "invalid operation for file type"
            ,            "operation timed out"
            ,            "operation not permitted"
            ,            "operation not supported"
            ,            "resource is in use" };

        if (err >= ERR_success)
             return "success";
        else if (err == ERR_eof)
             return "end of file";
        else if (size_t(-err) >= error_names.size())
             return "unknown error";
        else return error_names[-err];
    }
}
