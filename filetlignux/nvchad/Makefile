
# Builds NvChad from its git repository

all: cache/nvchad
	git -C cache/nvchad pull

cache/nvchad:
	mkdir -p cache
	git clone https://github.com/NvChad/NvChad cache/nvchad

