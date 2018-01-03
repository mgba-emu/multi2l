#include <nds.h>

#include <math.h>

#include "tween.h"

#define MAX_TWEENS 64

static struct Tween* _tweens[MAX_TWEENS];
static int _maxTween = 0;

void addTween(struct Tween* tween) {
	if (_maxTween >= MAX_TWEENS) {
		return;
	}
	_tweens[_maxTween] = tween;
	++_maxTween;
}

void removeTween(struct Tween* tween) {
	size_t i;
	for (i = 0; i < _maxTween; ++i) {
		if (_tweens[i] != tween) {
			continue;
		}
		_tweens[i] = _tweens[_maxTween - 1];
		--_maxTween;
		break;
	}
}

void updateTweens(int x) {
	size_t i;
	for (i = 0; i < _maxTween; ++i) {
		tween(_tweens[i], x);
	}
}

void tween(struct Tween* tween, int x) {
	tween->x = x;
	x -= tween->x0;
	if (x <= 0) {
		tween->y = tween->y0;
		return;
	}
	int x1 = tween->x1 - tween->x0;
	if (x >= x1) {
		tween->y = tween->y1;
		return;
	}

	int y1 = tween->y1 - tween->y0;
	float q = x / (float) x1;
	float y = q;

	switch (tween->type) {
	case TWEEN_LERP:
		break;
	case TWEEN_SINE:
		y = (1.f - cosf(q * (float) M_PI)) / 2.f;
		break;
	case TWEEN_LOGISTIC:
		y = 1.f / (1.f + expf(12.f * (0.5f - q)));
		break;
	}
	tween->y = (y * y1) + tween->y0;
}
