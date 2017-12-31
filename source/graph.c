#include <nds.h>

#include "fifo3d.h"
#include "graph.h"

struct Graph graphs[2];

void renderGraph(const struct Graph* graph) {
	fifo3DPost(FIFO_COMMAND_PACK(REG2ID(MATRIX_CONTROL), REG2ID(MATRIX_IDENTITY), REG2ID(MATRIX_TRANSLATE), REG2ID(MATRIX_SCALE)));
	fifo3DPost(0);
	fifo3DPost(-0x1000);
	fifo3DPost(-0x1000);
	fifo3DPost(0);
	fifo3DPost(0x20000);
	fifo3DPost(0x2AAA);
	fifo3DPost(0x20000);
	fifo3DPost(FIFO_COMMAND_PACK(REG2ID(MATRIX_CONTROL), REG2ID(MATRIX_IDENTITY), FIFO_COLOR, FIFO_POLY_FORMAT));
	fifo3DPost(2);
	fifo3DPost(graph->color);
	fifo3DPost(0x001F00C0);
	fifo3DPost(FIFO_COMMAND_PACK(FIFO_BEGIN, FIFO_VERTEX16, FIFO_VERTEX16, FIFO_NOP));
	fifo3DPost(3);

	u16 y = graph->values[graph->entry] + graph->offset;
	fifo3DPost(0xFFFF | ((y - 1) << 16));
	fifo3DPost(0);
	fifo3DPost(0xFFFF | ((y + 1) << 16));
	fifo3DPost(0);

	int i;
	for (i = 0; i < 256; ++i) {
		if (!(i & 1)) {
			fifo3DPost(FIFO_COMMAND_PACK(FIFO_VERTEX_XY, FIFO_VERTEX_XY, FIFO_VERTEX_XY, FIFO_VERTEX_XY));
		}
		u16 x = i;
		y = (graph->values[(i + graph->entry) & 0xFF] + graph->offset);
		fifo3DPost((x & 0xFFFF) | ((y - 16) << 16));
		fifo3DPost((x & 0xFFFF) | ((y + 16) << 16));
	}
}

void renderGraphs(int bitmask) {
	int i;
	for (i = 0; i < 2; ++i) {
		if (bitmask & (1 << i)) {
			renderGraph(&graphs[i]);
		}
	}
}
