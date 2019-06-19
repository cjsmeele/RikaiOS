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

section .text

global start_
global threadpoline_

extern main
extern CTORS_START_
extern CTORS_END_
extern PROCESS_ARGUMENTS_

start_:
    ;; Set up the stack.
    mov esp, stack_top

    ;; Call constructors of global objects.
    call call_constructors

    ;; Process argument information is at a known location,
    ;; pass it to main via the stack.
    mov  esi, PROCESS_ARGUMENTS_+4
    push dword  esi    ; push argv
    push dword [esi-4] ; push argc

    ;; Enter C++ main.
    call main

    ;; FIXME: Should call global destructors after main.

    ;; FIXME: Provide syscall number definitions for asm.
    mov eax, 3 ;; SYS_THREAD_DELETE
    mov ebx, 0 ;; (current thread)
    int 0xca
.hang:
    jmp .hang

call_constructors:
    ;; The CTORS area contains a list of function pointers we need to call.
    mov ebx, dword CTORS_START_

.loop:
    mov eax, dword CTORS_END_
    cmp ebx, eax
    jge .end
    call [ebx]
    add ebx, 4
    jmp .loop
.end:
    ret

;; Entrypoint for all new threads.
threadpoline_:
    ;; Context arg is in ebx, actual entrypoint is in eax.
    push ebx
    call eax
    push eax

    pop  ebx
    push ebx
    mov eax, 3 ;; SYS_THREAD_DELETE
    mov ebx, 0 ;; (current thread)
    int 0xca
.hang:
    jmp .hang

section .bss

;; We are responsible for creating our own stack.

align 16
stack: resb 1024*1024
stack_top:
