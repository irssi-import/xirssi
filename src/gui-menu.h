#ifndef __GUI_MENU_H
#define __GUI_MENU_H

typedef enum {
	ACTION_SEPARATOR,
	ACTION_SUB,
	ACTION_ENDSUB,
	ACTION_COMMAND
} MenuAction;

typedef struct {
	MenuAction action;
	char *name;
	char *data;
} MenuItem;

void gui_menu_fill(GtkWidget *menu, MenuItem *items, int items_count,
		   GCallback callback, void *user_data);

#endif
