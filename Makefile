.PHONY: clean indent
SHELL := /bin/bash
CFLAGS += -g -O0 -Wall
CFLAGS += `pkg-config --cflags sdl cairo librsvg-2.0`

LIBS += `pkg-config --libs sdl cairo librsvg-2.0`

TARGETS=svgViewer

all: $(TARGETS)

svgViewer: svgViewer.o cairosdl.o
	$(CC) -o svgViewer $+ $(LIBS) 

cairosdl.o: ./cairosdl/cairosdl.c ./cairosdl/cairosdl.h
	$(CC) $(CFLAGS) -c ./cairosdl/cairosdl.c

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $*.c

test: svgViewer
	./svgViewer tiger.svg

force:
	make clean all

clean:
	rm -f *.o *~	

indent: 
	indent -kr -i8 -brf *.c *.cpp *.h
