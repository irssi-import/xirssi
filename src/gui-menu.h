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

/* context menus: */
void gui_menu_nick_popup(Server *server, Channel *channel,
			 GSList *nicks, int button);
void gui_menu_url_popup(const char *url, int button);

#endif
