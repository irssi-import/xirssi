/*
 setup-servers-network.c : irssi

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
#include "network.h"
#include "misc.h"
#include "chat-protocols.h"
#include "chatnets.h"
#include "servers-setup.h"

#include "gui.h"
#include "glade/interface.h"

#define NETWORK_NONE "<none>" /* "no network" list item */

GList *get_chat_protocol_names(void);
GList *get_network_names(int chat_type, const char *network_none);

static void server_save(GObject *obj, ServerConfig *server)
{
	ChatProtocol *proto;
	GtkEntry *entry;
	GtkToggleButton *toggle;
	const char *proto_name, *address, *value;

	entry = g_object_get_data(obj, "address");
	address = gtk_entry_get_text(entry);

	if (*address == '\0') {
		/* name can't be empty */
		gui_popup_error("Server address can't be empty");
		return;
	}

	if (server == NULL) {
		/* create server config */
		entry = g_object_get_data(obj, "protocol");
		proto_name = gtk_entry_get_text(entry);

		proto = chat_protocol_find(proto_name);
		if (proto == NULL)
			proto = chat_protocol_get_unknown(proto_name);

		server = proto->create_server_setup();
		server->chat_type = proto->id;
	}

	entry = g_object_get_data(obj, "ipproto");
	value = gtk_entry_get_text(entry);
	if (g_strcasecmp(value, "ipv4") == 0)
		server->family = AF_INET;
	else if (g_strcasecmp(value, "ipv6") == 0)
		server->family = AF_INET6;
	else
		server->family = 0;

	g_free(server->address);
	server->address = g_strdup(address);

	entry = g_object_get_data(obj, "port");
	server->port = atoi(gtk_entry_get_text(entry));

	gui_entry_update(obj, "network", &server->chatnet);
	if (server->chatnet != NULL &&
	    strcmp(server->chatnet, NETWORK_NONE) == 0)
		g_free_and_null(server->chatnet);

	gui_entry_update(obj, "password", &server->password);
	gui_entry_update(obj, "own_host", &server->own_host);

	toggle = g_object_get_data(obj, "autoconnect");
	server->autoconnect = gtk_toggle_button_get_active(toggle);
	toggle = g_object_get_data(obj, "no_proxy");
	server->no_proxy = gtk_toggle_button_get_active(toggle);

	server_setup_add(server);
}

static gboolean event_response_server(GtkWidget *widget, int response_id,
				      ServerConfig *server)
{
	/* ok / cancel pressed */
	if (response_id == GTK_RESPONSE_OK)
		server_save(G_OBJECT(widget), server);

	gtk_widget_destroy(widget);
	return FALSE;
}

static GList *get_ip_protocol_names(void)
{
	GList *list;

	list = g_list_append(NULL, "Any");
	list = g_list_append(list, "IPv4");
	list = g_list_append(list, "IPv6");
	return list;
}

void server_dialog_show(ServerConfig *server, const char *network)
{
	GtkWidget *dialog;
	GtkCombo *protocol_combo, *network_combo, *ipproto_combo;
	GObject *obj;
	GList *list;
	char port[MAX_INT_STRLEN];

	dialog = create_dialog_server_settings();
	obj = G_OBJECT(dialog);

	g_signal_connect(obj, "response",
			 G_CALLBACK(event_response_server), server);

	/* set chat protocols */
	protocol_combo = g_object_get_data(obj, "protocol_combo");
	list = get_chat_protocol_names();
	gtk_combo_set_popdown_strings(protocol_combo, list);
	g_list_free(list);

	/* set networks */
	network_combo = g_object_get_data(obj, "network_combo");
	list = get_network_names(server == NULL ? -1 : server->chat_type,
				 NETWORK_NONE);
	gtk_combo_set_popdown_strings(network_combo, list);
	g_list_free(list);

	/* set default network */
	if (server != NULL)
		network = server->chatnet;
	if (network == NULL)
		network = NETWORK_NONE;
	gtk_entry_set_text(GTK_ENTRY(network_combo->entry), network);

	/* set ip protocols */
	ipproto_combo = g_object_get_data(obj, "ipproto_combo");
	list = get_ip_protocol_names();
	gtk_combo_set_popdown_strings(ipproto_combo, list);
	g_list_free(list);

	if (server != NULL) {
		/* set old values */
		gtk_widget_set_sensitive(GTK_WIDGET(protocol_combo), FALSE);
		gtk_entry_set_text(GTK_ENTRY(protocol_combo->entry),
				   CHAT_PROTOCOL(server)->name);

		if (server->family != 0) {
			gtk_entry_set_text(GTK_ENTRY(ipproto_combo->entry),
					   server->family == AF_INET ?
					   "IPv4" : "IPv6");
		}

		if (server->port <= 0)
			port[0] = '\0';
		else
			ltoa(port, server->port);

		gui_entry_set_from(obj, "address", server->address);
		gui_entry_set_from(obj, "port", port);
		gui_entry_set_from(obj, "password", server->password);
		gui_entry_set_from(obj, "own_host", server->own_host);
		gui_toggle_set_from(obj, "autoconnect", server->autoconnect);
		gui_toggle_set_from(obj, "no_proxy", server->no_proxy);
	}

	gtk_widget_grab_focus(g_object_get_data(obj, "address"));
	gtk_widget_show(dialog);
}
