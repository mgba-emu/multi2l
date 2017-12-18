#include <nds.h>

enum Sensor {
	SENSOR_TILT = 1,
	SENSOR_GYRO = 2,
	SENSOR_LIGHT = 4,
	SENSOR_RTC = 8,
	SENSOR_RUMBLE = 16
};

int detectSensors(void);
void testTilt(u16* x, u16* y);
u16 testGyro(void);
int testLight(void);
void setVRumble(int rumble);
int setupSensors(void);
