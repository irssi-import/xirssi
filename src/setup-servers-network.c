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
#include "chat-protocols.h"
#include "chatnets.h"

#include "gui.h"
#include "setup-servers.h"

static void network_save(GObject *data, NetworkConfig *network)
{
	ChatProtocol *proto;
	GtkEntry *entry;
	const char *name, *proto_name;

	entry = g_object_get_data(data, "name");
	name = gtk_entry_get_text(entry);

	if (*name == '\0') {
		/* name can't be empty */
		gui_popup_error("Network name can't be empty");
		return;
	}

	if (network == NULL) {
		/* create network */
		entry = g_object_get_data(data, "protocol");
		proto_name = gtk_entry_get_text(entry);

		proto = chat_protocol_find(proto_name);
		if (proto == NULL)
			proto = chat_protocol_get_unknown(proto_name);

		network = proto->create_chatnet();
		network->chat_type = proto->id;
	}

	g_free(network->name);
	network->name = g_strdup(name);

	gui_entry_update(data, "nick", &network->nick);
	gui_entry_update(data, "username", &network->username);
	gui_entry_update(data, "realname", &network->realname);
	gui_entry_update(data, "own_host", &network->own_host);
	gui_entry_update(data, "autosendcmd", &network->autosendcmd);

        chatnet_create(network);
}

static gboolean event_response_network(GtkWidget *widget, int response_id,
				       NetworkConfig *network)
{
	/* ok / cancel pressed */
	if (response_id == GTK_RESPONSE_OK) {
		network_save(g_object_get_data(G_OBJECT(widget), "data"),
			     network);
	}

	gtk_widget_destroy(widget);
	return FALSE;
}

void network_dialog_show(NetworkConfig *network)
{
	GtkWidget *dialog, *table, *sep;
	ChatProtocol *proto;
	int y;

	dialog = gtk_dialog_new_with_buttons("Add Network", NULL, 0,
					     GTK_STOCK_OK, GTK_RESPONSE_OK,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     NULL);
	g_signal_connect(G_OBJECT(dialog), "response",
			 G_CALLBACK(event_response_network), network);

	table = gtk_table_new(3, 3, FALSE);
	g_object_set_data(G_OBJECT(dialog), "data", table);
	gtk_container_set_border_width(GTK_CONTAINER(table), 7);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 7);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
			   table, TRUE, TRUE, 0);

	y = 0;

	/* -- */
	proto = network == NULL ? NULL :
		chat_protocol_find_id(network->chat_type);
	setup_server_add_protocol_widget(GTK_TABLE(table), y++, proto);

	gui_table_add_entry(GTK_TABLE(table), 0, y++, "Name", "name",
			    network == NULL ? NULL : network->name);

	sep = gtk_hseparator_new();
	gtk_table_attach(GTK_TABLE(table), sep, 0, 2, y, y+1,
			 GTK_FILL, GTK_FILL, 0, 10);
	y++;

	/* -- */
	gui_table_add_entry(GTK_TABLE(table), 0, y++, "Nick", "nick",
			    network == NULL ? NULL : network->nick);
	gui_table_add_entry(GTK_TABLE(table), 0, y++, "User Name", "username",
			    network == NULL ? NULL : network->username);
	gui_table_add_entry(GTK_TABLE(table), 0, y++, "Real Name", "realname",
			    network == NULL ? NULL : network->realname);

	sep = gtk_hseparator_new();
	gtk_table_attach(GTK_TABLE(table), sep, 0, 2, y, y+1,
			 GTK_FILL, GTK_FILL, 0, 10);
	y++;

	/* -- */
	gui_table_add_entry(GTK_TABLE(table), 0, y++, "Source Host", "own_host",
			    network == NULL ? NULL : network->own_host);
	gui_table_add_entry(GTK_TABLE(table), 0, y++, "Autosend Command",
			    "autosendcmd",
			    network == NULL ? NULL : network->autosendcmd);

	gtk_widget_show_all(dialog);
}
