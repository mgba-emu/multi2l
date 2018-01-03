#include <nds.h>

enum TweenType {
	TWEEN_LERP,
	TWEEN_SINE,
	TWEEN_LOGISTIC
};

struct Tween {
	int y0;
	int y1;
	int x0;
	int x1;

	int x;
	float y;
	enum TweenType type;
};

void addTween(struct Tween*);
void removeTween(struct Tween*);
void updateTweens(int x);

void tween(struct Tween* tween, int x);
