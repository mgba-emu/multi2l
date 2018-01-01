#include "text.h"

#include "fifo3d.h"

void render3DText(const char* str, int x, int y, u16 color) {
	fifo3DPost(FIFO_COMMAND_PACK(REG2ID(MATRIX_CONTROL), REG2ID(MATRIX_IDENTITY), REG2ID(MATRIX_TRANSLATE), REG2ID(MATRIX_SCALE)));
	fifo3DPost(0);
	fifo3DPost(-0x1000);
	fifo3DPost(-0x1000);
	fifo3DPost(0);
	fifo3DPost(0x20000);
	fifo3DPost(0x2AAAA);
	fifo3DPost(0x20000);
	fifo3DPost(FIFO_COMMAND_PACK(REG2ID(MATRIX_CONTROL), REG2ID(MATRIX_IDENTITY), FIFO_COLOR, FIFO_POLY_FORMAT));
	fifo3DPost(2);
	fifo3DPost(color);
	fifo3DPost(0x001F00C0);
	fifo3DPost(FIFO_COMMAND_PACK(FIFO_TEX_FORMAT, FIFO_PAL_FORMAT, FIFO_BEGIN, FIFO_NOP));
	fifo3DPost(0x2E400000);
	fifo3DPost(0);
	fifo3DPost(1);


	while (str && str[0]) {
		char c = str[0] - ' ';
		int cx = c & 0xF;
		int cy = c & ~0xF;
		fifo3DPost(FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX_XY, FIFO_TEX_COORD, FIFO_VERTEX_XY));
		fifo3DPost((cx << 7) | ((cy + 16) << 19));
		fifo3DPost(x | (y << 16));
		fifo3DPost(((cx + 1) << 7) | ((cy + 16) << 19));
		fifo3DPost((x + 8) | (y << 16));
		fifo3DPost(FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX_XY, FIFO_TEX_COORD, FIFO_VERTEX_XY));
		fifo3DPost(((cx + 1) << 7) | (cy << 19));
		fifo3DPost((x + 8) | ((y + 8) << 16));
		fifo3DPost((cx << 7) | (cy << 19));
		fifo3DPost(x | ((y + 8) << 16));
		x += 8;
		++str;
	}
}
