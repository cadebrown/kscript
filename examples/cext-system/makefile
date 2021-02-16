# makefile - build system for a C-style kscript extension
#
# @author: Cade Brown <cade@kscript.org>


# -*- Package Information -*-

# Name of the module being built
NAME           := system


# -*- Config -*-

# kscript directory
#KS             ?= /usr/local
KS             ?= ../..

# install path
PREFIX         := /usr/local
DESTDIR        ?= 
TODIR          := $(DESTDIR)$(PREFIX)


# -*- Files -*-

src_C          := $(wildcard src/*.c)
src_H          := $(wildcard include/*.h)

src_O          := $(patsubst %.c,%.o,$(src_C))

mod_SO         := ksm_$(NAME).so


# -*- Rules -*-

.PHONY: default all clean install uninstall FORCE

default: $(mod_SO)

all: $(mod_SO)

clean:
	rm -f $(wildcard $(mod_SO) $(src_O))

install: FORCE
	install -d $(TODIR)/lib/ks/kpm/$(NAME)
	install -m 664 $(mod_SO) $(PREFIX)/lib/ks/kpm/$(NAME)/$(notdir $(mod_SO))
	strip $(PREFIX)/lib/ks/kpm/$(NAME)/$(notdir $(mod_SO))

uninstall: FORCE
	rm -rf $(TODIR)/lib/ks/kpm/$(NAME)

FORCE:

# Rule to build the main library
$(mod_SO): $(src_O)
	@mkdir -p $(dir $@)
	$(CC) $^ -L$(KS)/lib -lks -shared -o $@

# Rule to build a single object file
%.o: %.c $(src_H)
	$(CC) -I$(KS)/include $< -fPIC -c -o $@
