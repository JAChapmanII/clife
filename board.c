#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "board.h"

int isDigit(char c) {
	return (
		(c == '1') || (c == '2') || (c == '3') ||
		(c == '4') || (c == '5') || (c == '6') ||
		(c == '7') || (c == '8') || (c == '9') ||
		(c == '0'));
}

int board_IsOn(Board *b, uint32_t x, uint32_t y) {
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

int parseWidthHeader(FILE *f, Board *b) { /* {{{ */
	uint32_t i, head;
	char glob[16];
	size_t bRead;

	if(!f || !b)
		return -1;

	/* Verify xHead is present in header */
	bRead = fread(&head, 1, 4, f);
	if(bRead != 4) {
		fprintf(stderr, "File is not long enough to have x-header!\n");
		return 1;
	}
	/*  78  20  3d  20
	 * 'x' ' ' '=' ' '
	 * it's backwards because... endianness? TODO */
	if(head != 0x203d2078) {
		fprintf(stderr, "File header is incorrect (wrong xHead)!\n");
		fprintf(stderr, "Recieved: %x\n", head);
		return 1;
	}

	/* Read width */
	for(i = 0; i < 16; ++i) {
		glob[i] = fgetc(f);
		if(feof(f)) {
			fprintf(stderr, "Ran out of file while looking for width!\n");
			return 1;
		}
		if(!isDigit(glob[i])) {
			break;
		}
	}
	if(!i) {
		fprintf(stderr, "Board width is not a number!\n");
		return 1;
	}
	if(i == 16) {
		fprintf(stderr, "Board width seems to be too many digits long!\n");
		return 1;
	}
	if(glob[i] != ',') {
		fprintf(stderr, "Board width is not ended properly!\n");
		fprintf(stderr, "Width ended with :'%c'\n", glob[i]);
		return 1;
	}

	b->width = atoi(glob);
	if(b->width <= 0) {
		fprintf(stderr, "Board width doesn't parse or is 0!\n");
		return 1;
	}
	if(b->width >= MAX_BOARD_WIDTH) {
		fprintf(stderr, "Board width is too large!\n");
		return 1;
	}
	return 0;
} /* }}} */
int parseHeightHeader(FILE *f, Board *b) { /* {{{ */
	uint32_t i, head;
	char glob[16];
	size_t bRead;

	if(!f || !b)
		return -1;

	/* Verify yHead is present in header */
	glob[0] = fgetc(f);
	if(glob[0] != ' ') {
		fprintf(stderr, "Malformed header after width!\n");
		fprintf(stderr, "There needs to be a space, not a '%c'\n", glob[0]);
		return 1;
	}
	bRead = fread(&head, 1, 4, f);
	if(bRead != 4) {
		fprintf(stderr, "File is not long enough to have y-header!\n");
		return 1;
	}
	/*  79  20  3d  20
	 * 'y' ' ' '=' ' '
	 * it's backwards because... endianness? TODO */
	if(head != 0x203d2079) {
		fprintf(stderr, "File header is incorrect (wrong yHead)!\n");
		fprintf(stderr, "Recieved: %x\n", head);
		return 1;
	}

	/* Read height */
	for(i = 0; i < 16; ++i) {
		glob[i] = fgetc(f);
		if(feof(f)) {
			fprintf(stderr, "Ran out of file while looking for width!\n");
			return 1;
		}
		if(!isDigit(glob[i])) {
			break;
		}
	}
	if(!i) {
		fprintf(stderr, "Board height is not a number!\n");
		return 1;
	}
	if(i == 16) {
		fprintf(stderr, "Board height seems to be too many digits long!\n");
		return 1;
	}
	if(glob[i] != ',') {
		fprintf(stderr, "Board height is not ended properly!\n");
		fprintf(stderr, "Height ended with :'%c'\n", glob[i]);
		return 1;
	}

	b->height = atoi(glob);
	if(b->width <= 0) {
		fprintf(stderr, "Board height doesn't parse or is 0!\n");
		return 1;
	}
	if(b->height >= MAX_BOARD_HEIGHT) {
		fprintf(stderr, "Board height is too large!\n");
		return 1;
	}
	return 0;
} /* }}} */
int parseRuleHeader(FILE *f, Board *b) { /* {{{ */
	uint32_t head;
	char glob[16];
	size_t bRead;

	if(!f || !b)
		return -1;

	/* Verify rHead is present */
	glob[0] = fgetc(f);
	if(glob[0] != ' ') {
		fprintf(stderr, "Malformed header after height!\n");
		fprintf(stderr, "There needs to be a space, not a '%c'\n", glob[0]);
		return 1;
	}
	glob[0] = fgetc(f);
	if(glob[0] != 'r') {
		fprintf(stderr, "Malformed header after height!\n");
		fprintf(stderr, "There needs to be a space, not a '%c'\n", glob[0]);
		return 1;
	}

	bRead = fread(&head, 1, 4, f);
	if(bRead != 4) {
		fprintf(stderr, "File is not long enough to have 1-r-header!\n");
		return 1;
	}
	/*  75  6c  65  20
	 * 'u' 'l' 'e' ' '
	 * it's backwards because... endianness? TODO */
	if(head != 0x20656c75) {
		fprintf(stderr, "File header is incorrect (wrong 1-rHead)!\n");
		fprintf(stderr, "Recieved: %x\n", head);
		return 1;
	}

	bRead = fread(&head, 1, 4, f);
	if(bRead != 4) {
		fprintf(stderr, "File is not long enough to have 2-r-header!\n");
		return 1;
	}
	/*  3d  20  42  33
	 * '=' ' ' 'B' '3'
	 * it's backwards because... endianness? TODO */
	if(head != 0x3342203d) {
		fprintf(stderr, "File header is incorrect (wrong 2-rHead)!\n");
		fprintf(stderr, "Recieved: %x\n", head);
		return 1;
	}

	bRead = fread(&head, 1, 4, f);
	if(bRead != 4) {
		fprintf(stderr, "File is not long enough to have 3-r-header!\n");
		return 1;
	}
	/*  2f  53  32  33
	 * '/' 'S' '2' '3'
	 * it's backwards because... endianness? TODO */
	if(head != 0x3332532f) {
		fprintf(stderr, "File header is incorrect (wrong 3-rHead)!\n");
		fprintf(stderr, "Recieved: %x\n", head);
		return 1;
	}
	printf("rHead is correct\n");

	glob[0] = fgetc(f);
	if(glob[0] != '\n') {
		fprintf(stderr, "Malformed header after rHead!\n");
		fprintf(stderr, "There needs to be a newline, not a '%c'\n", glob[0]);
		return 1;
	}
	return 0;
} /* }}} */

Board *board_readFile(FILE *f) { /* {{{ */
	uint32_t i;
	Board *b;

	b = malloc(sizeof(Board));
	if(!b) {
		fprintf(stderr, "Could not allocate space for board structure!\n");
		return 0;
	}

	if(parseWidthHeader(f, b)) {
		free(b);
		return 0;
	}
	printf("Board width: %d\n", b->width);
	if(parseHeightHeader(f, b)) {
		free(b);
		return 0;
	}
	printf("Board height: %d\n", b->height);

	if(parseRuleHeader(f, b)) {
		free(b);
		return 0;
	}

	printf("Allocating %ld bytes for board\n", board_getMemoryRequirement(b));
	b->board = malloc(board_getMemoryRequirement(b));
	if(!b->board) {
		fprintf(stderr, "Could not allocate enough memory\n");
		free(b);
		return 0;
	}

	for(i = 0; i < b->height; ++i) {
	}

	return b;
} /* }}} */

uint64_t board_getArea(Board *b) {
	return (uint64_t)b->width * b->height;
}
uint64_t board_getMemoryRequirement(Board *b) {
	return (board_getArea(b) + 7) >> 3;
}

/*
void writePrimes() {
	uint32_t x, y, cnt;
	char on;
	FILE *f = fopen("primes.rle", "w");
	if(!f) {
		fprintf(stderr, "Could not open primes.rle for writing\n");
		return;
	}

	fprintf(f, "x = %d, y = %d, rule = B3/S23\n", width, height);
	for(y = 0; y < height; ++y) {
		for(x = 0; x < width; ++x) {
			on = isAlive(prime, y*width + x);
			for(cnt = 0; (x < width) && (isAlive(prime, y*width + x) == on); ++x)
				cnt++;
			if((x != width - 1) || (isAlive(prime, y*width + width) != on))
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

