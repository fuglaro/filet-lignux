
# Builds filetwm from its git repository

all: filet-wm/filetwm

cache:
	mkdir cache

cache/filet-wm: cache
	git clone https://github.com/fuglaro/filet-wm.git cache/filet-wm

filet-wm/filetwm: cache/filet-wm
	git -C cache/filet-wm pull
	make -C cache/filet-wm

.PHONY: all
