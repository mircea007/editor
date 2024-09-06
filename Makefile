.DEFAULT_GOAL := bin/main

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    EXTRA_FLAGS = -Wall -Wextra -O0 -g -fsanitize=address -fsanitize=undefined
else
    EXTRA_FLAGS = -Wall -O2
endif

bin:
	mkdir bin

bin/main: bin main.cpp
	g++ -lncurses $(EXTRA_FLAGS) -o bin/main main.cpp
