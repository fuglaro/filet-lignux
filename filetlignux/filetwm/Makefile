# filetwm - dynamic window manager (filet-lignux fork)
# See LICENSE file for copyright and license details.

# version
VERSION = filetwm-8.0

# Customize below to fit your system
# includes and libs
INCS = -I/usr/X11R6/include -I/usr/include/freetype2
LDFLAGS = -L/usr/X11R6/lib -lX11 -lXi -lfontconfig -lXft -lXrandr -ldl

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\"
CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}

# compiler and linker
CC = cc

SRC = filetwm.c
OBJ = ${SRC:.c=.o}

all: options filetwm

options:
	@echo filetwm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

filetwm: ${OBJ}
	${CC} -rdynamic -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f filetwm ${OBJ}

.PHONY: all options clean
