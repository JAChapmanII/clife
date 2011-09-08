#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL/SDL.h>

#define MAX_BOARD_WIDTH  (1 << 16)
#define MAX_BOARD_HEIGHT (1 << 16)

uint32_t boardWidth, boardHeight;
uint64_t boardArea, memoryRequirement;
char torodialWorld;

int isDigit(char c) {
	return (
		(c == '1') || (c == '2') || (c == '3') ||
		(c == '4') || (c == '5') || (c == '6') ||
		(c == '7') || (c == '8') || (c == '9') ||
		(c == '0'));
}

void setupConstants(uint32_t width, uint32_t heigth, char torodial) {
	boardWidth  = width;
	boardHeight = heigth;
	boardArea  = (boardWidth*boardHeight);
	memoryRequirement = ((boardArea + 7) >> 3);
	torodialWorld = torodial;
}

/* We still use single dimension arrays so that the bit packing is easier to
 * do, and we already had code to act like it is a 2D array */
char *board = NULL, *new = NULL, *tmp;

/* Functions to check/set/clear "alive"-ness {{{ */
int isAlive(char *p, int x) {
	return (p[x >> 3] & (0x1 << (x % 8))) >> (x % 8);
}
void setAlive(char *p, int x) {
	p[x >> 3] |= (0x1 << (x % 8));
}
void setNotAlive(char *p, int x) {
	if(isAlive(p, x))
		p[x >> 3] -= (0x1 << (x % 8));
} /* }}} */

int readFile(FILE *f);
int stepLife();

SDL_Surface *screen;
Uint32 pOn, pOff;
void initSDL();
void drawGrid();

int main(int argc, char **argv) {
	int i, tAN, aCount, width, height, torodial;
	FILE *in;
	width = height = 768;
	torodial = 1;

	if(argc < 2) {
		in = stdin;
		printf("Looking for .rle on input.\n");
	} else {
		in = fopen(argv[1], "r");
		if(!in) {
			fprintf(stderr, "Could not open \"%s\" for reading!\n", argv[1]);
			return 1;
		}
	}

	aCount = readFile(in);
	printf("Initial population is %d out of %d area\n", aCount, width * height);

	/* Parse arguments as width, height, and not torodial {{{ */
	if(argc > 1) {
		width = height = atoi(argv[1]);
		if(width <= 0)
			width = height = 768;
		if(argc > 2) {
			height = atoi(argv[2]);
			if(height <= 0)
				height = 768;
			if(argc > 3) {
				torodial = 0;
			}
		}
	} /* }}} */

	setupConstants(width, height, torodial);

	initSDL();
	if(screen->format->BytesPerPixel != 4) {
		fprintf(stderr, "Screen not 32-bpp\n");
		return 1;
	}
	printf("Initialized SDL...\n");

	pOn  = SDL_MapRGB(screen->format,   0, 100, 255);
	pOff = SDL_MapRGB(screen->format, 255, 255, 255);
	printf("Constructed colors...\n");

	drawGrid();
	SDL_Delay(1000);

	for(i = 1; i > 0; ++i) {
		/* See if enter or escape was pressed {{{ */
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						case SDLK_RETURN: case SDLK_ESCAPE:
							printf("We recieved key input saying to stop.\n");
							i = -1;
							break;
						default:
							break;
					}
					break;
				default:
					break;
			}
		} /* }}} */
		/* If enter/escape was pressed, abort */
		if(i <= 0)
			break;
		drawGrid();
		/*SDL_Delay(333);*/
		tAN = stepLife();
		if(tAN == 0) {
			printf("All life is dead.\n");
			i = -1;
		}
	}
	printf("Broken from game loop\n");

	if(tAN != 0) {
		SDL_SaveBMP(screen, "out.bmp");
		printf("Saved final output to out.bmp\n");
	}

	return 0;
}

