/*
 gui-menu-main.c : irssi

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
#include "signals.h"

#include "gui-menu.h"
#include "dialogs.h"
#include "setup-servers.h"

enum {
	ACTION_QUIT = ACTION_CUSTOM,

	ACTION_SETUP_SERVERS,

	ACTION_ABOUT
};

static MenuItem items[] = {
	{ ACTION_SUB,		"_Irssi" },
	{ ACTION_QUIT,		"_Quit" },
	{ ACTION_ENDSUB },

	{ ACTION_SUB,		"_Setup" },
	{ ACTION_SETUP_SERVERS,	"_Servers" },
	{ ACTION_ENDSUB },

	{ ACTION_SUB,		"_Help" },
	{ ACTION_ABOUT,		"_About" },
	{ ACTION_ENDSUB }
};

static void menu_callback(void *user_data, const char *item_data, int action)
{
	switch (action) {
	case ACTION_QUIT:
		signal_emit("command quit", 1, "");
		break;
	case ACTION_ABOUT:
		dialog_about_show();
		break;
	case ACTION_SETUP_SERVERS:
		setup_servers_show();
		break;
	}
}

GtkWidget *gui_menu_bar_new(void)
{
	GtkWidget *menubar;

	menubar = gtk_menu_bar_new();
	gui_main_menu_fill(menubar);
	gtk_widget_show(menubar);
	return menubar;
}

void gui_main_menu_fill(GtkWidget *menu)
{
	gui_menu_fill(menu, items, G_N_ELEMENTS(items), menu_callback, menu);
}
