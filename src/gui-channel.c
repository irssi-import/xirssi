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
#include "gui-tab.h"
#include "gui-window-view.h"
#include "gui-window.h"
#include "gui-entry.h"
#include "gui-channel.h"
#include "gui-nicklist.h"
#include "gui-menu.h"

#include <gdk/gdkkeysyms.h>

typedef struct {
	WindowView *view;
	Channel *channel;
	GdkColor lock_color, unlock_color;

	GtkWidget *widget;
	GtkEntry *topic;

	unsigned int locked:1;
} ChannelTitle;

static gboolean colors_set = FALSE;
static GdkColor unlocked_text, unlocked_bg, unlocked_base;

static void colors_setup(GtkWidget *entry)
{
	colors_set = TRUE;

	memcpy(&unlocked_base, &entry->style->base[GTK_STATE_NORMAL],
	       sizeof(GdkColor));
	memcpy(&unlocked_bg, &entry->style->bg[GTK_STATE_NORMAL],
	       sizeof(GdkColor));
	memcpy(&unlocked_text, &entry->style->text[GTK_STATE_NORMAL],
	       sizeof(GdkColor));
}

static void title_set_colors(ChannelTitle *title)
{
	GtkWidget *topic;

	topic = GTK_WIDGET(title->topic);

	if (!title->locked) {
		gtk_widget_modify_base(topic, GTK_STATE_NORMAL, &unlocked_base);
		gtk_widget_modify_bg(topic, GTK_STATE_NORMAL, &unlocked_bg);
		gtk_widget_modify_text(topic, GTK_STATE_NORMAL, &unlocked_text);
	} else {
		gui_tab_set_focus_colors(topic, title->view->pane->focused);
	}
}

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

		/* set locked coloring */
		title->locked = TRUE;
                title_set_colors(title);

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

		/* set unlocked coloring */
		title->locked = FALSE;
		title_set_colors(title);

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

GtkWidget *_get_title(WindowView *view, WindowItem *witem)
{
	GtkWidget *hbox, *topic;
	ChannelGui *gui;
	ChannelTitle *title;

	title = g_new0(ChannelTitle, 1);
	title->view = view;
	title->channel = CHANNEL(witem);

	gui = CHANNEL_GUI(title->channel);
	gui->titles = g_slist_prepend(gui->titles, title);

	hbox = title->widget = gtk_hbox_new(FALSE, 0);
	g_signal_connect(G_OBJECT(hbox), "destroy",
			 G_CALLBACK(event_destroy), title);

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

	/* set the locking colors */
	if (!colors_set)
		colors_setup(topic);

	title->locked = TRUE;
	title_set_colors(title);

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

static ChannelTitle *channel_title_find(TabPane *pane)
{
	Channel *channel;
	GSList *tmp;

	if (pane->view == NULL)
		return NULL;

	channel = CHANNEL(pane->view->window->window->active);
	if (channel == NULL)
		return NULL;

	for (tmp = CHANNEL_GUI(channel)->titles; tmp != NULL; tmp = tmp->next) {
		ChannelTitle *title = tmp->data;

		if (title->view == pane->view)
			return title;
	}

	return NULL;
}

static void sig_pane_focused(TabPane *pane)
{
	ChannelTitle *title;

	title = channel_title_find(pane);
	if (title != NULL)
		title_set_colors(title);
}

static void sig_pane_unfocused(TabPane *pane)
{
	ChannelTitle *title;

	title = channel_title_find(pane);
	if (title != NULL)
                title_set_colors(title);
}

void gui_channels_init(void)
{
	signal_add_first("channel created", (SIGNAL_FUNC) sig_channel_created);
	signal_add("channel destroyed", (SIGNAL_FUNC) sig_channel_destroyed);
	signal_add("channel topic changed", (SIGNAL_FUNC) sig_channel_topic_changed);
	signal_add("tab pane focused", (SIGNAL_FUNC) sig_pane_focused);
	signal_add("tab pane unfocused", (SIGNAL_FUNC) sig_pane_unfocused);
}

void gui_channels_deinit(void)
{
	signal_remove("channel created", (SIGNAL_FUNC) sig_channel_created);
	signal_remove("channel destroyed", (SIGNAL_FUNC) sig_channel_destroyed);
	signal_remove("channel topic changed", (SIGNAL_FUNC) sig_channel_topic_changed);
	signal_remove("tab pane focused", (SIGNAL_FUNC) sig_pane_focused);
	signal_remove("tab pane unfocused", (SIGNAL_FUNC) sig_pane_unfocused);
}
