.DEFAULT_GOAL := bin/main

bin:
	mkdir bin

bin/main: bin main.c
	g++ -Wall -O2 -lcurses -o bin/main main.c
