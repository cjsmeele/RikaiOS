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

;; Create an 8K stack.
kernel_stack:     times (8*1024) db 0
kernel_stack_top:

;; Kernel entrypoint.
;; This sets up a stack and calls main.
kernel_start:
    mov esp, kernel_stack_top
    push edx ;; boot information struct pointer.
    call kmain
    cli
    hlt
