#include <nds.h>

#define MENU_BACK -2

enum CursorDirection {
	CUR_UP,
	CUR_DOWN,
	CUR_LEFT,
	CUR_RIGHT
};

struct MenuEntry {
	const char* text;
	bool enabled;
	int submenu;
	void (*fn)(void*);
	void* context;
};

struct EditorEntry {
	const char* placeholder;
	const char** choices;
	size_t nChoices;
	void (*fn)(size_t, void*);
	void* context;
};

int registerMenu(const struct MenuEntry*, size_t nEntries);
int registerEditor(const struct EditorEntry*, size_t nEntries);
void setMenu(int menuid);
void pushMenu(int menuid);
void popMenu(void);

void setEditorValue(int editor, int entry, size_t value);
size_t getEditorValue(int editor, int entry);

void renderMenu(void);
void cursorMove(enum CursorDirection);
void menuActivate(void);
void menuBack(void);
