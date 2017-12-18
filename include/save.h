#include <nds.h>

enum Save {
	SAVE_NONE = 0,
	SAVE_SRAM = 1,
	SAVE_EEPROM = 2,
	SAVE_FLASH = 4
};

struct SaveCharacteristics {
	enum Save type;
	u32 size;
	u16 flashId;
	const char* flashMfg;
};

int detectSaveType(struct SaveCharacteristics*);
const char* flashMfgName(int id);
u32 flashSize(int id);
