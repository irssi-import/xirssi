/*
 gui-menu-url.c : irssi

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
#include "misc.h"
#include "special-vars.h"
#include "servers.h"
#include "channels.h"

#include "gui-menu.h"

enum {
	ACTION_COMMAND = ACTION_CUSTOM
};

static MenuItem http_items[] = {
	{ ACTION_COMMAND,	"Open with _Galeon", "exec - -nosh -quiet galeon $0" },
	{ ACTION_COMMAND,	"Open with _Mozilla", "exec - -nosh mozilla $0" }
};

static MenuItem ftp_items[] = {
	{ ACTION_COMMAND,	"Open with _Galeon", "exec - -nosh galeon $0" },
	{ ACTION_COMMAND,	"Open with _lftp", "exec - -nosh xterm -e lftp $0" }
};

static MenuItem mail_items[] = {
	{ ACTION_COMMAND,	"Send mail with _mutt", "exec - -nosh xterm -e mutt $0" }
};

static MenuItem irc_items[] = {
	{ ACTION_COMMAND,	"Connect", "connect $0" }
};

static void menu_callback(void *user_data, const char *cmd, int action)
{
	GObject *obj = user_data;
	const char *url;

	url = g_object_get_data(obj, "url");
	eval_special_string(cmd, url, NULL, NULL);
}

static gboolean event_menu_destroy(GtkWidget *widget)
{
	g_free(g_object_get_data(G_OBJECT(widget), "url"));
	return FALSE;
}

void gui_menu_url_popup(const char *url, int button)
{
	MenuItem *items;
	GtkWidget *menu;
	char *new_url;
	int nitems;

	g_return_if_fail(url != NULL);

	/* fix the urls to contain proto:// */
	new_url = NULL;
	if (strncmp(url, "www.", 4) == 0)
		url = new_url = g_strconcat("http://", url, NULL);
	else if (strncmp(url, "ftp.", 4) == 0)
		url = new_url = g_strconcat("ftp://", url, NULL);

	if (strncmp(url, "http://", 7) == 0) {
		items = http_items;
		nitems = G_N_ELEMENTS(http_items);
	} else if (strncmp(url, "ftp://", 6) == 0) {
		items = ftp_items;
		nitems = G_N_ELEMENTS(ftp_items);
	} else if (strncmp(url, "mailto:", 7) == 0) {
		items = mail_items;
		nitems = G_N_ELEMENTS(mail_items);
	} else if (strncmp(url, "irc://", 6) == 0) {
		items = irc_items;
		nitems = G_N_ELEMENTS(irc_items);
	} else {
		/* unknown url protocol */
		return;
	}

	if (new_url == NULL)
		new_url = g_strdup(url);

	/* create the menu */
	menu = gtk_menu_new();

	g_signal_connect(G_OBJECT(menu), "destroy",
			 G_CALLBACK(event_menu_destroy), NULL);
	g_object_set_data(G_OBJECT(menu), "url", new_url);

	gui_menu_fill(menu, items, nitems, menu_callback, menu);

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, 0);
}
