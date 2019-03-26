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

QEMU     := qemu-system-i386
QEMU_KVM := qemu-system-x86_64 -enable-kvm
BOCHS    := bochs

DISK_IMG    := disk.img
DISK_SIZE_M := 100

.PHONY: build rebuild boot kernel disk run qemu qemu-kvm bochs clean

-include Makefile.local

build: boot kernel

rebuild:
	$(MAKE) -BC boot   all
	$(MAKE) -BC kernel all

boot:
	$(MAKE) -C boot   all

kernel:
	$(MAKE) -C kernel all

run: qemu

qemu: $(DISK_IMG)
	$(QEMU) \
	    -name osdev                        \
	    -m 512                             \
	    -drive format=raw,file=$(DISK_IMG) \
	    -serial stdio

qemu-kvm: $(DISK_IMG)
	$(QEMU_KVM) \
	    -name osdev                        \
	    -m 512                             \
	    -drive format=raw,file=$(DISK_IMG) \
	    -serial stdio

bochs: $(DISK_IMG)
	$(BOCHS)

disk: build
	./mkdisk.sh $(DISK_IMG) $(DISK_SIZE_M)
	./boot/utils/loader-install \
	    $(DISK_IMG)             \
	    boot/bootsect.bin       \
	    boot/stage2.bin         \
	    kernel/kernel.bin

clean:
	$(MAKE) -C boot   clean
	$(MAKE) -C kernel clean
	rm -f $(DISK_IMG)
