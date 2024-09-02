.DEFAULT_GOAL := bin/main

bin:
	mkdir bin

bin/main: bin main.cpp
	g++ -Wall -O2 -lcurses -o bin/main main.cpp
