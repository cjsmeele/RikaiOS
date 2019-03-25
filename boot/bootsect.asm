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

[bits 16]
[org  0x7c00]

;; Far jump to start (this resets the cs register).
jmp 0x0000:start

;; The above jmp instruction takes up 5 bytes.
;; Right after, we place information that will enable us to load the rest of
;; the bootloader.
;;
;; These fields will be overwritten by the bootloader installer.
;;
u32_stage2_start:    dd 0xffffffff
u8_stage2_blocks:    db 0xff
u32_kernel_start:    dd 0xffffffff
u32_kernel_blocks:   dd 0xffffffff

;; Temporary storage for the boot disk number.
u8_boot_device:      dd 0

;; Informative strings.
s_loading: db "stage1 ", 0
s_error:   db "cannot load stage2", 0

;; A function to print a null-terminated string pointed to by the si register.
puts:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0e
    mov bx, 0x0007
    int 0x10
    jmp puts
.done:
    ret

;; Entrypoint.
start:
    ;; Set all segment registers to zero.
    ;; The code segment (cs) is already set to zero by the far jump at the top.
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ;; Create a stack right below this bootsector code.
    mov sp, 0x7c00

    ;; Save the boot disk number.
    mov [u8_boot_device], dl

    mov si, s_loading
    call puts

    ;; Load stage2 from disk.
.load_stage2:
    mov si, struct_dap
    mov eax, [u8_stage2_blocks]
    mov [struct_dap.block_count], al
    mov eax, [u32_stage2_start]
    mov [struct_dap.block_number], eax
    mov ah, 0x42

    ;; NB: BIOS sets dl register to the boot device number.
    ;;     it is passed unchanged to int 0x13.
    int 0x13
    jc .error

    ;; Stage2 is now loaded right after the bootsector.
    ;; Push kernel location information as arguments, then jump to stage2.
    push dword [u32_kernel_blocks]
    push dword [u32_kernel_start]
    push dword [u8_boot_device]

    jmp 0x7e00

.error:
    ;; Something went wrong.
    mov si, s_error
    call puts
    cli
    hlt

align 8

;; Disk address packet structure.
;; This tells the bios what we want to read from disk, and where to store the
;; data in memory.
struct_dap:
.length:       db 0x10   ;; Struct length.
               db 0
.block_count:  db 0      ;; Amount of blocks to read (max 127).
               db 0
.dest_offset:  dw 0x7e00 ;; Destination offset (right after this bootcode).
.dest_segment: dw 0x0000 ;; Destination segment.
.block_number: dq 1      ;; The starting block address on disk.

;; Emit bootable disk signature.
times 510 - ($ - $$) db 0
dw 0xaa55
