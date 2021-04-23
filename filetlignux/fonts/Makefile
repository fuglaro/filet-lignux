
# Builds filetwm from its git repository

all: cache/JetBrainsMono
	git -C cache/JetBrainsMono pull

cache:
	mkdir cache

cache/JetBrainsMono: cache
	git clone https://github.com/JetBrains/JetBrainsMono.git cache/JetBrainsMono


.PHONY: all
