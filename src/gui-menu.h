#ifndef __GUI_MENU_H
#define __GUI_MENU_H

typedef enum {
	ACTION_SEPARATOR,
	ACTION_SUB,
	ACTION_ENDSUB,

	ACTION_CUSTOM
} MenuAction;

typedef struct {
	MenuAction action;
	char *name;
	char *data;
} MenuItem;

typedef void (*MenuCallback) (void *user_data, const char *item_data,
			      int action);

void gui_menu_fill(GtkWidget *menu, MenuItem *items, int items_count,
		   MenuCallback callback, void *user_data);

/* main menu */
GtkWidget *gui_menu_bar_new(void);
void gui_main_menu_fill(GtkWidget *menu);

/* context menus: */
void gui_menu_nick_popup(Server *server, Channel *channel,
			 GSList *nicks, int button);
void gui_menu_url_popup(const char *url, int button);
void gui_menu_channel_topic_popup(Channel *channel, GtkWidget *widget,
				  int locked, int button);

#endif
