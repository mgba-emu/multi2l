#include <nds.h>

#include <math.h>
#include <stdio.h>

#include "fifo3d.h"
#include "graph.h"
#include "matrix.h"
#include "menu.h"
#include "save.h"
#include "sensor.h"
#include "text.h"
#include "tween.h"

#include "font.h"

static u16* textBase[2] = {
	(u16*) 0x06002000,
	(u16*) 0x06202000,
};
char textGrid[2][24 * 32];

static bool cartInserted = false;
static char cartCode[7] = "";
static char cartTitle[13] = "";
static struct SaveCharacteristics cartSave = {};
static int cartSensors = 0;
static int cartLight = 0;
static u32 cartSize = 0;

enum View;
static void setView(enum View view);

int frame = 0;

static enum View {
	VIEW_MENU,
	VIEW_SENSORS
} currentView = VIEW_MENU;

static struct MenuEntry _rootMenu[] = {
	{ "Dump cartridge", false, -1, NULL, NULL },
	{ "Save tools", false, -1, NULL, NULL },
	{ "Sensors", true, -1, (void (*)(void*)) setView, (void*) VIEW_SENSORS },
};

static struct MenuEntry _backMenu[] = {
	{ "Back", true, -2, (void (*)(void*)) setView, (void*) VIEW_MENU },
};

static const char* viewName(enum View view) {
	switch (view) {
	case VIEW_SENSORS:
		return "Sensors";
	case VIEW_MENU:
		break;
	}
	return NULL;
}

static void setView(enum View view) {
	currentView = view;
}

static void updateTextGrid(void) {
	int i;
	for (i = 0; i < 24 * 32; ++i) {
		textBase[1][i] = textGrid[0][i] ? textGrid[0][i] - ' ' : 0;
		textBase[0][i] = textGrid[1][i] ? textGrid[1][i] - ' ' : 0;
	}
}

static void cartridgeHeartbeat(void) {
	// Load the GBA BIOS access pattern. This can unlock some pirate carts.
	const u16 table[] =  {
		0x479B, 0x7426, 0x11BC, 0x6D4F, 0x11BD, 0x32F1, 0x7FD9, 0x2CE7,
		0x5DA5, 0x11BD, 0x4610, 0x5DA4, 0x4E90, 0x6173, 0x2A84, 0x4E91,
		0x106A, 0x75FE, 0x29C8, 0x7839, 0x420E, 0x5D1B, 0x7838, 0x12A8,
		0x3F7D, 0x67B9, 0x26F3, 0x54EF, 0x7C23, 0x26F2, 0x6BC6, 0x4137,
		0x15AB, 0x730D, 0x6BC7, 0x3B4F, 0x5F24, 0x3DDA, 0x253F, 0x1749,
		0x3DDB, 0x70E6, 0x746C, 0x30F7, 0x531F, 0x6738, 0x531E, 0x1A51,
		0x1971, 0x5B7D, 0x4ED6, 0x1970, 0x3F27, 0x75CB, 0x3D62, 0x128C,
		0x74B8, 0x2FAD, 0x74B9, 0x64FD, 0x6C9A, 0x4F3A, 0x276D, 0x73EF,
		0x38B1, 0x4F3B, 0x571E, 0x7EA3, 0x6249, 0x3587, 0x1B7C, 0x3586,
		0x7AFB, 0x67E4, 0x5C92, 0x67E5, 0x2BCA, 0x438C, 0x2E6F, 0x587F,
		0x14B7, 0x2E6E, 0x4CB9, 0x6FA2, 0x38F0, 0x719E, 0x475A, 0x1F3C,
		0x6AD8, 0x475B, 0x5199, 0x3264, 0x7B41, 0x49EF, 0x5198, 0x1CD7,
	};

	vu8* gbarom = (vu8*) GBA_BUS;
	u8 b4 = gbarom[0xb4];
	if (b4 == 0xDA) {
		cartInserted = false;
		return;
	}

	if (!cartInserted) {
		vu16* hibase = (vu16*) (gbarom + 0x1FFFFE0);
		hibase[0];
		hibase[1];
		hibase[2];
		hibase[3];
		hibase[4];
		hibase[5];
		hibase[6];
		hibase[7];
		hibase[8];
		hibase[9];

		// 06AC
		int x;
		u32 hash = 0;
		for (x = 0x9D; x < 0xB8; ++ x) {
			u8 b = gbarom[x];
			hash = (hash >> 3) | (hash << 29);
			hash ^= b;
			b <<= 8;
			hash ^= b;
			b <<= 8;
			hash ^= b;
			b <<= 8;
			hash ^= b;
		}
		hash <<= 27;
		hash >>= 30;

		// 05B8
		int tindex = 6 * hash + (gbarom[0x9E] & 3) * 0x18;

		for (x = 2; x < 42; ++x) {
			GBA_BUS[x];
		}

		int y, z;
		for (y = 0; y < 6; ++y) {
			++tindex;
			u32 address = table[tindex];
			GBA_BUS[address];
			for (z = 10 * y; z < 10 * (y + 1); ++z) {
				GBA_BUS[x + z];
			}
		}
		for (z = 10 * y; z < 10 * (y + 1); ++z) {
			GBA_BUS[x + z];
		}
	}

	if (!GBA_BUS[0]) {
		cartInserted = false;
		return;
	}

	cartInserted = true;

	memcpy(cartCode, GBA_HEADER.gamecode, 6);
	memcpy(cartTitle, GBA_HEADER.title, 12);

	detectSaveType(&cartSave);
	cartSensors = setupSensors();
	cartLight = 0;
	cartSize = 0;
	if (detectMatrix()) {
		cartSize = 64 * 1024 * 1024;
	} else {
		int border = 0x800000;
		int end;
		for (end = 0; end < 4; ++end) {
			u16 bytes = GBAROM[border];
			if (bytes) {
				cartSize = border << 2;
				break;
			}
			if (GBAROM[border * 2 - 1] != 0xFFFF) {
				cartSize = border << 2;
				break;
			}
			border >>= 1;
		}
	}
}

