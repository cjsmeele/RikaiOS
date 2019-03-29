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
    F := @echo " . kernel/"
    MAKEFLAGS += --no-print-directory
endif

AS      := nasm
CXX     := clang++
LD      := ld.lld
OBJCOPY := llvm-objcopy
DOXYGEN := doxygen

ASM_SOURCES := $(shell find src -name '*.asm')
CXX_SOURCES := $(shell find src -name '*.cc')

ASFLAGS := -f elf32

# Search path for C++ headers.
CXX_INCLUDE :=               \
	-isystem ./include       \
	-isystem ./src           \
	-I       ../boot/include

# Lots of options for the C++ compiler, mostly to tell it that we are running
# "bare-metal", and to enable most warnings.

CXXFLAGS :=                         \
	--target=i686-elf               \
	-m32                            \
	-Os                             \
	-g3                             \
	-ggdb                           \
	-nostdlib                       \
	-ffreestanding                  \
	-fno-stack-protector            \
	-fno-exceptions                 \
	-fno-rtti                       \
	-fno-threadsafe-statics         \
	-fwrapv                         \
	-mno-red-zone                   \
	-mno-sse                        \
	-std=c++17                      \
	-pipe                           \
	$(CXX_INCLUDE)                  \
	-Wall                           \
	-Wextra                         \
	-Wpedantic                      \
	-Werror=return-type             \
	-Werror=unused-result           \
	-Werror=return-std-move         \
	-Wshadow                        \
	-Wpointer-arith                 \
	-Wcast-align                    \
	-Wwrite-strings                 \
	-Wmissing-declarations          \
	-Wredundant-decls               \
	-Winline                        \
	-Wuninitialized                 \
	-Wfatal-errors                  \
	-fdiagnostics-color=auto

LDFLAGS :=          \
	-Tkernel.ld     \
	-Map=kernel.map

DEPFILE    := deps.make
CXX_D      := $(CXX_SOURCES:src/%.cc=d/%.d)
CXX_O      := $(CXX_SOURCES:src/%.cc=o/%.o)
ASM_O      := $(ASM_SOURCES:src/%.asm=o/%.o)
KERNEL_ELF := kernel.elf
KERNEL_BIN := kernel.bin

.PHONY: all elf bin dep doc clean

-include Makefile.local

all: bin
elf: $(KERNEL_ELF)
bin: $(KERNEL_BIN)
dep: $(DEPFILE)
doc: Doxyfile
	$(F)doc/
	$(Q)mkdir -p doc && $(DOXYGEN) Doxyfile
clean:
	$(Q)rm -f $(KERNEL_BIN) $(KERNEL_ELF) kernel.map
	$(Q)rm -rf d o

$(KERNEL_BIN): $(KERNEL_ELF)
	$(F)$@
	$(Q)mkdir -p $(@D) && $(OBJCOPY) -O binary $< $@

$(KERNEL_ELF): $(ASM_O) $(CXX_O)
	$(F)$@
	$(Q)mkdir -p $(@D) && $(LD) $(LDFLAGS) -o $@ $^

$(CXX_O): o/%.o: src/%.cc
	$(F)$@
	$(Q)mkdir -p $(@D) && $(CXX) $(CXXFLAGS) -c -o $@ $<

$(ASM_O): o/%.o: src/%.asm
	$(F)$@
	$(Q)mkdir -p $(@D) && $(AS) $(ASFLAGS) -o $@ $<

$(CXX_D): d/%.d: src/%.cc
	$(Q)mkdir -p $(@D) && $(CXX) $(CXXFLAGS) -MM -MT $(<:src/%.cc=o/%.o) -o $@ $<

$(DEPFILE): $(CXX_D)
	$(F)$@
	$(Q)mkdir -p $(@D) && cat $^ > $@

# Include generated dependencies if we're compiling or linking.
ifeq (,$(filter $(MAKECMDGOALS),dep doc clean))
-include $(DEPFILE)
endif
