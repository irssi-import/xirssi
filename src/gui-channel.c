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

#include "gui-frame.h"
#include "gui-entry.h"
#include "gui-channel.h"
#include "gui-nicklist.h"
#include "gui-menu.h"

#include "lock.xpm"

#include <gdk/gdkkeysyms.h>

typedef struct {
	Channel *channel;

	GtkWidget *widget, *lock_image;
	GtkEntry *topic;
} ChannelTitle;

static GdkPixbuf *lock_pixbuf;

void gui_channel_topic_lock(GtkWidget *widget)
{
	ChannelTitle *title;
	Frame *frame;

	frame = g_object_get_data(G_OBJECT(widget), "focus_frame");
	if (frame != NULL) {
		gui_frame_remove_focusable_widget(frame, widget);
                g_object_set_data(G_OBJECT(widget), "focus_frame", NULL);

		/* reset the topic */
		title = g_object_get_data(G_OBJECT(widget), "title");
		gtk_entry_set_text(title->topic, title->channel->topic != NULL ?
				   title->channel->topic : "");

		/* show lock image */
		gtk_widget_show(title->lock_image);

		/* remove focus */
		gtk_widget_grab_focus(frame->entry->widget);
		gtk_editable_select_region(GTK_EDITABLE(frame->entry->widget),
					   0, 0);
	}
}

void gui_channel_topic_unlock(GtkWidget *widget)
{
	ChannelTitle *title;
	Frame *frame;

	frame = gui_widget_find_data(widget, "Frame");
	if (frame != NULL) {
		g_object_set_data(G_OBJECT(widget), "focus_frame", frame);
		gui_frame_add_focusable_widget(frame, widget);

		/* hide lock image */
		title = g_object_get_data(G_OBJECT(widget), "title");
		gtk_widget_hide(title->lock_image);

		/* get focus */
		gtk_widget_grab_focus(widget);
		gtk_editable_select_region(GTK_EDITABLE(widget), 0, 0);
	}
}

static gboolean event_destroy(GtkWidget *widget, ChannelTitle *title)
{
	ChannelGui *gui;

	gui = CHANNEL_GUI(title->channel);
	gui->titles = g_slist_remove(gui->titles, title);

	gui_channel_topic_lock(widget);
	g_free(title);
	return FALSE;
}

static gboolean event_activate(GtkWidget *widget, ChannelTitle *title)
{
	char *str;

	str = g_strconcat(title->channel->name, " ",
			  gtk_entry_get_text(title->topic), NULL);

	signal_emit("command topic", 3, str, title->channel->server,
		    title->channel);
	g_free(str);

	gui_channel_topic_lock(widget);
	return FALSE;
}

static gboolean event_button_press(GtkWidget *widget, GdkEventButton *event,
				   ChannelTitle *title)
{
	Frame *frame;
	GtkWidget *topic;

	topic = GTK_WIDGET(title->topic);

	frame = g_object_get_data(G_OBJECT(topic), "focus_frame");
	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		/* doubleclicking topic */
		if (frame == NULL)
			gui_channel_topic_unlock(topic);
		else
                        gui_channel_topic_lock(topic);
		return TRUE;
	} else if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
		/* right-clicking topic */
		gui_menu_channel_topic_popup(title->channel, topic,
					     frame == NULL, 3);
		return TRUE;
	}

	return FALSE;
}

static gboolean event_key_press(GtkWidget *widget, GdkEventKey *event,
				ChannelTitle *title)
{
	if (event->keyval == GDK_Escape) {
		gui_channel_topic_lock(widget);
		return TRUE;
	}
	return FALSE;
}

GtkWidget *_get_title(WindowItem *witem)
{
	GtkWidget *hbox, *topic, *image, *eventbox;
	ChannelGui *gui;
	ChannelTitle *title;

	title = g_new0(ChannelTitle, 1);
	title->channel = CHANNEL(witem);

	gui = CHANNEL_GUI(title->channel);
	gui->titles = g_slist_prepend(gui->titles, title);

	hbox = title->widget = gtk_hbox_new(FALSE, 0);
	g_signal_connect(G_OBJECT(hbox), "destroy",
			 G_CALLBACK(event_destroy), title);

	/* lock image */
	eventbox = gtk_event_box_new();
	g_signal_connect(G_OBJECT(eventbox), "button_press_event",
			 G_CALLBACK(event_button_press), title);
	gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, FALSE, 2);

	title->lock_image = image = gtk_image_new_from_pixbuf(lock_pixbuf);
	gtk_container_add(GTK_CONTAINER(eventbox), image);

	/* topic */
	topic = gtk_entry_new();
	g_object_set_data(G_OBJECT(topic), "title", title);
	g_signal_connect(G_OBJECT(topic), "activate",
			 G_CALLBACK(event_activate), title);
	g_signal_connect(G_OBJECT(topic), "button_press_event",
			 G_CALLBACK(event_button_press), title);
	g_signal_connect(G_OBJECT(topic), "key_press_event",
			 G_CALLBACK(event_key_press), title);
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

	gui_nicklist_destroy(gui->nicklist);
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
	lock_pixbuf = gdk_pixbuf_new_from_xpm_data((const char **) lock_xpm);

	signal_add_first("channel created", (SIGNAL_FUNC) sig_channel_created);
	signal_add("channel destroyed", (SIGNAL_FUNC) sig_channel_destroyed);
	signal_add("channel topic changed", (SIGNAL_FUNC) sig_channel_topic_changed);
}

void gui_channels_deinit(void)
{
	gdk_pixbuf_unref(lock_pixbuf);

	signal_remove("channel created", (SIGNAL_FUNC) sig_channel_created);
	signal_remove("channel destroyed", (SIGNAL_FUNC) sig_channel_destroyed);
	signal_remove("channel topic changed", (SIGNAL_FUNC) sig_channel_topic_changed);
}
