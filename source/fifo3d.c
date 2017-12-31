#include <nds.h>

#include "fifo3d.h"

static u32 _fifo[0x100];
static u32 _fifoEntry = 0;

void fifo3DPost(u32 cmd) {
	if ((_fifoEntry & 0x7F) == 112) {
		fifo3DFlush();
	}
	_fifo[_fifoEntry] = cmd;
	++_fifoEntry;
}

void fifo3DFlush(void) {
	while (DMA_CR(0) & DMA_BUSY);

	int count = (_fifoEntry & 0x7F);
	if (!count) {
		return;
	}
	DC_FlushRange(&_fifo[_fifoEntry & 0x80], count * 4);
	DMA_SRC(0) = (u32) &_fifo[_fifoEntry & 0x80];
	DMA_DEST(0) = 0x4000400;
	DMA_CR(0) = DMA_FIFO | count;

	_fifoEntry &= 0x80;
	_fifoEntry ^= 0x80;
}
