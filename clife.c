#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL/SDL.h>
#include <pthread.h>

#include "board.h"

#define DEFAULT_WIDTH  800
#define DEFAULT_HEIGHT 600

#define PAN_SPEED 2

/* Note: this is hard coded, but has a name ;) */
#define THREAD_COUNT 4

int32_t windowWidth, windowHeight, wStartX, wStartY, zoomLevel;
Board *board, *new = NULL, *tmp;
char keyboard[SDLK_LAST];

int stepLife();
void *stepThread(void *args);

SDL_Surface *screen;
Uint32 pOn, pOff;
void initSDL();
void drawGrid();

int main(int argc, char **argv) {
	int i, tAN;
	FILE *in;

	windowWidth  = DEFAULT_WIDTH;
	windowHeight = DEFAULT_HEIGHT;

	if(argc < 2) {
		in = stdin;
		printf("Looking for .rle on input.\n");
	} else {
		in = fopen(argv[1], "r");
		if(!in) {
			fprintf(stderr, "Could not open \"%s\" for reading!\n", argv[1]);
			return 1;
		}
		if(argc > 2) {
			windowWidth = atoi(argv[2]);
			if(windowWidth <= 0)
				windowWidth = DEFAULT_WIDTH;
			if(argc > 3) {
				windowHeight = atoi(argv[3]);
				if(windowHeight <= 0)
					windowHeight = DEFAULT_HEIGHT;
			}
		}
	}

	board = board_readFile(in);
	if(!board)
		return 1;

	printf("Board middle (byte aligned): %d, %d\n",
			(board->width >> 4) << 3, (board->height >> 4) << 3);

	if(argc > 4) {
		board->torodial = 1;
	}

	printf("Initial population is %ld out of %ld area\n",
			board->alive, board_getArea(board));

	board->torodial = 1;

	/*
	if(argc > 2) {
		printf("Board is not torodial\n");
		board->torodial = 0;
	}
	*/

	if((uint32_t)windowWidth > board->width)
		windowWidth = board->width;
	if((uint32_t)windowHeight > board->height)
		windowHeight = board->height;

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
					keyboard[event.key.keysym.sym] = 1;
					switch(event.key.keysym.sym) {
						case SDLK_RETURN: case SDLK_ESCAPE:
							printf("We recieved key input saying to stop.\n");
							i = -1;
							break;
						default:
							break;
					}
					break;
				case SDL_KEYUP:
					keyboard[event.key.keysym.sym] = 0;
					break;
				default:
					break;
			}
		} /* }}} */
		/* If enter/escape was pressed, abort */
		if(i <= 0)
			break;

		if(keyboard[SDLK_LEFT])
			wStartX -= (PAN_SPEED << (3 - zoomLevel));
		if(keyboard[SDLK_RIGHT])
			wStartX += (PAN_SPEED << (3 - zoomLevel));
		if(keyboard[SDLK_DOWN])
			wStartY += (PAN_SPEED << (3 - zoomLevel));
		if(keyboard[SDLK_UP])
			wStartY -= (PAN_SPEED << (3 - zoomLevel));
		if(keyboard[SDLK_a])
			zoomLevel++;
		if(keyboard[SDLK_o])
			zoomLevel--;

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
	pthread_t threads[THREAD_COUNT];
	uint64_t targs[3 * THREAD_COUNT];
	uint32_t midX, midY;

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

	midX = (board->width >> 4) << 3;
	midY = (board->height >> 4) << 3;

	/* top left corner */
	((uint32_t *)targs)[0] = 0;
	((uint32_t *)targs)[1] = 0;
	((uint32_t *)targs)[2] = midX;
	((uint32_t *)targs)[3] = midY;

	/* top right corner */
	((uint32_t *)targs)[6] = midX;
	((uint32_t *)targs)[7] = 0;
	((uint32_t *)targs)[8] = board->width;
	((uint32_t *)targs)[9] = midY;

	/* bottom left corner */
	((uint32_t *)targs)[12] = 0;
	((uint32_t *)targs)[13] = midY;
	((uint32_t *)targs)[14] = midX;
	((uint32_t *)targs)[15] = board->height;

	/* bottom right corner */
	((uint32_t *)targs)[18] = midX;
	((uint32_t *)targs)[19] = midY;
	((uint32_t *)targs)[20] = board->width;
	((uint32_t *)targs)[21] = board->height;

	if(pthread_create(&threads[0], NULL, stepThread, (void *)(&targs[0]))) {
		fprintf(stderr, "thread creation 0 failed\n");
		exit(1);
	}
	if(pthread_create(&threads[1], NULL, stepThread, (void *)(&targs[3]))) {
		fprintf(stderr, "thread creation 1 failed\n");
		exit(1);
	}
	if(pthread_create(&threads[2], NULL, stepThread, (void *)(&targs[6]))) {
		fprintf(stderr, "thread creation 2 failed\n");
		exit(1);
	}
	if(pthread_create(&threads[3], NULL, stepThread, (void *)(&targs[9]))) {
		fprintf(stderr, "thread creation 3 failed\n");
		exit(1);
	}

	if(pthread_join(threads[0], NULL)) {
		fprintf(stderr, "thread 0 join failed\n");
		exit(1);
	}
	if(pthread_join(threads[1], NULL)) {
		fprintf(stderr, "thread 1 join failed\n");
		exit(1);
	}
	if(pthread_join(threads[2], NULL)) {
		fprintf(stderr, "thread 2 join failed\n");
		exit(1);
	}
	if(pthread_join(threads[3], NULL)) {
		fprintf(stderr, "thread 3 join failed\n");
		exit(1);
	}

	board->alive = targs[2] + targs[5] + targs[8] + targs[11];

	tmp = board;
	board = new;
	new = tmp;
	return new->alive;
} /* }}} */
void *stepThread(void *args) { /* {{{ */
	uint32_t x, y, cx, cy, *ends;
	uint64_t *alive;
	char aliveCount;

	ends = (uint32_t *)args;
	alive = &((uint64_t *)args)[2];
	alive = 0;
	for(x = ends[0]; x < ends[2]; ++x) {
		for(y = ends[1]; y < ends[3]; ++y) {
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
				alive++;
		}
	}
	return NULL;
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

	screen = SDL_SetVideoMode(windowWidth, windowHeight, 32, videoFlags);
	if(screen == NULL) {
		fprintf(stderr, "Unable to create plot screen: %s\n", SDL_GetError());
		exit(1);
	}

	zoomLevel = 0;
	wStartX = wStartY = 0;
} /* }}} */
void drawGrid() { /* {{{ */
	uint32_t x, y, *bufp;

	if(zoomLevel < 0)
		zoomLevel = 0;
	if(zoomLevel >= 4)
		zoomLevel = 3;

	if(wStartX > (int32_t)board->width - (windowWidth >> zoomLevel))
		wStartX = (int32_t)board->width - (windowWidth >> zoomLevel) - 1;
	if(wStartX < 0)
		wStartX = 0;

	if(wStartY > (int32_t)board->height - (windowHeight >> zoomLevel))
		wStartY = (int32_t)board->height - (windowHeight >> zoomLevel) - 1;
	if(wStartY < 0)
		wStartY = 0;

	if(SDL_MUSTLOCK(screen)) {
		if(SDL_LockSurface(screen) < 0) {
			fprintf(stderr, "Required to lock screen, but can't\n");
			exit(1);
		}
	}
	for(x = 0; x < (uint32_t)windowWidth && x < board->width; ++x) {
		for(y = 0; y < (uint32_t)windowHeight && y < board->height; ++y) {
			bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
			*bufp = (board_IsOn(board,
				(x + (wStartX << zoomLevel)) >> zoomLevel,
				(y + (wStartY << zoomLevel)) >> zoomLevel)) ?
					pOn : pOff;
		}
	}
	if(SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
	SDL_UpdateRect(screen, 0, 0, windowWidth, windowHeight);
} /* }}} */

