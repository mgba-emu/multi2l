#include <nds.h>

#include <math.h>

#include "fifo3d.h"
#include "menu.h"
#include "text.h"
#include "tween.h"

#define MAX_MENUS 32

struct Menu {
	const struct MenuEntry* entries;
	size_t nEntries;
	int active;
	int oldActive;
};

static struct Menu _menus[MAX_MENUS];
static int _maxMenu = 0;

static int _activeMenu = 0;
static int _menuStack[MAX_MENUS];
static int _menuStackPointer = 0;

static struct Tween _menuTween = {
	.x = -1,
	.type = TWEEN_LOGISTIC
};

static struct Tween _oldMenuTween = {
	.x = -1,
	.type = TWEEN_LOGISTIC
};

extern int frame;

int registerMenu(const struct MenuEntry* entries, size_t nEntries) {
	if (_maxMenu >= MAX_MENUS) {
		return -1;
	}
	_menus[_maxMenu].entries = entries;
	_menus[_maxMenu].nEntries = nEntries;
	_menus[_maxMenu].active = 0;
	_menus[_maxMenu].oldActive = 0;
	return _maxMenu++;
}

void setMenu(int menuid) {
	_activeMenu = menuid;

	if (_menuTween.x < 0) {
		_menuTween.x = 0;
		_menuTween.y0 = 10;
		_menuTween.y1 = 10;
		addTween(&_menuTween);
	}

	if (_oldMenuTween.x < 0) {
		_oldMenuTween.x = 0;
		_oldMenuTween.y0 = 10;
		_oldMenuTween.y1 = 10;
		addTween(&_oldMenuTween);
	}
}

void pushMenu(int menuid) {
	if (_menuStackPointer >= MAX_MENUS) {
		return;
	}
	_menuStack[_menuStackPointer] = _activeMenu;
	++_menuStackPointer;
	setMenu(menuid);
}

void popMenu(void) {
	if (_menuStackPointer > 0) {
		--_menuStackPointer;
	}
	setMenu(_menuStack[_menuStackPointer]);
}

void renderMenu(void) {
	int y = 170;
	const struct Menu* menu = &_menus[_activeMenu];
	size_t i;
	for (i = 0; i < menu->nEntries; ++i) {
		int bright = !menu->entries[i].enabled * 0x12;
		int x = 20;
		if (menu->active == i) {
			int bounce = (frame & 0x3F) - 0x20;
			bounce *= bounce;
			bounce = (0x400 - bounce) >> 6;
			bright = (bright + bounce) >> 1;
			x += _menuTween.y;
		} else if (menu->oldActive == i) {
			x += _oldMenuTween.y;
		}
		render3DText(menu->entries[i].text, x, y, bright * 0x421);
		y -= 10;
	}
}

void cursorMove(enum CursorDirection dir) {
	struct Menu* menu = &_menus[_activeMenu];
	int new = menu->active;
	switch (dir) {
	case CUR_UP:
		new = (menu->nEntries + menu->active - 1) % menu->nEntries;
		break;
	case CUR_DOWN:
		new = (menu->active + 1) % menu->nEntries;
		break;
	case CUR_LEFT:
		new = 0;
		break;
	case CUR_RIGHT:
		new = menu->nEntries - 1;
		break;
	}

	if (new == menu->active) {
		return;
	}

	_oldMenuTween.y0 = _menuTween.y;
	_oldMenuTween.y1 = 0;
	_oldMenuTween.x0 = frame;
	_oldMenuTween.x1 = frame + 8;
	_menuTween.y0 = 0;
	_menuTween.y1 = 10;
	_menuTween.x0 = frame + 4;
	_menuTween.x1 = frame + 12;
	menu->oldActive = menu->active;
	menu->active = new;
}

void menuActivate(void) {
	const struct Menu* menu = &_menus[_activeMenu];
	const struct MenuEntry* entry = &menu->entries[menu->active];
	if (!entry->enabled) {
		return;
	}
	if (entry->submenu == -2) {
		popMenu();
	} else if (entry->submenu >= 0) {
		pushMenu(entry->submenu);
	}
	if (entry->fn) {
		entry->fn(entry->context);
	}
}

void menuBack(void) {
	const struct Menu* menu = &_menus[_activeMenu];
	size_t i;
	for (i = 0; i < menu->nEntries; ++i) {
		const struct MenuEntry* entry = &menu->entries[i];
		if (entry->submenu == -2 && entry->fn) {
			entry->fn(entry->context);
		}
	}
	popMenu();
}
