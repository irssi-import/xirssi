/*
 setup-channels-edit.c : irssi

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
#include "channels-setup.h"

#include "gui.h"
#include "glade/interface.h"

#define NETWORK_NONE "<none>"

GList *get_chat_protocol_names(void);
GList *get_network_names(int chat_type, const char *network_none);

static void channel_save(GObject *obj, ChannelConfig *channel)
{
	ChatProtocol *proto;
	GtkEntry *entry;
	GtkToggleButton *toggle;
	const char *proto_name, *name;

	entry = g_object_get_data(obj, "name");
	name = gtk_entry_get_text(entry);

	if (*name == '\0') {
		/* name can't be empty */
		gui_popup_error("Channel name can't be empty");
		return;
	}

	if (channel == NULL) {
		/* create channel config */
		entry = g_object_get_data(obj, "protocol");
		proto_name = gtk_entry_get_text(entry);

		proto = chat_protocol_find(proto_name);
		if (proto == NULL)
			proto = chat_protocol_get_unknown(proto_name);

		channel = proto->create_channel_setup();
		channel->chat_type = proto->id;
	}

	g_free(channel->name);
	channel->name = g_strdup(name);

	gui_entry_update(obj, "network", &channel->chatnet);
	if (channel->chatnet != NULL &&
	    strcmp(channel->chatnet, NETWORK_NONE) == 0)
		g_free_and_null(channel->chatnet);

	gui_entry_set_from(obj, "name", channel->name);
	gui_entry_update(obj, "password", &channel->password);
	gui_entry_update(obj, "botmasks", &channel->botmasks);
	gui_entry_update(obj, "autosendcmd", &channel->autosendcmd);

	toggle = g_object_get_data(obj, "autojoin");
	channel->autojoin = gtk_toggle_button_get_active(toggle);

	channel_setup_create(channel);
}

static gboolean event_response_channel(GtkWidget *widget, int response_id,
				      ChannelConfig *channel)
{
	/* ok / cancel pressed */
	if (response_id == GTK_RESPONSE_OK)
		channel_save(G_OBJECT(widget), channel);

	gtk_widget_destroy(widget);
	return FALSE;
}

void channel_dialog_show(ChannelConfig *channel, const char *network)
{
	GtkWidget *dialog;
	GtkCombo *protocol_combo, *network_combo;
	GObject *obj;
	GList *list;

	dialog = create_dialog_channel_settings();
	obj = G_OBJECT(dialog);

	g_signal_connect(obj, "response",
			 G_CALLBACK(event_response_channel), channel);

	/* set chat protocols */
	protocol_combo = g_object_get_data(obj, "protocol_combo");
	list = get_chat_protocol_names();
	gtk_combo_set_popdown_strings(protocol_combo, list);
	g_list_free(list);

	/* set networks */
	network_combo = g_object_get_data(obj, "network_combo");
	list = get_network_names(channel == NULL ? -1 : channel->chat_type,
				 NETWORK_NONE);
	gtk_combo_set_popdown_strings(network_combo, list);
	g_list_free(list);

	/* set default network */
	if (channel != NULL)
		network = channel->chatnet;
	if (network == NULL)
		network = NETWORK_NONE;
	gtk_entry_set_text(GTK_ENTRY(network_combo->entry), network);

	if (channel != NULL) {
		/* set old values */
		gtk_widget_set_sensitive(GTK_WIDGET(protocol_combo), FALSE);
		gtk_entry_set_text(GTK_ENTRY(protocol_combo->entry),
				   CHAT_PROTOCOL(channel)->name);

		gui_entry_set_from(obj, "name", channel->name);
		gui_entry_set_from(obj, "password", channel->password);
		gui_entry_set_from(obj, "botmasks", channel->botmasks);
		gui_entry_set_from(obj, "autosendcmd", channel->autosendcmd);
		gui_toggle_set_from(obj, "autojoin", channel->autojoin);
	}

	gtk_widget_show(dialog);
}
