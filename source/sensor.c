#include <nds.h>

static vu8* const SENSOR_BASE = SRAM + 0x8000;
static vu16* const GPIO_DATA = (vu16*) 0x080000C4;
static vu16* const GPIO_DIR = (vu16*) 0x080000C6;
static vu16* const GPIO_CNT = (vu16*) 0x080000C8;
static int lightCount = 0;
static int lightData = 0;

enum Sensor {
	SENSOR_TILT = 1,
	SENSOR_GYRO = 2,
	SENSOR_LIGHT = 4,
	SENSOR_RTC = 8,
	SENSOR_RUMBLE = 16
};

const static struct SensorInfo {
	const char* const code;
	int sensors;
} data[] = {
	{ "KYGE", SENSOR_TILT },
	{ "KYGP", SENSOR_TILT },
	{ "KYGJ", SENSOR_TILT },

	{ "KHPJ", SENSOR_TILT },

	{ "RZWE", SENSOR_GYRO | SENSOR_RUMBLE },
	{ "RZWP", SENSOR_GYRO | SENSOR_RUMBLE },
	{ "RZWJ", SENSOR_GYRO | SENSOR_RUMBLE },

	{ "V49E", SENSOR_RUMBLE },
	{ "V49P", SENSOR_RUMBLE },
	{ "V49J", SENSOR_RUMBLE },

	{ "U3IE", SENSOR_LIGHT | SENSOR_RTC },
	{ "U3IP", SENSOR_LIGHT | SENSOR_RTC },
	{ "U3IJ", SENSOR_LIGHT | SENSOR_RTC },

	{ "U32E", SENSOR_LIGHT | SENSOR_RTC },
	{ "U32P", SENSOR_LIGHT | SENSOR_RTC },
	{ "U32J", SENSOR_LIGHT | SENSOR_RTC },

	{ "U33J", SENSOR_LIGHT | SENSOR_RTC },

	{ 0, 0 }
};

int detectSensors(void) {
	int i;
	for (i = 0; data[i].code; ++i) {
		if (memcmp(data[i].code, GBA_HEADER.gamecode, 4) == 0) {
			return data[i].sensors;
		}
	}
	return 0;
}

static void setupGPIO(int pins) {
	*GPIO_CNT = 1;
	*GPIO_DIR = pins & 0xF;
}

static void setupGyro(void) {
	setupGPIO(0xB);
	u16 exmem = REG_EXMEMCNT;
	exmem &= ~0x7F;
	exmem |= 0x17;
	REG_EXMEMCNT = exmem;
}

u16 testGyro(void) {
	u16 state = *GPIO_DATA & 0x8;
	*GPIO_DATA = state | 3;
	*GPIO_DATA = state | 2;
	int i;
	u16 data = 0;
	for (i = 0; i < 16; ++i) {
		u16 bit = (*GPIO_DATA & 4) >> 2;
		*GPIO_DATA = state;
		data |= bit << (15 - i);
		*GPIO_DATA = state | 2;
	}
	data &= 0xFFF;
	return data;
}

static void setupTilt(void) {
	u16 exmem = REG_EXMEMCNT;
	exmem &= ~0x63;
	exmem |= 0x23;
	REG_EXMEMCNT = exmem;
}

void testTilt(u16* x, u16* y) {
	*SENSOR_BASE = 0x55;
	*(SENSOR_BASE + 0x100) = 0xAA;
	while (!(*(SENSOR_BASE + 0x300) & 0x80));
	*x = *(SENSOR_BASE + 0x200);
	*x += (*(SENSOR_BASE + 0x300) & 0xF) << 8;
	*y = *(SENSOR_BASE + 0x400);
	*y += (*(SENSOR_BASE + 0x500) & 0xF) << 8;
}

static void clearLight(void) {
	*GPIO_DATA = 2;
	*GPIO_DATA = 0;
	lightData = 0;
	lightCount = 0;
}

static void setupLight(void) {
	setupGPIO(0x7);
	clearLight();
}

int testLight(void) {
	*GPIO_DATA = 1;
	*GPIO_DATA = 0;
	lightData += !((*GPIO_DATA & 8) >> 3);
	++lightCount;
	int value = -1;
	if (lightCount == 256) {
		value = lightData;
		clearLight();
	}
	return value;
}

void setVRumble(int rumble) {
	u16 state = *GPIO_DATA & 0x7;
	*GPIO_DATA = state | ((rumble & 1) << 3);
}

int setupSensors(void) {
	int sensors = detectSensors();
	if (sensors & SENSOR_TILT) {
		setupTilt();
	}
	if (sensors & SENSOR_GYRO) {
		setupGyro();
	}
	if (sensors & SENSOR_LIGHT) {
		setupLight();
	}
	return sensors;
}
