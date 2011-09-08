#ifndef BOARD_H
#define BOARD_H

#define MAX_BOARD_WIDTH  (1 << 16)
#define MAX_BOARD_HEIGHT (1 << 16)

typedef struct {
	uint32_t boardWidth, boardHeight;
	uint64_t boardArea, memoryRequirement, aliveCount;
	char *board, torodial;
} Board;

Board *board_readFile(FILE *f);

int board_IsOn(Board *b, uint64_t n);
int board_Set(Board *b, uint64_t n, char state);

#endif /* BOARD_H */
