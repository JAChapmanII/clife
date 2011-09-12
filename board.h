#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#define MAX_BOARD_WIDTH  (1 << 16)
#define MAX_BOARD_HEIGHT (1 << 16)

#define board_IsOn(b, x, y) \
	(b->board[((y)*b->width + (x)) >> 3] & (0x1 << (((y)*b->width + (x)) % 8))) >> (((y)*b->width + (x)) % 8)

#define board_Set(b, x, y, state) \
	{ uint64_t n =y*b->width + x; if(state) b->board[n >> 3] |= (0x1 << (n % 8)); \
		else if(board_IsOn(b, x, y)) b->board[n >> 3] -= (0x1 << (n % 8)); }


typedef struct {
	uint32_t width, height;
	uint64_t alive;
	char *board, torodial;
} Board;

/*
inline int board_IsOn(Board *b, uint32_t x, uint32_t y) {
	uint64_t n =y*b->width + x;
	return (b->board[n >> 3] & (0x1 << (n % 8))) >> (n % 8);
}
int board_Set(Board *b, uint32_t x, uint32_t y, char state) {
	uint64_t n =y*b->width + x;
	if(state)
		b->board[n >> 3] |= (0x1 << (n % 8));
	else if(board_IsOn(b, x, y))
		b->board[n >> 3] -= (0x1 << (n % 8));
	return state;
}
*/

Board *board_readFile(FILE *f);

uint64_t board_getArea(Board *b);
uint64_t board_getMemoryRequirement(Board *b);

#endif /* BOARD_H */
