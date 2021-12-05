
# Builds NvChad from its git repository

all: cache/nvim

cache/nvim:
	mkdir -p cache
	wget -qO- 'https://github.com/neovim/neovim/releases/latest/download/nvim.appimage' > cache/nvim
	chmod a+x cache/nvim

