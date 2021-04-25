
# Builds mimi from its git repository

all: cache/mimi
	git -C cache/mimi pull

cache:
	mkdir cache

cache/mimi: cache
	git clone https://github.com/march-linux/mimi.git cache/mimi

.PHONY: all
