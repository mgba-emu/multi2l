enum Save {
	SAVE_NONE = 0,
	SAVE_SRAM = 1,
	SAVE_EEPROM = 2,
	SAVE_FLASH = 4
};

int detectSaveType(void);
u16 detectFlashMfg();
const char* flashMfgName(int id);
