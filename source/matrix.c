#include <nds.h>
#include <stdio.h>
#include <fat.h>

static void writeChange(const u32* buffer) {
	// Input registers are at 0x08800000 - 0x088001FF
	*(vu32*) 0x08800184 = buffer[1];
	*(vu32*) 0x08800188 = buffer[2];
	*(vu32*) 0x0880018C = buffer[3];

	*(vu32*) 0x08800180 = buffer[0];
}

static void readChange(void) {
	// Output registers are at 0x08000100 - 0x080001FF
	while (*(vu32*) 0x08000180 & 0x1000); // Busy bit
}

void dumpMatrix(const char* fname) {
	FILE* f = fopen(fname, "wb");

	u32 cmd[4] = {
		0x11, // Command
		0, // ROM address
		0x08001000, // Virtual address
		0x8, // Size (in 0x200 byte blocks)
	};

	size_t i;
	for (i = 0x0; i < 0x04000000; i += 0x1000) {
		cmd[1] = i,
		writeChange(cmd);
		readChange();
		fwrite(GBAROM + (0x1000 >> 1), 0x1000, 1, f);
	}

	fclose(f);
}
