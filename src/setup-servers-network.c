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

#include "glade/interface.h"

GList *get_chat_protocol_names(void);

static void network_save(GObject *obj, NetworkConfig *network)
{
	ChatProtocol *proto;
	GtkEntry *entry;
	const char *name, *proto_name;

	entry = g_object_get_data(obj, "name");
	name = gtk_entry_get_text(entry);

	if (*name == '\0') {
		/* name can't be empty */
		gui_popup_error("Network name can't be empty");
		return;
	}

	if (network == NULL) {
		/* create network */
		entry = g_object_get_data(obj, "protocol");
		proto_name = gtk_entry_get_text(entry);

		proto = chat_protocol_find(proto_name);
		if (proto == NULL)
			proto = chat_protocol_get_unknown(proto_name);

		network = proto->create_chatnet();
		network->chat_type = proto->id;
	}

	g_free(network->name);
	network->name = g_strdup(name);

	gui_entry_update(obj, "nick", &network->nick);
	gui_entry_update(obj, "username", &network->username);
	gui_entry_update(obj, "realname", &network->realname);
	gui_entry_update(obj, "own_host", &network->own_host);
	gui_entry_update(obj, "autosendcmd", &network->autosendcmd);

        chatnet_create(network);
}

static gboolean event_response_network(GtkWidget *widget, int response_id,
				       NetworkConfig *network)
{
	/* ok / cancel pressed */
	if (response_id == GTK_RESPONSE_OK)
		network_save(G_OBJECT(widget), network);

	gtk_widget_destroy(widget);
	return FALSE;
}

void network_dialog_show(NetworkConfig *network)
{
	GtkWidget *dialog;
	GtkCombo *protocol_combo;
	GObject *obj;
	GList *list;

	dialog = create_dialog_network_settings();
	obj = G_OBJECT(dialog);

	g_signal_connect(obj, "response",
			 G_CALLBACK(event_response_network), network);

	protocol_combo = g_object_get_data(obj, "protocol_combo");

	/* set chat protocols */
	list = get_chat_protocol_names();
	gtk_combo_set_popdown_strings(protocol_combo, list);
	g_list_free(list);

	if (network != NULL) {
		/* set old values */
		gtk_widget_set_sensitive(GTK_WIDGET(protocol_combo), FALSE);
		gtk_entry_set_text(GTK_ENTRY(protocol_combo->entry),
				   CHAT_PROTOCOL(network)->name);

		gui_entry_set_from(obj, "name", network->name);
		gui_entry_set_from(obj, "nick", network->nick);
		gui_entry_set_from(obj, "username", network->username);
		gui_entry_set_from(obj, "realname", network->realname);
		gui_entry_set_from(obj, "own_host", network->own_host);
		gui_entry_set_from(obj, "autosendcmd", network->autosendcmd);
	}

	gtk_widget_grab_focus(g_object_get_data(obj, "name"));
	gtk_widget_show(dialog);
}
