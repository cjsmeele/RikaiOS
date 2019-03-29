;; Copyright 2019 Chris Smeele
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;;     https://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.

[bits 32]

global kernel_start
extern kmain

;; Kernel entrypoint.
;; This sets up a stack and calls main.
kernel_start:
    mov esp, kernel_stack_top
    push edx ; Boot information struct pointer, passed to kmain.
    call run_constructors
    call kmain
    ;; If kmain ever returns, hang the machine.
    cli
    hlt

align 16
;; Create a 32K stack.
kernel_stack:     times (32*1024) db 0
kernel_stack_top:

;; Some C++ datastructures must be initialised manually at startup.
;; These initialisation steps are recorded as function addresses in the .ctors
;; section.
;; Here we make sure to call all global constructors before calling
;; the C++ kmain function.

extern CTORS_START
extern CTORS_END

run_constructors:
    mov ebx, dword CTORS_START

    ;; Iterate over and call all global ctors.
.loop:
    mov eax, dword CTORS_END
    cmp ebx, eax
    jge .end
    call [ebx]
    add ebx, 4
    jmp .loop
.end:
    ret
