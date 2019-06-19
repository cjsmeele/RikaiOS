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

ifndef TOOLCHAIN
    # Default to clang: it doesn't require building a cross-compiler.
    TOOLCHAIN = clang

    # Use TOOLCHAIN=custom if you want to override CXX/LD etc. manually on the
    # command-line.
endif

NAME   = libsys

START_ASM = src/start.asm
START_OBJ = o/start.o

CXX_SOURCES = $(shell find src -name '*.cc')
CXX_O       = $(CXX_SOURCES:%.cc=%.o)

BIN = lib/libsys.a

AS  = nasm

ifeq ($(TOOLCHAIN), gcc)
    CXX        = i686-elf-g++
    AR         = i686-elf-ar
    USE_LIBGCC = 1
endif
ifeq ($(TOOLCHAIN), clang)
    CXX         = clang++ --target=i686-elf
    AR          = llvm-ar
    USE_CLANGRT = 1
endif

DOXYGEN = doxygen

ASM_SOURCES = $(shell find src -name '*.asm')
CXX_SOURCES = $(shell find src -name '*.cc')

ASFLAGS = -f elf32

# Search path for C++ headers.
CXX_INCLUDE = \
	-Iinclude \
	-I../../kernel/include

# XXX: We may need threadsafe statics.
#-fhosted
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

.PHONY: all clean

all: $(BIN)

-include Makefile.local

ifdef WERROR
    CXXFLAGS += -Werror
endif

# Make targets {{{

$(BIN): $(START_OBJ) $(CXX_O)
	$(F)$@
	$(Q)mkdir -p $(@D) && $(AR) -rcs $@ $^

$(CXX_O): %.o: %.cc
	$(F)$@
	$(Q)mkdir -p $(@D) && $(CXX) -o $@ -c $< $(CXXFLAGS)

$(START_OBJ): $(START_ASM)
	$(F)$@
	$(Q)mkdir -p $(@D) && $(AS) $(ASFLAGS) -o $@ $<

clean:
	$(Q)rm -rf $(BIN) lib o

# }}}
