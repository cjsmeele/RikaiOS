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

# QEMU     = qemu-system-i386 -full-screen
QEMU     = qemu-system-i386
QEMU_KVM = qemu-system-x86_64 -enable-kvm -cpu max
QEMU_IMG = qemu-img
BOCHS    = bochs
GDB      = gdb

BOCHSRC  = ./bochsrc
GDBRC    = ./gdbrc

DISK_IMG      = ./disk.img
DISK_SIZE_M   = 100

# VirtualBox does not like raw images.
DISK_IMG_QCOW = ./disk.qcow
DISK_IMG_VDI  = ./disk.vdi
DISK_IMG_VMDK = ./disk.vmdk

QEMU_SERIAL        = stdio
QEMU_SERIAL_DEBUG  = file:serial-out.bin

ifdef NOVGA
    QEMU_DISPLAY = none
else
    QEMU_DISPLAY = sdl
endif

QEMUFLAGS =                                                \
    -name osdev                                            \
    -m 1G                                                  \
    -drive if=ide,bus=0,unit=0,format=raw,file=$(DISK_IMG) \
    -serial  $(QEMU_SERIAL)                                \
    -display $(QEMU_DISPLAY)


QEMUFLAGS_DEBUG =                                          \
    -name osdev                                            \
    -m 1G                                                  \
    -drive if=ide,bus=0,unit=0,format=raw,file=$(DISK_IMG) \
    -serial  $(QEMU_SERIAL_DEBUG)                          \
    -display $(QEMU_DISPLAY)                               \
    -S -gdb tcp::1133

.PHONY: build user doc rebuild boot kernel disk qcow run debug qemu qemu-kvm bochs clean

-include Makefile.local

build: boot kernel

user:
	$(Q)$(MAKE) -C user all

doc:
	$(Q)$(MAKE) -C kernel doc

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

disk: build user
	$(F)$(DISK_IMG)
	$(Q)./mkdisk.sh $(DISK_IMG) $(DISK_SIZE_M)
	$(Q)./boot/utils/loader-install \
	        $(DISK_IMG)             \
	        boot/bootsect.bin       \
	        boot/stage2.bin         \
	        kernel/kernel.bin

qcow: disk
	$(F)$(DISK_IMG_QCOW)
	$(Q)$(QEMU_IMG) convert -f raw -O qcow $(DISK_IMG) $(DISK_IMG_QCOW)
vdi: disk
	$(F)$(DISK_IMG_VDI)
	$(Q)$(QEMU_IMG) convert -f raw -O vdi $(DISK_IMG) $(DISK_IMG_VDI)
	# VBox throws a tantrum if you replace a VDI file with a different UUID,
	# so we fix.
	VBoxManage internalcommands sethduuid $(DISK_IMG_VDI) \
		aeeeeeee-aeee-aeee-aaaa-aaaaaaaaaaaa
vmdk: disk
	$(F)$(DISK_IMG_VMDK)
	$(Q)$(QEMU_IMG) convert -f raw -O vmdk $(DISK_IMG) $(DISK_IMG_VMDK)
	VBoxManage internalcommands sethduuid $(DISK_IMG_VMDK) \
		aeeeeeee-aeee-aeee-aaaa-aaaaaaaaaaaa

clean:
	$(Q)$(MAKE) -C boot   clean
	$(Q)$(MAKE) -C kernel clean
	$(Q)$(MAKE) -C user   clean
	$(Q)rm -f $(DISK_IMG) $(DISK_IMG_QCOW)
