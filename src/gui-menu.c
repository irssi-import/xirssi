/*
 gui-menu.c : irssi

    Copyright (C) 2002 Timo Sirainen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "module.h"

#include "gui-menu.h"

static void menu_callback(GObject *item, void *user_data)
{
	MenuCallback callback;
	const char *item_data;
	int action;

	callback = g_object_get_data(item, "callback");
	item_data = g_object_get_data(item, "item_data");
	action = GPOINTER_TO_INT(g_object_get_data(item, "action"));

	callback(user_data, item_data, action);
}

void gui_menu_fill(GtkWidget *menu, MenuItem *items, int items_count,
		   MenuCallback callback, void *user_data)
{
	GtkWidget *newmenu, *item;
	int i;

	for (i = 0; i < items_count; i++) {
		switch (items[i].action) {
		case ACTION_SEPARATOR:
			item = gtk_menu_item_new();
			break;
		case ACTION_SUB:
			newmenu = gtk_menu_new();
			g_object_set_data(G_OBJECT(newmenu), "parent", menu);

			item = gtk_menu_item_new_with_mnemonic(items[i].name);
			g_object_set_data(G_OBJECT(newmenu), "item", item);

			item = NULL;
			menu = newmenu;
			break;
		case ACTION_ENDSUB:
			newmenu = g_object_get_data(G_OBJECT(menu), "parent");
			item = g_object_get_data(G_OBJECT(menu), "item");
			if (newmenu != NULL) {
				g_object_set_data(G_OBJECT(menu), "item", NULL);
				gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),
							  menu);
				menu = newmenu;
			}
			break;
		default:
			item = gtk_menu_item_new_with_mnemonic(items[i].name);
			g_object_set_data(G_OBJECT(item), "item_data",
					  items[i].data);
			g_object_set_data(G_OBJECT(item), "callback",
					  callback);
			g_object_set_data(G_OBJECT(item), "action",
					  GINT_TO_POINTER(items[i].action));
			g_signal_connect(G_OBJECT(item), "activate",
					 G_CALLBACK(menu_callback), user_data);
			break;
		}

		if (item != NULL)
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}
}
