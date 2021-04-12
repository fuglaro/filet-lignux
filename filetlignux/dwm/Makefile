# dwm - dynamic window manager (filet-lignux fork)
# See LICENSE file for copyright and license details.

# dwm version
VERSION = filetlignux-fork-7.0

# Customize below to fit your system
# includes and libs
INCS = -I/usr/X11R6/include -I/usr/include/freetype2
LDFLAGS = -L/usr/X11R6/lib -lX11 -lXi -lfontconfig -lXft -lXrandr

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\"
CFLAGS   = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}

# compiler and linker
CC = cc

SRC = dwm.c
OBJ = ${SRC:.c=.o}

all: options dwm

options:
	@echo dwm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h

config.h:
	cp config.def.h $@

dwm: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f dwm ${OBJ}

.PHONY: all options clean
