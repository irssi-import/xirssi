/*
 gui-channel.c : irssi

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
#include "modules.h"
#include "signals.h"

#include "gui-channel.h"
#include "gui-nicklist.h"

typedef struct {
	Channel *channel;

	GtkWidget *widget;
	GtkEntry *topic;
} ChannelTitle;

static gboolean event_destroy(GtkWidget *widget, ChannelTitle *title)
{
	ChannelGui *gui;

	gui = CHANNEL_GUI(title->channel);
	gui->titles = g_slist_remove(gui->titles, title);

	g_free(title);
	return FALSE;
}

GtkWidget *_get_title(WindowItem *witem)
{
	GtkWidget *hbox, *topic;
	ChannelGui *gui;
	ChannelTitle *title;

	title = g_new0(ChannelTitle, 1);
	title->channel = CHANNEL(witem);

	gui = CHANNEL_GUI(title->channel);
	gui->titles = g_slist_prepend(gui->titles, title);

	hbox = title->widget = gtk_hbox_new(FALSE, 0);
	g_signal_connect(G_OBJECT(hbox), "destroy",
			 G_CALLBACK(event_destroy), title);

	/* topic */
	topic = gtk_entry_new();
	title->topic = GTK_ENTRY(topic);
	if (title->channel->topic != NULL)
		gtk_entry_set_text(title->topic, title->channel->topic);
	gtk_box_pack_start(GTK_BOX(hbox), topic, TRUE, TRUE, 0);

	gtk_widget_show_all(hbox);
	return hbox;
}

static void sig_channel_created(Channel *channel)
{
	ChannelGui *gui;

	gui = g_new0(ChannelGui, 1);
	gui->channel = channel;
	gui->nicklist = gui_nicklist_new(channel);
	gui->get_title = _get_title;

	MODULE_DATA_SET(channel, gui);

	signal_emit("gui channel created", 1, gui);
}

static void sig_channel_destroyed(Channel *channel)
{
	ChannelGui *gui;

	gui = CHANNEL_GUI(channel);
	signal_emit("gui channel destroyed", 1, gui);

	while (gui->titles != NULL) {
		ChannelTitle *title = gui->titles->data;
		gtk_widget_destroy(title->widget);
	}

	gui_nicklist_unref(gui->nicklist);
	g_free(gui);

	MODULE_DATA_UNSET(channel);
}

static void sig_channel_topic_changed(Channel *channel)
{
	ChannelGui *gui;
	GSList *tmp;
	char *text;

	if (channel->topic == NULL)
		text = NULL;
	else {
		text = g_locale_to_utf8(channel->topic, -1, NULL, NULL, NULL);
		if (text == NULL) {
			text = g_convert(channel->topic, strlen(channel->topic),
					 "UTF-8", "CP1252", NULL, NULL, NULL);
		}
	}

	gui = CHANNEL_GUI(channel);
	for (tmp = gui->titles; tmp != NULL; tmp = tmp->next) {
		ChannelTitle *title = tmp->data;

		gtk_entry_set_text(title->topic, text == NULL ? "" : text);
	}
	g_free(text);
}

void gui_channels_init(void)
{
	signal_add_first("channel created", (SIGNAL_FUNC) sig_channel_created);
	signal_add("channel destroyed", (SIGNAL_FUNC) sig_channel_destroyed);
	signal_add("channel topic changed", (SIGNAL_FUNC) sig_channel_topic_changed);
}

void gui_channels_deinit(void)
{
	signal_remove("channel created", (SIGNAL_FUNC) sig_channel_created);
	signal_remove("channel destroyed", (SIGNAL_FUNC) sig_channel_destroyed);
	signal_remove("channel topic changed", (SIGNAL_FUNC) sig_channel_topic_changed);
}
