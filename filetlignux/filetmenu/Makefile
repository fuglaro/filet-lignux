
# Builds filetmenu from its git repository

all: cache/filet-menu
	git -C cache/filet-menu pull
	make -C cache/filet-menu

cache:
	mkdir cache

cache/filet-menu: cache
	git clone https://github.com/fuglaro/filet-menu.git cache/filet-menu

.PHONY: all
