cc=g++
cflags=-Wall -g -ggdb -std=c++20
#cflags=-Wall -std=c++20
ldflags=-lSDL2 -lSDL2_ttf -lm 

all: example
example:
	$(cc) -o example example.cpp SDL_console.cpp $(cflags) $(ldflags) 
lib:
	$(cc) -fPIC -shared -o libSDL_console.so SDL_console.cpp $(cflags) $(ldflags)
