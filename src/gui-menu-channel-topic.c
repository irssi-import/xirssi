/*
 gui-menu-channel-topic.c : irssi

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

#include "gui-channel.h"
#include "gui-menu.h"

static MenuItem locked_items[] = {
	{ ACTION_COMMAND,	"Unlock topic", "unlock" }
};

static MenuItem unlocked_items[] = {
	{ ACTION_COMMAND,	"Lock topic", "lock" }
};

static void menu_callback(GtkWidget *item, GtkWidget *menu)
{
	GtkWidget *widget;
	const char *cmd;

	cmd = g_object_get_data(G_OBJECT(item), "data");
	widget = g_object_get_data(G_OBJECT(menu), "topic_widget");

	if (strcmp(cmd, "lock") == 0)
		gui_channel_topic_lock(widget);
	else if (strcmp(cmd, "unlock") == 0)
		gui_channel_topic_unlock(widget);

	gtk_widget_unref(widget);
}

void gui_menu_channel_topic_popup(Channel *channel, GtkWidget *widget,
				  int locked, int button)
{
	GtkWidget *menu;

	g_return_if_fail(channel != NULL);

	menu = gtk_menu_new();

	gtk_widget_ref(widget);
	g_object_set_data(G_OBJECT(menu), "topic_widget", widget);

	if (locked) {
		gui_menu_fill(menu, locked_items,
			      G_N_ELEMENTS(locked_items),
			      G_CALLBACK(menu_callback), menu);
	} else {
		gui_menu_fill(menu, unlocked_items,
			      G_N_ELEMENTS(unlocked_items),
			      G_CALLBACK(menu_callback), menu);
	}

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, 0);
}
