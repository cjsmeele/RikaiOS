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

OUTPUT_FORMAT(elf32-i386)
ENTRY(start_)

SECTIONS {

    PROCESS_ARGUMENTS_ = 0x40000000;

    . = 0x40100000;

    .text : {
        *(.text*)
        *(.gnu.linkonce.t*)
    }

    .rodata ALIGN(0x1000) : {

        CTORS_START_ = .;
        *(SORT(.ctor*))
        CTORS_END_ = .;

        DTORS_START_ = .;
        *(SORT(.dtor*))
        DTORS_END_ = .;

        *(.rodata*)
        *(.gnu.linkonce.r*)
    }

    .data ALIGN(0x1000) : {
        *(.data)
        *(.gnu.linkonce.d*)
    }

    .bss ALIGN(0x1000) : {
        BSS_START = .;
        *(COMMON)
        *(.gnu.linkonce.b*)
        *(.bss)
        BSS_END = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}