int readFile(FILE *f) { /* {{{ */
	size_t bRead;
	uint32_t i, j, pCount = 0;
	uint32_t xHead, yHead[2], rHead[8];
	/*char yHead[6], rHead[15];*/
	char glob[16];

	/* Verify xHead is present in header */
	bRead = fread(&xHead, 1, 4, f);
	if(bRead != 4) {
		fprintf(stderr, "File is not long enough to have x-header!\n");
		exit(1);
	}
	/*  78  20  3d  20
	 * 'x' ' ' '=' ' ' 
	 * it's backwards because... endianness? TODO */
	if(xHead != 0x203d2078) {
		fprintf(stderr, "File header is incorrect (wrong xHead)!\n");
		fprintf(stderr, "Recieved: %x\n", xHead);
		exit(1);
	}

	/* Read width */
	for(i = 0; i < 16; ++i) {
		glob[i] = fgetc(f);
		if(feof(f)) {
			fprintf(stderr, "Ran out of file while looking for width!\n");
			exit(1);
		}
		if(!isDigit(glob[i])) {
			break;
		}
	}
	if(!i) {
		fprintf(stderr, "Board width is not a number!\n");
		exit(1);
	}
	if(i == 16) {
		fprintf(stderr, "Board width seems to be too many digits long!\n");
		exit(1);
	}
	if(glob[i] != ',') {
		fprintf(stderr, "Board width is not ended properly!\n");
		fprintf(stderr, "Width ended with :'%c'\n", glob[i]);
		exit(1);
	}

	boardWidth = atoi(glob);
	if(boardWidth <= 0) {
		fprintf(stderr, "Board width doesn't parse or is 0!\n");
		exit(1);
	}
	if(boardWidth >= MAX_BOARD_WIDTH) {
		fprintf(stderr, "Board width is too large!\n");
		exit(1);
	}
	printf("Board width: %d\n", boardWidth);

	board = malloc(memoryRequirement);
	if(!board) {
		fprintf(stderr, "Could not allocate enough memory\n");
		exit(1);
	}
	for(i = 0; i < boardArea; ++i)
		setAlive(board, i);

	for(i = 2; i < boardArea; ++i) {
		if(!isAlive(board, i))
			continue;
		pCount++;
		for(j = i * 2; j < boardArea; j += i)
			setNotAlive(board, j);
	}
	return pCount;
} /* }}} */

