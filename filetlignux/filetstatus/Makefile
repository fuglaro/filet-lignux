# filetstatus - status bar updater for FiletLignux
# See LICENSE file for copyright and license details.

VERSION = filetstatus-1.0

INCS = -I/usr/X11R6/include

all: filetstatus

.c.o:
	cc -c -std=c99 -pedantic -Wall -Os ${INCS} -DVERSION=\"${VERSION}\" $<

filetstatus: filetstatus.o
	cc -rdynamic -o $@ $? -lX11

clean:
	rm -f filetstatus filetstatus.o

.PHONY: all clean
