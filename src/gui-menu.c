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

void gui_menu_fill(GtkWidget *menu, MenuItem *items, int items_count,
		   GCallback callback, void *user_data)
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
		case ACTION_COMMAND:
			item = gtk_menu_item_new_with_mnemonic(items[i].name);
			g_object_set_data(G_OBJECT(item), "data", items[i].data);
			g_signal_connect(G_OBJECT(item), "activate",
					 callback, user_data);
			break;
		default:
			item = NULL;
			break;
		}
		if (item != NULL)
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}
}
