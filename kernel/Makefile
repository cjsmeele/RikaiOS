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

ifndef TOOLCHAIN
    # Default to clang: it doesn't require building a cross-compiler.
    TOOLCHAIN = clang

    # Use TOOLCHAIN=custom if you want to override CXX/LD etc. manually on the
    # command-line.
endif

AS = nasm

ifeq ($(TOOLCHAIN), gcc)
    CXX        = i686-elf-g++
    LD         = i686-elf-ld
    OBJCOPY    = i686-elf-objcopy
    USE_LIBGCC = 1
endif
ifeq ($(TOOLCHAIN), clang)
    CXX         = clang++ --target=i686-elf
    LD          = ld.lld
    OBJCOPY     = llvm-objcopy
    USE_CLANGRT = 1
endif

DOXYGEN = doxygen

ASM_SOURCES = $(shell find src -name '*.asm')
CXX_SOURCES = $(shell find src -name '*.cc')

ASFLAGS = -f elf32

# Search path for C++ headers.
CXX_INCLUDE =          \
	-I ../boot/include \
	-I ./include       \
	-I ./src

# Lots of options for the C++ compiler, mostly to tell it that we are running
# "bare-metal", and to enable most warnings.

CXXFLAGS =                          \
	-march=i686                     \
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
	-Wshadow                        \
	-Wpointer-arith                 \
	-Wcast-align                    \
	-Wwrite-strings                 \
	-Wmissing-declarations          \
	-Wredundant-decls               \
	-Wuninitialized                 \
	-Wfatal-errors                  \
	-fdiagnostics-color=auto

# The GCC libgcc lib and the clang builtins lib contain runtime support
# functions that are needed for a.o. some division operations, floating point
# ops, and advanced bit operations (popcnt, etc.).
ifdef USE_LIBGCC
    LD_BUILTIN_DIR  =                                          \
        .                                                      \
        $(wildcard /usr/local/lib/gcc/i686-elf/*) \
        $(wildcard /usr/lib/gcc/i686-elf/*)
    LD_BUILTIN_NAME = gcc
endif
ifdef USE_CLANGRT
    LD_BUILTIN_DIR  =                                \
        .                                            \
        $(wildcard /usr/local/lib/clang/*/lib/linux) \
        $(wildcard /usr/lib/clang/*/lib/linux)
    LD_BUILTIN_NAME = clang_rt.builtins-i386
endif

LDFLAGS =                              \
	-Tkernel.ld                        \
	-Map=kernel.map                    \
	$(addprefix -L, $(LD_BUILTIN_DIR)) \
	$(addprefix -l, $(LD_BUILTIN_NAME))

DEPFILE    = deps.make
CXX_D      = $(CXX_SOURCES:src/%.cc=d/%.d)
CXX_O      = $(CXX_SOURCES:src/%.cc=o/%.o)
ASM_O      = $(ASM_SOURCES:src/%.asm=o/%.o)
KERNEL_ELF = kernel.elf
KERNEL_BIN = kernel.bin

.PHONY: all elf bin dep doc clean

-include Makefile.local

ifdef WERROR
    CXXFLAGS += -Werror
endif

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
	$(Q)mkdir -p $(@D) && $(LD) -o $@ $^ $(LDFLAGS)

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
