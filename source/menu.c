#include <nds.h>

#include <math.h>

#include "fifo3d.h"
#include "menu.h"
#include "text.h"
#include "tween.h"

#define MAX_MENUS 32
#define MAX_EDITOR_ENTRIES 32

struct Menu {
	bool isEditor;
	union {
		const struct MenuEntry* entries;
		struct {
			const struct EditorEntry* editor;
			size_t editorValues[MAX_EDITOR_ENTRIES];
		};
	};
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
	_menus[_maxMenu].isEditor = false;
	_menus[_maxMenu].entries = entries;
	_menus[_maxMenu].nEntries = nEntries;
	_menus[_maxMenu].active = 0;
	_menus[_maxMenu].oldActive = 0;
	return _maxMenu++;
}

int registerEditor(const struct EditorEntry* entries, size_t nEntries) {
	if (_maxMenu >= MAX_MENUS) {
		return -1;
	}
	_menus[_maxMenu].isEditor = true;
	_menus[_maxMenu].editor = entries;
	_menus[_maxMenu].nEntries = nEntries;
	_menus[_maxMenu].active = 0;
	_menus[_maxMenu].oldActive = 0;
	size_t i;
	for (i = 0; i < nEntries; ++i) {
		if (!entries[i].nChoices) {
			continue;
		}
		_menus[_maxMenu].active = i;
		_menus[_maxMenu].oldActive = i;
		break;
	}
	memset(_menus[_maxMenu].editorValues, 0, nEntries * sizeof(*_menus[_maxMenu].editorValues));
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

void setEditorValue(int editor, int entry, size_t value) {
	if (editor >= MAX_MENUS) {
		return;
	}
	struct Menu* menu = &_menus[editor];
	if (!menu->isEditor) {
		return;
	}
	if (entry >= menu->nEntries) {
		return;
	}
	if (value >= menu->editor[entry].nChoices) {
		return;
	}
	menu->editorValues[entry] = value;
}

size_t getEditorValue(int editor, int entry) {
	if (editor >= MAX_MENUS) {
		return 0;
	}
	const struct Menu* menu = &_menus[editor];
	if (!menu->isEditor) {
		return 0;
	}
	if (entry >= menu->nEntries) {
		return 0;
	}
	return menu->editorValues[entry];
}

void renderMenu(void) {
	const struct Menu* menu = &_menus[_activeMenu];
	int bounce = (frame & 0x3F) - 0x20;
	bounce *= bounce;
	bounce = (0x400 - bounce) >> 5;
	size_t i;
	if (menu->isEditor) {
		int x = 20;
		for (i = 0; i < menu->nEntries; ++i) {
			int bright = 0;
			int y = 170;
			if (menu->active == i) {
				bright = (bright + bounce) >> 1;
			}
			const char* text = menu->editor[i].placeholder;
			if (menu->editor[i].choices) {
				text = menu->editor[i].choices[menu->editorValues[i]];
			}
			render3DText(text, x, y, bright * 0x421);
			x += strlen(text) * 8;
		}
	} else {
		int y = 170;
		for (i = 0; i < menu->nEntries; ++i) {
			int bright = !menu->entries[i].enabled * 0x12;
			int x = 20;
			if (menu->active == i) {
				bright = (bright + bounce) >> 1;
				x += _menuTween.y;
			} else if (menu->oldActive == i) {
				x += _oldMenuTween.y;
			}
			render3DText(menu->entries[i].text, x, y, bright * 0x421);
			y -= 10;
		}
	}
}

void cursorMove(enum CursorDirection dir) {
	struct Menu* menu = &_menus[_activeMenu];
	int new = menu->active;
	if (menu->isEditor) {
		const struct EditorEntry* editor = &menu->editor[new];
		switch (dir) {
		case CUR_UP:
			menu->editorValues[new] = (menu->editorValues[new] + 1) % editor->nChoices;
			break;
		case CUR_DOWN:
			menu->editorValues[new] = (editor->nChoices + menu->editorValues[new] - 1) % editor->nChoices;
			break;
		case CUR_LEFT:
			do {
				new = (menu->nEntries + new - 1) % menu->nEntries;
			} while (!menu->editor[new].nChoices);
			break;
		case CUR_RIGHT:
			do {
				new = (new + 1) % menu->nEntries;
			} while (!menu->editor[new].nChoices);
			break;
		}
	} else {
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
	if (menu->isEditor) {
		popMenu();
		size_t i;
		for (i = 0; i < menu->nEntries; ++i) {
			const struct EditorEntry* entry = &menu->editor[i];
			if (!entry->fn) {
				continue;
			}
			entry->fn(menu->editorValues[i], entry->context);
		}
	} else {
		const struct MenuEntry* entry = &menu->entries[menu->active];
		if (!entry->enabled) {
			return;
		}
		if (entry->submenu == MENU_BACK) {
			popMenu();
		} else if (entry->submenu >= 0) {
			pushMenu(entry->submenu);
		}
		if (entry->fn) {
			entry->fn(entry->context);
		}
	}
}

void menuBack(void) {
	const struct Menu* menu = &_menus[_activeMenu];
	if (!menu->isEditor) {
		size_t i;
		for (i = 0; i < menu->nEntries; ++i) {
			const struct MenuEntry* entry = &menu->entries[i];
			if (entry->submenu == MENU_BACK && entry->fn) {
				entry->fn(entry->context);
			}
		}
	}
	popMenu();
}
