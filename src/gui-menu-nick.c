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
#include "servers.h"
#include "channels.h"
#include "nicklist.h"

typedef enum {
	ACTION_OP,
	ACTION_DEOP,
	ACTION_HALFOP,
	ACTION_DEHALFOP,
	ACTION_VOICE,
	ACTION_DEVOICE,
	ACTION_KICKBAN,
	ACTION_DCC_CHAT,
	ACTION_DCC_SEND,
	ACTION_PING,
	ACTION_VERSION,
	ACTION_TIME,
	ACTION_WHOIS,
	ACTION_QUERY
} MenuAction;

static void menu_callback(const char *data, MenuAction action,
			  GtkWidget *widget);

static GtkItemFactoryEntry menu_items[] = {
	{ "/_Op",		NULL, NULL, 0, "<Branch>" },
	{ "/Op/Give _Ops",	NULL, menu_callback, ACTION_OP, NULL },
	{ "/Op/Take Ops",	NULL, menu_callback, ACTION_DEOP, NULL },
	{ "/Op/sep1",		NULL, NULL, 0, "<Separator>" },
	{ "/Op/Give _Halfops",	NULL, menu_callback, ACTION_HALFOP, NULL },
	{ "/Op/Take Halfops",	NULL, menu_callback, ACTION_DEHALFOP, NULL },
	{ "/Op/sep2",		NULL, NULL, 0, "<Separator>" },
	{ "/Op/Give _Voice",	NULL, menu_callback, ACTION_VOICE, NULL },
	{ "/Op/Take Voice",	NULL, menu_callback, ACTION_DEVOICE, NULL },
	{ "/Op/sep3",		NULL, NULL, 0, "<Separator>" },
	{ "/Op/Kickban..",	NULL, menu_callback, ACTION_KICKBAN, NULL },
	{ "/_DCC",		NULL, NULL, 0, "<Branch>" },
	{ "/DCC/_Chat",		NULL, menu_callback, ACTION_DCC_CHAT, NULL },
	{ "/DCC/_Send File",	NULL, menu_callback, ACTION_DCC_SEND, NULL },
	{ "/_CTCP",		NULL, NULL, 0, "<Branch>" },
	{ "/CTCP/_Ping",	NULL, menu_callback, ACTION_PING, NULL },
	{ "/CTCP/_Version",	NULL, menu_callback, ACTION_VERSION, NULL },
	{ "/CTCP/_Time",	NULL, menu_callback, ACTION_TIME, NULL },
	{ "/_Whois",		NULL, menu_callback, ACTION_WHOIS, NULL },
	{ "/_Query",		NULL, menu_callback, ACTION_QUERY, NULL }
};

static void menu_callback(const char *data, MenuAction action,
			  GtkWidget *widget)
{
	printf("callback\n");
}

static gboolean event_menu_destroy(GtkWidget *widget)
{
	g_free(g_object_get_data(G_OBJECT(widget), "space_nicks"));
	g_free(g_object_get_data(G_OBJECT(widget), "comma_nicks"));
	return FALSE;
}

#if 0
static gboolean menuitem_nicks_cmd(const char *cmd)
{
	printf("%p\n", gtk_menu_item_get_submenu(item));
	Channel *channel;
	char signal[50], *nicks;

	channel = g_object_get_data(G_OBJECT(widget->parent), "channel");
	nicks = g_object_get_data(G_OBJECT(widget->parent), "nicks");

	if (g_slist_find(channels, channel) == NULL) {
		/* channel was destroyed while waiting */
		return FALSE;
	}

	g_snprintf(signal, sizeof(signal), "command %s", cmd);
	signal_emit(signal, 3, nicks, channel->server, channel);
	return FALSE;
}
#endif

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

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		       event->button, 0);
}
