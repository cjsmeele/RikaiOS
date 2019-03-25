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

AS      := yasm
CXX     := clang++
LD      := ld.lld
OBJCOPY := llvm-objcopy

STAGE2_CXX_SOURCES := \
	stage2/main.cc    \
	stage2/memory.cc  \
	stage2/console.cc \
	stage2/disk.cc    \
	stage2/asm.cc

STAGE2_ASFLAGS := -f elf32

# Lots of options for the C++ compiler, mostly to tell it that we are running
# "bare-metal", and to enable most warnings.

STAGE2_CXXFLAGS :=           \
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

STAGE2_LDFLAGS := -Tstage2.ld

.PHONY: all clean bootloader installer

-include Makefile.local

all: bootloader installer

# Bootloader {{{

bootloader: bootsect.bin stage2.bin

bootsect.bin: bootsect.asm
	$(AS) -o $@ $<

stage2.bin: stage2.elf
	$(OBJCOPY) -Obinary $< $@

STAGE2_CXX_O := $(STAGE2_CXX_SOURCES:%.cc=%.o)

stage2.elf: $(STAGE2_CXX_O) stage2/start.o
	$(LD) $(STAGE2_LDFLAGS) -o $@ $^

$(STAGE2_CXX_O): stage2/%.o: stage2/%.cc
	$(CXX) $(STAGE2_CXXFLAGS) -c -o $@ $<

stage2/start.o: stage2/start.asm
	$(AS) $(STAGE2_ASFLAGS) -o $@ $<

# }}}
# Installer {{{

installer: utils/loader-install

utils/loader-install: utils/loader-install.cc
	$(CXX) -Wall -Wextra -pedantic -std=c++17 -o $@ $<

# }}}

clean:
	rm -f utils/loader-install
	rm -f bootsect.bin stage2.bin stage2.elf
	rm -f $(STAGE2_CXX_O) stage2/start.o
