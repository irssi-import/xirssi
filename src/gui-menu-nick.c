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

static void menu_callback(GtkWidget *item, GtkWidget *menu)
{
	Server *server;
	Channel *channel;
	const char *cmd, *space_nicks, *comma_nicks;
	const char *server_tag, *channel_name;
	char *data;

	cmd = g_object_get_data(G_OBJECT(item), "data");
	space_nicks = g_object_get_data(G_OBJECT(menu), "space_nicks");
	comma_nicks = g_object_get_data(G_OBJECT(menu), "comma_nicks");
	server_tag = g_object_get_data(G_OBJECT(menu), "server_tag");
	channel_name = g_object_get_data(G_OBJECT(menu), "channel_name");

	server = server_find_tag(server_tag);
	channel = server == NULL || channel_name == NULL ? NULL :
		channel_find(server, channel_name);

	data = g_strconcat(comma_nicks, " ", space_nicks, NULL);
	eval_special_string(cmd, data, server, channel);
	g_free(data);
}

static gboolean event_menu_destroy(GtkWidget *widget)
{
	g_free(g_object_get_data(G_OBJECT(widget), "space_nicks"));
	g_free(g_object_get_data(G_OBJECT(widget), "comma_nicks"));
	return FALSE;
}

void gui_menu_nick_popup(Server *server, Channel *channel,
			 GSList *nicks, GdkEventButton *event)
{
	GtkWidget *menu;
	char *space_nicks, *comma_nicks;

	g_return_if_fail(server != NULL);
	g_return_if_fail(nicks != NULL);
	g_return_if_fail(event != NULL);

	space_nicks = gslist_to_string(nicks, " ");
	comma_nicks = gslist_to_string(nicks, ",");

	/* create the menu as item menu */
	menu = gtk_menu_new();

	g_signal_connect(G_OBJECT(menu), "destroy",
			 G_CALLBACK(event_menu_destroy), NULL);
	g_object_set_data(G_OBJECT(menu), "space_nicks", space_nicks);
	g_object_set_data(G_OBJECT(menu), "comma_nicks", comma_nicks);
	g_object_set_data(G_OBJECT(menu), "server_tag", server->tag);
	if (channel != NULL) {
		g_object_set_data(G_OBJECT(menu), "channel_name",
				  channel->name);
	}

	gui_menu_fill(menu, items, sizeof(items)/sizeof(items[0]),
		      G_CALLBACK(menu_callback), menu);

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		       event->button, 0);
}
