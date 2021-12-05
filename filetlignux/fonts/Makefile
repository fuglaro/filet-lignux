
# Builds NvChad from its git repository

all: cache/fonts

cache/fonts:
	mkdir -p cache/fonts
	wget -qO- 'https://github.com/ryanoasis/nerd-fonts/releases/latest/download/CascadiaCode.zip' | busybox unzip -d cache/fonts -

