# makefile - Build system for kscript on *NIX systems
#
#
# @author: Cade Brown <cade@cade.site>


# -*- Config -*-

TMP                ?= .tmp


# -*- Targets -*-

LIB_SHARED         := libks.so
LIB_STATIC         := libks.a
BIN_BINARY         := ks

LIB                := $(LIB_SHARED)
BIN                := $(BIN_BINARY)



LDFLAGS            += -lm -lgmp


# -*- Source Files -*-

LIB_SRC_H          := $(wildcard include/ks/*.h)
LIB_SRC_C          := $(wildcard \
						src/*.c \
						src/types/*.c \
						src/modules/os/*.c \
						src/modules/m/*.c \
					)
BIN_SRC_C          := $(wildcard src/ks/*.c)


# -*- Object Files (Generated) -*-

LIB_SRC_O          := $(patsubst %.c,$(TMP)/%.o,$(LIB_SRC_C))
BIN_SRC_O          := $(patsubst %.c,$(TMP)/%.o,$(BIN_SRC_C))


# -*- RULES -*-

.PHONY: default all clean lib bin

default: bin

all: lib bin

clean:
	rm -f $(wildcard $(LIB) $(BIN) $(LIB_SRC_O) $(BIN_SRC_O))

lib: $(LIB)

bin: $(BIN)


# -*- Compilation Rules -*-

$(TMP)/%.o: %.c $(LIB_SRC_H)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -fPIC -Iinclude -o $@ $<

$(LIB_SHARED): $(LIB_SRC_O)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -shared -fPIC -o $@ $(LIB_SRC_O) $(LDFLAGS)

$(LIB_STATIC): $(LIB_SRC_O)
	@mkdir -p $(dir $@)
	$(AR) cr $@ $(LIB_SRC_O)

$(BIN_BINARY): $(LIB) $(BIN_SRC_O)
	$(CC) -L. $(BIN_SRC_O) -lks $(LDFLAGS) -o $@


# -*- Regeneration Rules (for devs, mainly) -*-


# This rule will update the implementation of 'ksm_gamma', based on the generator
src/modules/m/impl_gamma.c: tools/impl_gamma.py
	$< --type double --prec 64 --utilprefix ksm_ --funcname ksm_gamma --include "ks/impl.h" > $@


