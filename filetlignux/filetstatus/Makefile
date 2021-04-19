# filetstatus - status bar updater for FiletLignux
# See LICENSE file for copyright and license details.

# filetstatus version
VERSION = filetstatus-1.0

# Customize below to fit your system
# includes and libs
INCS = -I/usr/X11R6/include
LDFLAGS = -L/usr/X11R6/lib -lX11

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\"
CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}

# compiler and linker
CC = cc

SRC = filetstatus.c
OBJ = ${SRC:.c=.o}

all: options filetstatus

options:
	@echo filetstatus build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

filetstatus: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f filetstatus filetstatus.o

.PHONY: all options clean
