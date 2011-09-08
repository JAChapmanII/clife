LDFLAGS=`sdl-config --libs`
CFLAGS=-pedantic -ansi -Wall -Wextra `sdl-config --cflags`

clife: clife.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o clife

