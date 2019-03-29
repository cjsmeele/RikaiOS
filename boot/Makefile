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
    F := @echo " . boot/"
    MAKEFLAGS += --no-print-directory
endif

AS      = nasm
CXX     = clang++
LD      = ld.lld
OBJCOPY = llvm-objcopy

STAGE2_CXX_SOURCES =  \
	stage2/main.cc    \
	stage2/memory.cc  \
	stage2/console.cc \
	stage2/disk.cc    \
	stage2/asm.cc

STAGE2_HEADERS =             \
	stage2/types.hh          \
	stage2/asm.hh            \
	stage2/console.hh        \
	stage2/disk.hh           \
	stage2/memory.hh         \
	include/boot/bootinfo.hh

STAGE2_ASFLAGS = -f elf32

# Lots of options for the C++ compiler, mostly to tell it that we are running
# "bare-metal", and to enable most warnings.

STAGE2_CXXFLAGS =            \
	--target=i686-elf        \
	-m16                     \
	-Os                      \
	-g3                      \
	-ggdb                    \
	-nostdlib                \
	-ffreestanding           \
	-fno-stack-protector     \
	-fno-exceptions          \
	-fno-rtti                \
	-fno-threadsafe-statics  \
	-fwrapv                  \
	-mno-red-zone            \
	-mno-sse                 \
	-std=c++17               \
	-pipe                    \
	-Istage2                 \
	-Wall                    \
	-Wextra                  \
	-Wpedantic               \
	-Werror=return-type      \
	-Werror=unused-result    \
	-Werror=return-std-move  \
	-Wshadow                 \
	-Wpointer-arith          \
	-Wcast-align             \
	-Wwrite-strings          \
	-Wmissing-declarations   \
	-Wredundant-decls        \
	-Winline                 \
	-Wuninitialized          \
	-Wfatal-errors           \
	-fdiagnostics-color=auto

STAGE2_LDFLAGS =    \
	-Tstage2.ld     \
	-Map=stage2.map

STAGE2_CXX_O = $(STAGE2_CXX_SOURCES:%.cc=%.o)

.PHONY: all bootloader stage1 stage2 installer clean

-include Makefile.local

all:        bootloader installer
bootloader: stage1 stage2
stage1:     bootsect.bin
stage2:     stage2.bin
installer:  utils/loader-install
clean:
	$(Q)rm -f utils/loader-install
	$(Q)rm -f bootsect.bin stage2.bin stage2.elf stage2.map
	$(Q)rm -f $(STAGE2_CXX_O) stage2/start.o

bootsect.bin: bootsect.asm
	$(F)$@
	$(Q)$(AS) -o $@ $<

stage2.bin: stage2.elf
	$(F)$@
	$(Q)$(OBJCOPY) -O binary $< $@

stage2.elf: $(STAGE2_CXX_O) stage2/start.o
	$(F)$@
	$(Q)$(LD) $(STAGE2_LDFLAGS) -o $@ $^

$(STAGE2_CXX_O): stage2/%.o: stage2/%.cc $(STAGE2_HEADERS)
	$(F)$@
	$(Q)$(CXX) $(STAGE2_CXXFLAGS) -c -o $@ $<

stage2/start.o: stage2/start.asm
	$(F)$@
	$(Q)$(AS) $(STAGE2_ASFLAGS) -o $@ $<

utils/loader-install: utils/loader-install.cc
	$(F)$@
	$(Q)$(CXX) -Wall -Wextra -pedantic -std=c++17 -o $@ $<
