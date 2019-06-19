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

ifndef NAME
$(error NAME variable missing, should contain name of program to build)
endif

BIN = ../$(NAME).elf

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

ifndef TOOLCHAIN
    # Default to clang: it doesn't require building a cross-compiler.
    TOOLCHAIN = clang

    # Use TOOLCHAIN=custom if you want to override CXX/LD etc. manually on the
    # command-line.
endif

AS  = nasm

ifeq ($(TOOLCHAIN), gcc)
    CXX        = i686-elf-g++
    LD         = i686-elf-ld
    OBJCOPY    = i686-elf-objcopy
    AR         = i686-elf-ar
    USE_LIBGCC = 1
endif
ifeq ($(TOOLCHAIN), clang)
    CXX         = clang++ --target=i686-elf
    LD          = ld.lld
    OBJCOPY     = llvm-objcopy
    AR          = llvm-ar
    USE_CLANGRT = 1
endif

ASM_SOURCES = $(shell find -name '*.asm')
CXX_SOURCES = $(shell find -name '*.cc')
CXX_O       = $(CXX_SOURCES:%.cc=%.o)
ASM_O       = $(ASM_SOURCES:%.asm=%.o)

ASFLAGS = -f elf32

# Search path for C++ headers.
CXX_INCLUDE =               \
	-I ../libsys/include    \
	-I ../../kernel/include

# XXX: We may need threadsafe statics in the future.
CXXFLAGS =                          \
	-march=i686                     \
	-m32                            \
	-msoft-float                    \
	-Os                             \
	-g3                             \
	-ggdb                           \
	-nostdlib                       \
	-fno-stack-protector            \
	-fno-exceptions                 \
	-fno-rtti                       \
	-fno-threadsafe-statics         \
	-fwrapv                         \
	-fdiagnostics-color=auto        \
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
	-Wno-unused-variable            \
	-Wno-unused-function            \
	-Wfatal-errors                  \
	-fdiagnostics-color=auto

LDFILE = ../libsys/link.ld

# The GCC libgcc lib and the clang builtins lib contain runtime support
# functions that are needed for a.o. some division operations, floating point
# ops, and advanced bit operations (popcnt, etc.).
ifdef USE_LIBGCC
    LD_BUILTIN_DIR  =                             \
        . .. ../..                                \
        $(wildcard /usr/local/lib/gcc/i686-elf/*) \
        $(wildcard /usr/lib/gcc/i686-elf/*)
    LD_BUILTIN_NAME = gcc
endif
ifdef USE_CLANGRT
    LD_BUILTIN_DIR  =                                \
        . .. ../..                                   \
        $(wildcard /usr/local/lib/clang/*/lib/linux) \
        $(wildcard /usr/lib/clang/*/lib/linux)
    LD_BUILTIN_NAME = clang_rt.builtins-i386
endif

LDFLAGS =                               \
	-T$(LDFILE)                         \
	-L ../libsys/lib                    \
	-lsys                               \
	-Map=$(BIN).map                     \
	$(addprefix -L, $(LD_BUILTIN_DIR))  \
	$(addprefix -l, $(LD_BUILTIN_NAME))

.PHONY: all clean

all: $(BIN)

-include Makefile.local

ifdef WERROR
    CXXFLAGS += -Werror
endif

$(BIN): $(ASM_O) $(CXX_O)
	$(F)$@
	$(Q)mkdir -p $(@D) && $(LD) -o $@ $^ $(LDFLAGS)

$(CXX_O): %.o: %.cc
	$(F)$@
	$(Q)mkdir -p $(@D) && $(CXX) -o $@ -c $< $(CXXFLAGS)

$(ASM_O): %.o: src/%.asm
	$(F)$@
	$(Q)mkdir -p $(@D) && $(AS) $(ASFLAGS) -o $@ $<

clean:
	$(Q)rm -f $(BIN) $(BIN).map $(ASM_O) $(CXX_O)
