# status - status bar updater for FiletLignux
# See LICENSE file for copyright and license details.

include config.mk

SRC = flstatus.c
OBJ = ${SRC:.c=.o}

all: options flstatus

options:
	@echo flstatus build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

flstatus: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f flstatus flstatus.o

.PHONY: all options clean