/*
void writePrimes() {
	uint32_t x, y, cnt;
	char on;
	FILE *f = fopen("primes.rle", "w");
	if(!f) {
		fprintf(stderr, "Could not open primes.rle for writing\n");
		return;
	}

	fprintf(f, "x = %d, y = %d, rule = B3/S23\n", boardWidth, boardHeight);
	for(y = 0; y < boardHeight; ++y) {
		for(x = 0; x < boardWidth; ++x) {
			on = isAlive(prime, y*boardWidth + x);
			for(cnt = 0; (x < boardWidth) && (isAlive(prime, y*boardWidth + x) == on); ++x)
				cnt++;
			if((x != boardWidth - 1) || (isAlive(prime, y*boardWidth + boardWidth) != on))
				--x;
			if(cnt > 1)
				fprintf(f, "%d%c", cnt, (on) ? 'o' : 'b');
			else
				fprintf(f, "%c", (on) ? 'o' : 'b');
		}
		fprintf(f, "$");
	}
	fprintf(f, "!\n");

	fclose(f);
}
*/
int stepLife() { /* {{{ */
	uint32_t x, y, cx, cy, totalAliveNow = 0;
	char aliveCount;

	if(!new) {
		new = malloc(memoryRequirement);
		if(!new) {
			fprintf(stderr, "Could not allocate enough memory to simulate!\n");
			exit(1);
		}
	}

	for(x = 0; x < boardWidth; ++x) {
		for(y = 0; y < boardHeight; ++y) {
			aliveCount = 0;

			cx = x;
			/* Up neighbor */
			cy = y - 1;
			if(torodialWorld && y == 0)
				cy = boardHeight - 1;
			if(torodialWorld || y != 0)
				aliveCount += isAlive(board, cy*boardWidth + cx);

			/* Down neighbor */
			cy = y + 1;
			if(torodialWorld && cy >= boardHeight)
				cy = 0;
			if(cy < boardHeight)
				aliveCount += isAlive(board, cy*boardWidth + cx);

			cy = y;
			/* Left neighbor */
			cx = x - 1;
			if(torodialWorld && x == 0)
				cx = boardWidth - 1;
			if(torodialWorld || x != 0)
				aliveCount += isAlive(board, cy*boardWidth + cx);

			/* Right neighbor */
			cx = x + 1;
			if(torodialWorld && cx >= boardWidth)
				cx = 0;
			if(cx < boardWidth)
				aliveCount += isAlive(board, cy*boardWidth + cx);

			cy = y - 1;
			if(torodialWorld && y == 0)
				cy = boardHeight - 1;
			if(torodialWorld || y != 0) {
				/* Up left neighbor */
				cx = x - 1;
				if(torodialWorld && x == 0)
					cx = boardWidth - 1;
				if(torodialWorld || x != 0)
					aliveCount += isAlive(board, cy*boardWidth + cx);

				/* Up right neighbor */
				cx = x + 1;
				if(torodialWorld && cx >= boardWidth)
					cx = 0;
				if(cx < boardWidth)
					aliveCount += isAlive(board, cy*boardWidth + cx);
			}

			cy = y + 1;
			if(torodialWorld && cy >= boardHeight)
				cy = 0;
			if(cy < boardHeight) {
				/* Down left neighbor */
				cx = x - 1;
				if(torodialWorld && x == 0)
					cx = boardWidth - 1;
				if(torodialWorld || x != 0)
					aliveCount += isAlive(board, cy*boardWidth + cx);

				/* Down right neighbor */
				cx = x + 1;
				if(torodialWorld && cx >= boardWidth)
					cx = 0;
				if(cx < boardWidth)
					aliveCount += isAlive(board, cy*boardWidth + cx);
			}

			setNotAlive(new, y*boardWidth + x);
			if(isAlive(board, y*boardWidth + x) &&
					((aliveCount == 2) || (aliveCount == 3)))
				setAlive(new, y*boardWidth + x);
			if(!isAlive(board, y*boardWidth + x) && (aliveCount == 3))
				setAlive(new, y*boardWidth + x);
			if(isAlive(new, y*boardWidth + x))
				totalAliveNow++;
		}
	}
	tmp = board;
	board = new;
	new = tmp;
	return totalAliveNow;
} /* }}} */

void initSDL() { /* {{{ */
	const SDL_VideoInfo *videoInfo;
	int videoFlags;
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	videoInfo = SDL_GetVideoInfo();
	if(!videoInfo) {
		fprintf(stderr, "Can't query video info: %s\n", SDL_GetError());
		exit(1);
	}

	videoFlags = SDL_HWPALETTE;
	/*videoFlags |= SDL_RESIZABLE; TODO */

	if(videoInfo->hw_available)
		videoFlags |= SDL_HWSURFACE;
	else
		videoFlags |= SDL_SWSURFACE;

	if(videoInfo->blit_hw)
		videoFlags |= SDL_HWACCEL;

	screen = SDL_SetVideoMode(boardWidth, boardHeight, 32, videoFlags);
	if(screen == NULL) {
		fprintf(stderr, "Unable to create plot screen: %s\n", SDL_GetError());
		exit(1);
	}
} /* }}} */
void drawGrid() { /* {{{ */
	uint32_t x, y, *bufp;

	if(SDL_MUSTLOCK(screen)) {
		if(SDL_LockSurface(screen) < 0) {
			fprintf(stderr, "Required to lock screen, but can't\n");
			exit(1);
		}
	}
	for(x = 0; x < boardWidth; ++x) {
		for(y = 0; y < boardHeight; ++y) {
			bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
			*bufp = (isAlive(board, y*boardWidth + x)) ? pOn : pOff;
		}
	}
	if(SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
	SDL_UpdateRect(screen, 0, 0, boardWidth, boardHeight);
} /* }}} */

