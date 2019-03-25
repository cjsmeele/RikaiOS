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

AS      := nasm
CXX     := clang++
LD      := ld.lld
OBJCOPY := llvm-objcopy

CXX_SOURCES :=  \
	src/main.cc

ASFLAGS := -f elf32

# Lots of options for the C++ compiler, mostly to tell it that we are running
# "bare-metal", and to enable most warnings.

CXXFLAGS :=                  \
	--target=i686-elf        \
	-m32                     \
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

LDFLAGS := -Tkernel.ld

CXX_O      := $(CXX_SOURCES:%.cc=%.o)
KERNEL_ELF := kernel.elf
KERNEL_BIN := kernel.bin

.PHONY: all elf bin clean

-include Makefile.local

all: $(KERNEL_BIN)

bootloader: bootsect.bin stage2.bin

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

$(KERNEL_ELF): $(CXX_O) src/start.o
	$(LD) $(LDFLAGS) -o $@ $^

$(CXX_O): src/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

src/start.o: src/start.asm
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f $(KERNEL_BIN) $(KERNEL_ELF)
	rm -f $(CXX_O) src/start.o