static void heartbeat(void) {
	cartridgeHeartbeat();

	int graphsUsed = 0;
	memset(&textGrid[0][0], 0, sizeof(textGrid[0]));
	strcpy(&textGrid[0][0], "Slot-2: ");
	if (cartInserted) {
		strcpy(&textGrid[0][8], "Inserted");
		sprintf(&textGrid[0][32], "ID: %s", cartCode);
		sprintf(&textGrid[0][64], "Title: %s", cartTitle);
		sprintf(&textGrid[0][96], "Size: %ld", cartSize);
		strcpy(&textGrid[0][128], "Save type: ");
		switch (cartSave.type) {
		case SAVE_SRAM:
			strcpy(&textGrid[0][139], "SRAM/FRAM");
			break;
		case SAVE_FLASH:
			sprintf(&textGrid[0][139], "Flash (%04X, %s)", cartSave.flashId, cartSave.flashMfg);
			break;
		case SAVE_EEPROM:
			strcpy(&textGrid[0][139], "EEPROM");
			break;
		default:
			strcpy(&textGrid[0][139], "None");
			break;
		}
		sprintf(&textGrid[0][160], "Save size: %ld", cartSave.size);
		strcpy(&textGrid[0][192], "Sensors:");
		if (cartSensors) {
			int tile = 226;
			if (cartSensors & SENSOR_TILT) {
				u16 x, y;
				testTilt(&x, &y);
				sprintf(&textGrid[0][tile], "Tilt: %X, %X", x, y);
				tile += 32;
				graphs[0].offset = 0x380;
				graphs[0].entry = frame;
				graphs[0].values[(frame - 1) & 0xFF] = x - 0x370;
				graphs[1].offset = 0x780;
				graphs[1].entry = frame;
				graphs[1].values[(frame - 1) & 0xFF] = y - 0x370;
				graphsUsed = 3;
			}
			if (cartSensors & SENSOR_GYRO) {
				u16 z = testGyro();
				sprintf(&textGrid[0][tile], "Gyroscope: %X", z);
				tile += 32;
				graphs[0].offset = 0x580;
				graphs[0].entry = frame;
				graphs[0].values[(frame - 1) & 0xFF] = z - 0x6C0;
				graphsUsed = 1;
			}
			if (cartSensors & SENSOR_LIGHT) {
				int light = testLight();
				if (light >= 0) {
					cartLight = light;
				}
				sprintf(&textGrid[0][tile], "Light: %X", cartLight);
				tile += 32;
			}
			if (cartSensors & SENSOR_RTC) {
				struct RTCValue rtc;
				if (readRTC(&rtc)) {
					sprintf(&textGrid[0][tile], "RTC: 20%02d-%02d-%02d @ %02d:%02d:%02d", rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second);
				} else {
					sprintf(&textGrid[0][tile], "RTC: Battery dead?");
				}
				tile += 32;
			}
			if (cartSensors & SENSOR_RUMBLE) {
				sprintf(&textGrid[0][tile], "Rumble: Present");
				tile += 32;
			}
		} else {
			strcpy(&textGrid[0][201], "None");
		}
	} else {
		strcpy(&textGrid[0][8], "Empty");
	}
	if (!graphsUsed) {
		graphs[0].offset = 0x580;
		graphs[0].entry = frame;
		graphs[0].values[(frame - 1) & 0xFF] = (sinf(frame * 0.063f) * 256);
		graphsUsed = 1;
	}
	if (!cartInserted || currentView == VIEW_SENSORS) {
		renderGraphs(graphsUsed);
	}

	updateTweens(frame);
	int bounce = (frame & 0x3F) - 0x20;
	bounce *= bounce;
	bounce = (0x400 - bounce) >> 6;

	if (cartInserted) {
		scanKeys();
		u16 keys = keysDown();
		u16 repeat = keysDownRepeat();

		if (repeat & KEY_DOWN) {
			cursorMove(CUR_DOWN);
		} else if (repeat & KEY_UP) {
			cursorMove(CUR_UP);
		}

		if (repeat & KEY_LEFT) {
			cursorMove(CUR_LEFT);
		} else if (repeat & KEY_RIGHT) {
			cursorMove(CUR_RIGHT);
		}

		if (keys & KEY_A) {
			menuActivate();
		}
		if (keys & KEY_B) {
			menuBack();
		}

		if (repeat) {
			updateTweens(frame);
		}

		renderMenu();
		const char* name = viewName(currentView);
		if (name) {
			render3DText(name, 128 - strlen(name) * 4, 3 + (bounce >> 2), 0);
		}
	} else {
		render3DText("Please insert cartridge", 69, 3 + bounce, 0);
	}

	render3DText("Multi2l", 2, 2, 0x421 * (30 - (bounce >> 2)));

	fifo3DFlush();
	++frame;
	updateTextGrid();
	GFX_FLUSH = 0;
}

