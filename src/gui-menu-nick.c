/*
 gui-menu-nick.c : irssi

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

static MenuItem items[] = {
	{ ACTION_SUB,		"_Op" },
	{ ACTION_COMMAND,	"Give _Ops", "op $1-" },
	{ ACTION_COMMAND,	"Take Ops", "deop $1-" },
	{ ACTION_SEPARATOR },
	{ ACTION_COMMAND,	"Give _Halfops", "halfop $1-" },
	{ ACTION_COMMAND,	"Take Halfops", "dehalfop $1-" },
	{ ACTION_SEPARATOR },
	{ ACTION_COMMAND,	"Give _Voice", "voice $1-" },
	{ ACTION_COMMAND,	"Take Voice", "devoice $1-" },
	{ ACTION_SEPARATOR },
	{ ACTION_COMMAND,	"Kickban...", "kickban -gui $0" },
	{ ACTION_ENDSUB },

	{ ACTION_SUB,		"_DCC" },
	{ ACTION_COMMAND,	"_Chat", "dcc chat $0" },
	{ ACTION_COMMAND,	"_Send File", "dcc send $0" },
	{ ACTION_ENDSUB },

	{ ACTION_SUB,		"_CTCP" },
	{ ACTION_COMMAND,	"_Ping", "ping $0" },
	{ ACTION_COMMAND,	"_Version", "ctcp $0 version" },
	{ ACTION_COMMAND,	"_Time", "ctcp $0 time" },
	{ ACTION_ENDSUB },

	{ ACTION_COMMAND,	"_Whois", "whois $0" },
	{ ACTION_COMMAND,	"_Query", "query $0" }
};

static void menu_callback(void *user_data, const char *cmd, int action)
{
	GObject *obj = user_data;
	Server *server;
	Channel *channel;
	GetNicksFunc func;
	void *func_data;
	const char *nicks;
	char *data;

	func = g_object_get_data(obj, "nicks_func");
	func_data = g_object_get_data(obj, "nicks_func_data");

	nicks = func(&server, &channel, func_data);
	if (nicks == NULL)
		return;

	data = g_strconcat(nicks, " ", nicks, NULL);
	g_strdelimit(data+strlen(nicks)+1, " ", ',');

	eval_special_string(cmd, data, server, channel);
	g_free(data);
}

static gboolean event_destroy(GtkWidget *widget)
{
	GetNicksFunc func;
	void *user_data;

	func = g_object_get_data(G_OBJECT(widget), "nicks_func");
	user_data = g_object_get_data(G_OBJECT(widget), "nicks_func_data");

	func(NULL, NULL, user_data);
	return FALSE;
}

void gui_menu_nick_popup(GetNicksFunc func, void *user_data, int button)
{
	GtkWidget *menu;

	/* create the menu */
	menu = gtk_menu_new();

	g_signal_connect(G_OBJECT(menu), "destroy",
			 G_CALLBACK(event_destroy), NULL);
	g_object_set_data(G_OBJECT(menu), "nicks_func", func);
	g_object_set_data(G_OBJECT(menu), "nicks_func_data", user_data);

	gui_menu_fill(menu, items, G_N_ELEMENTS(items), menu_callback, menu);

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, 0);
}
