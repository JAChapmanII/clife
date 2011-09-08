#ifndef BOARD_H
#define BOARD_H

#define MAX_BOARD_WIDTH  (1 << 16)
#define MAX_BOARD_HEIGHT (1 << 16)

typedef struct {
	uint32_t width, height;
	uint64_t alive;
	char *board, torodial;
} Board;

Board *board_readFile(FILE *f);

int board_IsOn(Board *b, uint32_t x, uint32_t y);
int board_Set(Board *b, uint32_t x, uint32_t y, char state);

uint64_t board_getArea(Board *b);
uint64_t board_getMemoryRequirement(Board *b);

#endif /* BOARD_H */
