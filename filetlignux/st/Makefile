
# Builds st with a customised config.h

all: st/st

cache:
	mkdir cache

cache/st: cache
	git clone https://git.suckless.org/st cache/st
	ln -s '../../config.h' cache/st/config.h

st/st: cache/st
	git -C cache/st pull
	make -C cache/st

.PHONY: all
