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

enum {
	ACTION_LOCK = ACTION_CUSTOM,
	ACTION_UNLOCK
};

static MenuItem locked_items[] = {
	{ ACTION_UNLOCK,	"Unlock topic" }
};

static MenuItem unlocked_items[] = {
	{ ACTION_LOCK,		"Lock topic" }
};

static void menu_callback(void *user_data, const char *item_data, int action)
{
	GObject *obj = user_data;
	GtkWidget *widget;

	widget = g_object_get_data(obj, "topic_widget");

	switch (action) {
	case ACTION_LOCK:
		gui_channel_topic_lock(widget);
		break;
	case ACTION_UNLOCK:
		gui_channel_topic_unlock(widget);
		break;
	}

	gtk_widget_unref(widget);
}

void gui_menu_channel_topic_popup(Channel *channel, GtkWidget *widget,
				  int locked, int button)
{
	GtkWidget *menu;

	g_return_if_fail(channel != NULL);

	menu = gtk_menu_new();
	g_signal_connect(G_OBJECT(menu), "selection_done",
			 G_CALLBACK(gtk_widget_destroy), NULL);

	gtk_widget_ref(widget);
	g_object_set_data(G_OBJECT(menu), "topic_widget", widget);

	if (locked) {
		gui_menu_fill(menu, locked_items,
			      G_N_ELEMENTS(locked_items),
			      menu_callback, menu);
	} else {
		gui_menu_fill(menu, unlocked_items,
			      G_N_ELEMENTS(unlocked_items),
			      menu_callback, menu);
	}

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, 0);
}
