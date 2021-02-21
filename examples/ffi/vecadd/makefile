# makefile - build system for the 'vecadd' example
#
# @author: Cade Brown <cade@kscript.org>

# -*- Tools -*-


# -*- Files -*-

src_H          := $(wildcard *.h)

src_C          := $(wildcard *.c)


# -*- Outputs -*-

src_O          := $(patsubst %.c,%.o,$(src_C))

lib_SO         := libvecadd.so

# -*- Rules -*-

.PHONY: default clean FORCE

default: $(lib_SO)

clean: FORCE
	rm -f $(wildcard $(src_O) $(lib_SO))
	
FORCE:

$(lib_SO): $(src_O)
	$(CC) -shared $^ -o $@

%.o: %.c $(src_H)
	$(CC) -fPIC -c $< -o $@
