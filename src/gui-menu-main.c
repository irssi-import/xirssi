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
#include "setup.h"

#define FAQ_URL "http://nl.irssi.org/?page=docs&doc=faq"
#define STARTUP_HOWTO_URL "http://irssi.org/?page=docs&doc=startup-HOWTO"

enum {
	ACTION_PREFERENCES = ACTION_CUSTOM,
	ACTION_QUIT,

	ACTION_FAQ,
	ACTION_HOWTO,
	ACTION_HOMEPAGE,
	ACTION_ABOUT
};

static int images_initialized = FALSE;
static GtkWidget *img_quit, *img_preferences, *img_home;

static MenuItem items[] = {
	{ ACTION_SUB,		"_Irssi" },
	{ ACTION_PREFERENCES,	"_Preferences", NULL, &img_preferences },
	{ ACTION_SEPARATOR },
	{ ACTION_QUIT,		"_Quit", NULL, &img_quit },
	{ ACTION_ENDSUB },

	{ ACTION_SUB,		"_Help" },
	{ ACTION_FAQ,		"_FAQ" },
	{ ACTION_HOWTO,		"_Startup HOWTO" },
	{ ACTION_HOMEPAGE,	"_Home Page", NULL, &img_home },
	{ ACTION_SEPARATOR },
	{ ACTION_ABOUT,		"_About" },
	{ ACTION_ENDSUB }
};

static void menu_callback(void *user_data, const char *item_data, int action)
{
	switch (action) {
	case ACTION_QUIT:
		signal_emit("command quit", 1, "");
		break;
	case ACTION_PREFERENCES:
		setup_preferences_show();
		break;

	case ACTION_FAQ:
		signal_emit("url open", 2, FAQ_URL, "http");
		break;
	case ACTION_HOWTO:
		signal_emit("url open", 2, STARTUP_HOWTO_URL, "http");
		break;
	case ACTION_HOMEPAGE:
		signal_emit("url open", 2, IRSSI_WEBSITE, "http");
		break;
	case ACTION_ABOUT:
		dialog_about_show();
		break;
	}
}

static void init_images(void)
{
	img_quit = gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU);
	img_preferences = gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
	img_home = gtk_image_new_from_stock(GTK_STOCK_HOME, GTK_ICON_SIZE_MENU);
}

GtkWidget *gui_menu_bar_new(void)
{
	GtkWidget *menubar;

	if (!images_initialized)
		init_images();

	menubar = gtk_menu_bar_new();
	gui_main_menu_fill(menubar);
	gtk_widget_show(menubar);
	return menubar;
}

void gui_main_menu_fill(GtkWidget *menu)
{
	gui_menu_fill(menu, items, G_N_ELEMENTS(items), menu_callback, menu);
}
