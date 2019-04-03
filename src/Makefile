# Copyright 2019 Chris Smeele
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Fancy output options.
# (use make <target> VERBOSE=1 to disable)
ifdef VERBOSE
    Q :=
    E := @true 
    F := @true 
else
    Q := @
    E := @echo 
    F := @echo " . "
    MAKEFLAGS += --no-print-directory
endif

QEMU     = qemu-system-i386
QEMU_KVM = qemu-system-x86_64 -enable-kvm
BOCHS    = bochs
GDB      = gdb

BOCHSRC  = ./bochsrc
GDBRC    = ./gdbrc

DISK_IMG    = ./disk.img
DISK_SIZE_M = 100

QEMU_SERIAL        = stdio
QEMU_SERIAL_DEBUG  = file:serial-out.bin

ifdef NOVGA
    QEMU_DISPLAY = none
else
    QEMU_DISPLAY = sdl
endif

QEMUFLAGS =                            \
    -name osdev                        \
    -m 1G                              \
    -drive format=raw,file=$(DISK_IMG) \
    -serial $(QEMU_SERIAL)             \
    -display $(QEMU_DISPLAY)

QEMUFLAGS_DEBUG =                      \
    -name osdev                        \
    -m 1G                              \
    -drive format=raw,file=$(DISK_IMG) \
    -serial $(QEMU_SERIAL_DEBUG)       \
    -display $(QEMU_DISPLAY)           \
    -S -gdb tcp::1133

.PHONY: doc build rebuild boot kernel disk run debug qemu qemu-kvm bochs clean

-include Makefile.local

doc:
	$(Q)$(MAKE) -C kernel doc

build: boot kernel

rebuild:
	$(Q)$(MAKE) -BC boot   all
	$(Q)$(MAKE) -BC kernel all

boot:
	$(Q)$(MAKE) -C boot   all

kernel:
	$(Q)$(MAKE) -C kernel all

run: qemu

qemu: $(DISK_IMG)
	$(QEMU) $(QEMUFLAGS)

qemu-kvm: $(DISK_IMG)
	$(QEMU_KVM) $(QEMUFLAGS)

debug: $(DISK_IMG)
	$(QEMU) $(QEMUFLAGS_DEBUG) &
	$(Q)$(GDB) -q -x $(GDBRC)

bochs: $(DISK_IMG)
	$(Q)rm -f $(DISK_IMG).lock
	$(BOCHS) -q -f $(BOCHSRC)

disk: build
	$(F)$(DISK_IMG)
	$(Q)./mkdisk.sh $(DISK_IMG) $(DISK_SIZE_M)
	$(Q)./boot/utils/loader-install \
	        $(DISK_IMG)             \
	        boot/bootsect.bin       \
	        boot/stage2.bin         \
	        kernel/kernel.bin

clean:
	$(Q)$(MAKE) -C boot   clean
	$(Q)$(MAKE) -C kernel clean
	$(Q)rm -f $(DISK_IMG)