# makefile - build system for a C-style kscript extension
#
# @author: Cade Brown <cade@kscript.org>


# -*- Package Information -*-

# Name of the module being built
NAME           := system


# -*- Config -*-

# Directory that kscript is installed to
KSDIR          ?= ../../


# -*- Files -*-

src_C          := $(wildcard src/*.c)
src_H          := $(wildcard include/*.h)

src_O          := $(patsubst %.c,%.o,$(src_C))

lib_DLL        := lib/libksm_$(NAME).so


# -*- Rules -*-

.PHONY: all default clean lib

default: lib

all: lib

lib: $(lib_DLL)

# Rule to clean temporary files
clean:
	rm -f $(wildcard $(lib_DLL) $(src_O))

# Rule to build the main library
$(lib_DLL): $(src_O)
	@mkdir -p $(dir $@)
	$(CC) $^ -L$(KSDIR)/lib -lks -shared -o $@

# Rule to build a single object file
%.o: %.c $(src_H)
	$(CC) -I$(KSDIR)/include $< -fPIC -c -o $@
