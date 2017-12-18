#include <fat.h>
#include <nds.h>
#include <stdio.h>

#include "save.h"

vu8* const FLASH_BASE_1 = SRAM + 0x5555;
vu8* const FLASH_BASE_2 = SRAM + 0x2AAA;
vu8* const SRAM_BASE = SRAM;

#define SIZE_SRAM 0x8000
#define SIZE_FLASH 0x10000

u8 saveBackup[0x20000];

static void bytecopy(const u8* src, vu8* dst, size_t size) {
	size_t i;
	for (i = 0; i < size; ++i) {
		dst[i] = src[i];
	}
}

static void flashCommand(u8 cmd) {
	*FLASH_BASE_1 = 0xAA;
	*FLASH_BASE_2 = 0x55;
	*FLASH_BASE_1 = cmd;
}

static void switchBank(int bank) {
	flashCommand(0xB0);
	*SRAM_BASE = bank;
}

static void writeSector(int sector, const u8* bytes) {
	flashCommand(0x80);
	*FLASH_BASE_1 = 0xAA;
	*FLASH_BASE_2 = 0x55;
	vu8* base = &SRAM_BASE[sector << 12];
	*base = 0x30;
	while (*base != 0xFF);
	bytecopy(bytes, base, 0x1000);
}

u16 detectFlashMfg(void) {
	flashCommand(0x90);
	u16 id = SRAM_BASE[0] | (SRAM_BASE[1] << 8);
	flashCommand(0xF0);
	return id;
}

void destroyFlash(int banks) {
	flashCommand(0x80);
	flashCommand(0x10);
	while (*SRAM_BASE != 0xFF) {
		swiWaitForVBlank();
	}

	if (banks < 2) {
		return;
	}

	flashCommand(0xB0);
	*SRAM_BASE = 1;

	flashCommand(0x80);
	flashCommand(0x10);
	while (*SRAM_BASE != 0xFF) {
		swiWaitForVBlank();
	}
}

void testFlashTimings(void) {
	int i;
	for (i = 0; i < 16; ++i) {
		u32 addr;
		u32 eraseCycles, burnCycles;
		vu8* base = &SRAM_BASE[i << 12];
		volatile int timeout;
		bool timedOut = false;
		flashCommand(0x80);
		*FLASH_BASE_1 = 0xAA;
		*FLASH_BASE_2 = 0x55;
		TIMER0_DATA = 0;
		TIMER0_CR = TIMER_ENABLE;
		*base = 0x30;
		for (timeout = 0; *base != 0xFF; ++timeout) {
			if (timeout >= 500000) {
				timedOut = true;
				break;
			}
		}
		eraseCycles = TIMER0_DATA;
		if (timedOut) {
			printf("E Timeout! Incorrect value: %02X\n", *base);
		}
		TIMER0_CR = 0;
		TIMER0_CR = TIMER_ENABLE;
		for (addr = 0; addr < 0x1000; ++addr) {
			base[addr] = 0x80;
		}
		burnCycles = TIMER0_DATA;
		TIMER0_CR = 0;
		printf("Erase: %lu, program: %lu\n", eraseCycles, burnCycles);
	}
}

const char* flashMfgName(int id) {
	id = detectFlashMfg();
	const char* mfg = "Unknown";

	switch (id) {
	case 0xD4BF:
		mfg = "SST";
		break;
	case 0x1CC2:
		mfg = "Macronix";
		break;
	case 0x1B32:
		mfg = "Panasonic";
		break;
	case 0x3D1F:
		mfg = "Atmel";
		break;
	case 0x1362:
		mfg = "Sanyo";
		break;
	case 0x09C2:
		mfg = "Macronix";
		break;
	}
	return mfg;
}

int detectSaveType(void) {
	u8 b = *SRAM_BASE;
	*SRAM_BASE = 0x80;

	int tries;
	for (tries = 0; tries < 1000; ++tries) {
		if (*SRAM_BASE == 0x80) {
			*SRAM_BASE = b;
			return SAVE_SRAM;
		} else if (detectFlashMfg() != 0xFFFF) {
			return SAVE_FLASH;
		}
	}
	return SAVE_EEPROM;
}
