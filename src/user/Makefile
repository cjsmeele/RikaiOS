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

# Gets a list of all subdirectories containing Makefiles (excluding the
# standard library).
PROGRAMS := $(filter-out libsys/ bin/, $(dir $(wildcard */Makefile)))

BIN_DST = $(PROGRAMS:%/=bin/%.elf)
BIN_SRC = $(PROGRAMS:%/=%.elf)

.PHONY: all clean libsys $(PROGRAMS)

all: libsys $(BIN_DST)

-include Makefile.local

libsys:
	$(MAKE) -C $@

$(BIN_DST): bin/%.elf: %.elf
	$(Q)mkdir -p $(@D) && cp $< $@

$(BIN_SRC): %.elf: % libsys
	$(MAKE) -C $<

$(PROGRAMS):
	$(MAKE) -C $@

clean:
	$(Q)rm -rf bin
	$(Q)for i in $(PROGRAMS) libsys; do $(MAKE) -C "$$i" clean; done
