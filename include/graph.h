#include <nds.h>

struct Graph {
	s16 values[256];
	u16 color;
	s16 offset;
	u8 entry;
};

extern struct Graph graphs[2];

void renderGraph(const struct Graph* graph);
void renderGraphs(int bitmask);
