#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL/SDL.h>

#include "board.h"

Board *board, *new = NULL, *tmp;

int stepLife();

SDL_Surface *screen;
Uint32 pOn, pOff;
void initSDL();
void drawGrid();

int main(int argc, char **argv) {
	int i, tAN;
	FILE *in;

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

	board = board_readFile(in);
	if(!board)
		return 1;

	printf("Initial population is %ld out of %ld area\n",
			board->alive, board_getArea(board));

	board->torodial = 1;
	if(argc > 2) {
		printf("Board is not torodial\n");
		board->torodial = 0;
	}

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
	SDL_SaveBMP(screen, "initial.bmp");
	printf("Saving to initial.bmp\n");
	SDL_Delay(500);

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

int stepLife() { /* {{{ */
	uint32_t x, y, cx, cy;
	char aliveCount;

	if(!new) {
		new = malloc(sizeof(Board));
		if(!new) {
			fprintf(stderr, "Could not allocate enough memory to simulate!\n");
			exit(1);
		}
		new->width = board->width;
		new->height = board->height;
		new->alive = board->alive;
		new->torodial = board->torodial;
		new->board = malloc(board_getMemoryRequirement(board));
		if(!new->board) {
			fprintf(stderr, "Could not allocate enough memory for new->board!\n");
			exit(1);
		}
	}

	board->alive = 0;
	for(x = 0; x < board->width; ++x) {
		for(y = 0; y < board->height; ++y) {
			aliveCount = 0;

			cx = x;
			/* Up neighbor */
			cy = y - 1;
			if(board->torodial && y == 0)
				cy = board->height - 1;
			if(board->torodial || y != 0)
				aliveCount += board_IsOn(board, cx, cy);

			/* Down neighbor */
			cy = y + 1;
			if(board->torodial && cy >= board->height)
				cy = 0;
			if(cy < board->height)
				aliveCount += board_IsOn(board, cx, cy);

			cy = y;
			/* Left neighbor */
			cx = x - 1;
			if(board->torodial && x == 0)
				cx = board->width - 1;
			if(board->torodial || x != 0)
				aliveCount += board_IsOn(board, cx, cy);

			/* Right neighbor */
			cx = x + 1;
			if(board->torodial && cx >= board->width)
				cx = 0;
			if(cx < board->width)
				aliveCount += board_IsOn(board, cx, cy);

			cy = y - 1;
			if(board->torodial && y == 0)
				cy = board->height - 1;
			if(board->torodial || y != 0) {
				/* Up left neighbor */
				cx = x - 1;
				if(board->torodial && x == 0)
					cx = board->width - 1;
				if(board->torodial || x != 0)
					aliveCount += board_IsOn(board, cx, cy);

				/* Up right neighbor */
				cx = x + 1;
				if(board->torodial && cx >= board->width)
					cx = 0;
				if(cx < board->width)
					aliveCount += board_IsOn(board, cx, cy);
			}

			cy = y + 1;
			if(board->torodial && cy >= board->height)
				cy = 0;
			if(cy < board->height) {
				/* Down left neighbor */
				cx = x - 1;
				if(board->torodial && x == 0)
					cx = board->width - 1;
				if(board->torodial || x != 0)
					aliveCount += board_IsOn(board, cx, cy);

				/* Down right neighbor */
				cx = x + 1;
				if(board->torodial && cx >= board->width)
					cx = 0;
				if(cx < board->width)
					aliveCount += board_IsOn(board, cx, cy);
			}

			board_Set(new, x, y, 0);
			if(board_IsOn(board, x, y) &&
					((aliveCount == 2) || (aliveCount == 3)))
				board_Set(new, x, y, 1);
			if(!board_IsOn(board, x, y) && (aliveCount == 3))
				board_Set(new, x, y, 1);
			if(board_IsOn(new, x, y))
				board->alive++;
		}
	}
	tmp = board;
	board = new;
	new = tmp;
	return new->alive;
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

	screen = SDL_SetVideoMode(board->width, board->height, 32, videoFlags);
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
	for(x = 0; x < board->width; ++x) {
		for(y = 0; y < board->height; ++y) {
			bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
			*bufp = (board_IsOn(board, x, y)) ? pOn : pOff;
		}
	}
	if(SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
	SDL_UpdateRect(screen, 0, 0, board->width, board->height);
} /* }}} */