int main(void) {
	lcdMainOnBottom();
	videoSetMode(MODE_1_3D);
	videoSetModeSub(MODE_0_2D);
	vramSetPrimaryBanks(VRAM_A_MAIN_BG, VRAM_B_MAIN_SPRITE, VRAM_C_SUB_BG, VRAM_D_LCD);
	vramSetBanks_EFG(VRAM_E_MAIN_SPRITE, VRAM_F_LCD, VRAM_G_LCD);

	int i;
	for (i = 0; i < 96; ++i) {
		((u32*) VRAM_D)[(i & ~0xF) * 8 + (i & 0xF)] = fontTiles[i * 8];
		((u32*) VRAM_D)[(i & ~0xF) * 8 + (i & 0xF) + 0x10] = fontTiles[i * 8 + 1];
		((u32*) VRAM_D)[(i & ~0xF) * 8 + (i & 0xF) + 0x20] = fontTiles[i * 8 + 2];
		((u32*) VRAM_D)[(i & ~0xF) * 8 + (i & 0xF) + 0x30] = fontTiles[i * 8 + 3];
		((u32*) VRAM_D)[(i & ~0xF) * 8 + (i & 0xF) + 0x40] = fontTiles[i * 8 + 4];
		((u32*) VRAM_D)[(i & ~0xF) * 8 + (i & 0xF) + 0x50] = fontTiles[i * 8 + 5];
		((u32*) VRAM_D)[(i & ~0xF) * 8 + (i & 0xF) + 0x60] = fontTiles[i * 8 + 6];
		((u32*) VRAM_D)[(i & ~0xF) * 8 + (i & 0xF) + 0x70] = fontTiles[i * 8 + 7];
	}

	VRAM_F[0] = 0x0000;
	VRAM_F[1] = 0x7FFF;

	vramSetBankD(VRAM_D_TEXTURE_SLOT0);
	vramSetBankF(VRAM_F_TEX_PALETTE);

	sysSetCartOwner(BUS_OWNER_ARM9);

	setBackdropColor(0x7FFF);
	setBackdropColorSub(0x7FFF);
	dmaCopyHalfWords(3, fontTiles, BG_GFX, fontTilesLen);
	dmaCopyHalfWords(3, fontTiles, BG_GFX_SUB, fontTilesLen);

	glInit();
	glClearColor(0xFF, 0xFF, 0xFF, 0);
	glClearDepth(0x7FFF);
	glClearPolyID(0x3F);
	glViewport(0, 0, 255, 191);
	glEnable(GL_TEXTURE_2D);

	int rootMenu = registerMenu(_rootMenu, 3);
	int backMenu = registerMenu(_backMenu, 1);
	_rootMenu[2].submenu = backMenu;
	setMenu(rootMenu);

	bgInit(1, BgType_Text4bpp, BgSize_T_256x256, 4, 0);
	bgInitSub(0, BgType_Text4bpp, BgSize_T_256x256, 4, 0);

	irqInit();
	irqSet(IRQ_VBLANK, heartbeat);
	irqEnable(IRQ_VBLANK);

	graphs[0].color = 0;
	graphs[0].entry = 0;
	graphs[1].color = 0;
	graphs[1].entry = 0;

	while (1) {
		swiWaitForVBlank();
	}
}
