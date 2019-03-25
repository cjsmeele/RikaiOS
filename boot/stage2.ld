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
ENTRY(stage2_main)

SECTIONS {
    . = 0x00007e00;

    .start ALIGN (0x10) : {
        stage2/start.o (*)
    }

    .text ALIGN (0x10) : {
        *(.text*)
    }

    .rodata ALIGN (0x10) : {
        *(.rodata*)
    }

    .data ALIGN (0x10) : {
        *(.data)
    }

    .bss ALIGN (0x10) : {
        STAGE2_BSS_START = .;
        *(COMMON)
        *(.bss)
        STAGE2_BSS_END = .;
    }

    /DISCARD/ : {
        *(.comment)
    }
}
