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
#include "setup-servers.h"

#define NETWORK_NONE "<none>"

static void server_save(GObject *data, ServerConfig *server)
{
	ChatProtocol *proto;
	GtkEntry *entry;
	GtkToggleButton *toggle;
	const char *proto_name, *address, *value;

	entry = g_object_get_data(data, "address");
	address = gtk_entry_get_text(entry);

	if (*address == '\0') {
		/* name can't be empty */
		gui_popup_error("Server address can't be empty");
		return;
	}

	if (server == NULL) {
		/* create server */
		entry = g_object_get_data(data, "protocol");
		proto_name = gtk_entry_get_text(entry);

		proto = chat_protocol_find(proto_name);
		if (proto == NULL)
			proto = chat_protocol_get_unknown(proto_name);

		server = proto->create_server_setup();
		server->chat_type = proto->id;
	}

	entry = g_object_get_data(data, "ipproto");
	value = gtk_entry_get_text(entry);
	if (g_strcasecmp(value, "ipv4") == 0)
		server->family = AF_INET;
	else if (g_strcasecmp(value, "ipv6") == 0)
		server->family = AF_INET6;
	else
		server->family = 0;

	g_free(server->address);
	server->address = g_strdup(address);

	entry = g_object_get_data(data, "port");
	server->port = atoi(gtk_entry_get_text(entry));

	gui_entry_update(data, "network", &server->chatnet);
	if (server->chatnet != NULL &&
	    strcmp(server->chatnet, NETWORK_NONE) == 0)
		g_free_and_null(server->chatnet);

	gui_entry_update(data, "password", &server->password);
	gui_entry_update(data, "own_host", &server->own_host);

	toggle = g_object_get_data(data, "autoconnect");
	server->autoconnect = gtk_toggle_button_get_active(toggle);
	toggle = g_object_get_data(data, "no_proxy");
	server->no_proxy = gtk_toggle_button_get_active(toggle);

	server_setup_add(server);
}

static gboolean event_response_server(GtkWidget *widget, int response_id,
				      ServerConfig *server)
{
	/* ok / cancel pressed */
	if (response_id == GTK_RESPONSE_OK) {
		server_save(g_object_get_data(G_OBJECT(widget), "data"),
			    server);
	}

	gtk_widget_destroy(widget);
	return FALSE;
}

static GList *get_network_names(int chat_type)
{
	GList *list;
	GSList *tmp;

	list = NULL;
	for (tmp = chatnets; tmp != NULL; tmp = tmp->next) {
		NetworkConfig *network = tmp->data;

		if (chat_type == -1 || network->chat_type == chat_type)
			list = g_list_append(list, network->name);
	}
	list = g_list_append(list, NETWORK_NONE);
	return list;
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
	GtkWidget *dialog, *table, *sep, *label, *combo, *checkbox;
	ChatProtocol *proto;
	GList *list;
	char port[MAX_INT_STRLEN];
	int y;

	if (server != NULL)
		network = server->chatnet;

	dialog = gtk_dialog_new_with_buttons("Add Network", NULL, 0,
					     GTK_STOCK_OK, GTK_RESPONSE_OK,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     NULL);
	g_signal_connect(G_OBJECT(dialog), "response",
			 G_CALLBACK(event_response_server), server);

	table = gtk_table_new(3, 3, FALSE);
	g_object_set_data(G_OBJECT(dialog), "data", table);
	gtk_container_set_border_width(GTK_CONTAINER(table), 7);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 7);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
			   table, TRUE, TRUE, 0);

	y = 0;

	/* -- */
	proto = server == NULL ? NULL :
		chat_protocol_find_id(server->chat_type);
	setup_server_add_protocol_widget(GTK_TABLE(table), y++, proto);

	label = gtk_label_new("Network");
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, y, y+1,
			 GTK_FILL, 0, 0, 0);

	combo = gtk_combo_new();
        gtk_combo_set_value_in_list(GTK_COMBO(combo), TRUE, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), combo,
				  1, 2, y, y+1);
	y++;

	list = get_network_names(server == NULL ? -1 : server->chat_type);
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
	g_list_free(list);

	if (network == NULL)
		network = NETWORK_NONE;
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), network);
	g_object_set_data(G_OBJECT(table), "network", GTK_COMBO(combo)->entry);

	sep = gtk_hseparator_new();
	gtk_table_attach(GTK_TABLE(table), sep, 0, 2, y, y+1,
			 GTK_FILL, GTK_FILL, 0, 10);
	y++;

	/* -- */

	/* ip protocol */
	label = gtk_label_new("IP Protocol");
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, y, y+1,
			 GTK_FILL, 0, 0, 0);

	combo = gtk_combo_new();
	gtk_table_attach_defaults(GTK_TABLE(table), combo,
				  1, 2, y, y+1);
	y++;

	list = get_ip_protocol_names();
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);

	gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(combo)->entry),
				  FALSE);
	if (server != NULL && server->family != 0) {
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry),
				   server->family == AF_INET ? "IPv4" : "IPv6");
	}
	g_list_free(list);
	g_object_set_data(G_OBJECT(table), "ipproto", GTK_COMBO(combo)->entry);

	/* address */
	gui_table_add_entry(GTK_TABLE(table), 0, y++, "Address", "address",
			    server == NULL ? NULL : server->address);

	/* port */
	if (server == NULL || server->port <= 0)
		port[0] = '\0';
	else
		ltoa(port, server->port);
	gui_table_add_entry(GTK_TABLE(table), 0, y++, "Port", "port", port);

	sep = gtk_hseparator_new();
	gtk_table_attach(GTK_TABLE(table), sep, 0, 2, y, y+1,
			 GTK_FILL, GTK_FILL, 0, 10);
	y++;

	/* -- */
	gui_table_add_entry(GTK_TABLE(table), 0, y++, "Password", "password",
			    server == NULL ? NULL : server->password);
	gui_table_add_entry(GTK_TABLE(table), 0, y++, "Source Host", "own_host",
			    server == NULL ? NULL : server->own_host);

	sep = gtk_hseparator_new();
	gtk_table_attach(GTK_TABLE(table), sep, 0, 2, y, y+1,
			 GTK_FILL, GTK_FILL, 0, 10);
	y++;

	checkbox = gtk_check_button_new_with_label("Automatically connect at startup");
	g_object_set_data(G_OBJECT(table), "autoconnect", checkbox);
	gtk_table_attach(GTK_TABLE(table), checkbox, 0, 2, y, y+1,
			 GTK_FILL, GTK_FILL, 0, 0);
	y++;

	checkbox = gtk_check_button_new_with_label("Disable connecting through proxy");
	g_object_set_data(G_OBJECT(table), "no_proxy", checkbox);
	gtk_table_attach(GTK_TABLE(table), checkbox, 0, 2, y, y+1,
			 GTK_FILL, GTK_FILL, 0, 0);
	y++;

	gtk_widget_show_all(dialog);
}
