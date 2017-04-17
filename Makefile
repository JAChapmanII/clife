LDFLAGS=`sdl-config --libs`
CFLAGS=-pedantic -ansi -Wall -Wextra `sdl-config --cflags`

ifndef RELEASE
CFLAGS+=-g
endif

clife: clife.o board.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o clife

