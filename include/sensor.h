#include <nds.h>

enum Sensor {
	SENSOR_TILT = 1,
	SENSOR_GYRO = 2,
	SENSOR_LIGHT = 4,
	SENSOR_RTC = 8,
	SENSOR_RUMBLE = 16
};

struct RTCValue {
	u8 year;
	u8 month;
	u8 day;
	u8 dayOfWeek;
	u8 hour;
	u8 minute;
	u8 second;
};

int detectSensors(void);
void testTilt(u16* x, u16* y);
u16 testGyro(void);
int testLight(void);
bool readRTC(struct RTCValue*);
void writeRTC(const struct RTCValue*);
void setVRumble(int rumble);
int setupSensors(void);
