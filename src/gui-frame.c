/*
 gui-frame.c : irssi

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

#include "keyboard.h"

#include "gui-frame.h"
#include "gui-entry.h"
#include "gui-keyboard.h"
#include "gui-tab.h"
#include "gui-window.h"

#include <gdk/gdkkeysyms.h>

GSList *frames;
Frame *active_frame;

static gboolean event_destroy(GtkWidget *window, Frame *frame)
{
	frame->destroying = TRUE;

	frames = g_slist_remove(frames, frame);
	if (active_frame == frame)
		gui_frame_set_active(frames != NULL ? frames->data : NULL);

	/* destroy tabs first */
	gtk_widget_destroy(GTK_WIDGET(frame->notebook));

	signal_emit("gui frame destroyed", 1, frame);
	g_object_set_data(G_OBJECT(frame->widget), "Frame", NULL);
	printf("frame destroyed\n");
	g_free(frame);

	if (frames == NULL) {
		/* last window killed - quit */
		signal_emit("command quit", 1, "");
	}

	return FALSE;
}

static gboolean event_focus(GtkWidget *widget, GdkEventFocus *event,
			    Frame *frame)
{
	gui_frame_set_active(frame);
	if (frame->active_tab != NULL && frame->active_tab->active_win != NULL)
		window_set_active(frame->active_tab->active_win);
	return FALSE;
}

static gboolean event_key_press_after(GtkWidget *widget, GdkEventKey *event,
				      Frame *frame)
{
	char *str;

	if (event->keyval == GDK_Control_L || event->keyval == GDK_Control_R ||
	    event->keyval == GDK_Meta_L || event->keyval == GDK_Meta_R ||
	    event->keyval == GDK_Alt_L || event->keyval == GDK_Alt_R)
		return FALSE;

	str = gui_keyboard_get_event_string(event);
	if (key_pressed(frame->entry->keyboard, str)) {
		g_signal_stop_emission_by_name(G_OBJECT(widget),
					       "key_press_event");
		g_free(str);
		return TRUE;
	}
	g_free(str);

	return FALSE;
}

static gboolean event_key_press(GtkWidget *widget, GdkEventKey *event,
				Frame *frame)
{
	GtkWidget *entry;
	int pos;

	if (event->keyval == GDK_Tab ||
	    event->keyval == GDK_Up ||
	    event->keyval == GDK_Down ||
	    (event->state & GDK_CONTROL_MASK) != 0) {
		/* kludging around keys changing focus (and some others) and
		   not letting us handle it in the after-signal. and we can't
		   just handle everything here, because it will break
		   dead-keys */
		return event_key_press_after(widget, event, frame);
	}

	entry = frame->entry->widget;
	if (!GTK_WIDGET_HAS_FOCUS(entry) &&
	    (event->state & (GDK_CONTROL_MASK|GDK_MOD1_MASK)) == 0) {
		/* normal key pressed, change focus to entry field -
		   the whole text is selected after grab_focus so we need
		   to unselect it so that the text isn't replaced with the
		   next key press */
                pos = gtk_editable_get_position(GTK_EDITABLE(entry));
		gtk_widget_grab_focus(GTK_WIDGET(entry));
		gtk_editable_select_region(GTK_EDITABLE(entry), pos, pos);
	}

	return FALSE;
}

static gboolean event_switch_page(GtkNotebook *notebook, GtkNotebookPage *page,
				  gint page_num, Frame *frame)
{
	Tab *tab;

	tab = gui_tab_get_page(frame, page_num);
	frame->active_tab = tab;
	if (tab->active_win != NULL)
		window_set_active(tab->active_win);
	return FALSE;
}

Frame *gui_frame_new(int show)
{
	GtkWidget *window, *vbox, *notebook, *statusbar;
	Frame *frame;

	frame = g_new0(Frame, 1);

	/* create the window */
	frame->widget = window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	frame->window = GTK_WINDOW(window);
	g_object_set_data(G_OBJECT(window), "Frame", frame);

	g_signal_connect(G_OBJECT(window), "destroy",
			 G_CALLBACK(event_destroy), frame);
	g_signal_connect(G_OBJECT(window), "focus_in_event",
			 G_CALLBACK(event_focus), frame);
	g_signal_connect_after(GTK_OBJECT(window), "key_press_event",
			       G_CALLBACK(event_key_press_after), frame);
	g_signal_connect(GTK_OBJECT(window), "key_press_event",
			 G_CALLBACK(event_key_press), frame);

	gtk_window_set_default_size(frame->window, 700, 400);
        gtk_window_set_resizable(frame->window, TRUE);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	notebook = gtk_notebook_new();
	g_signal_connect(GTK_OBJECT(notebook), "switch_page",
			 G_CALLBACK(event_switch_page), frame);
	frame->notebook = GTK_NOTEBOOK(notebook);
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	/* entry */
	frame->entry = gui_entry_new(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame->entry->widget, FALSE, FALSE, 0);

	/* statusbar */
	statusbar = gtk_statusbar_new();
	frame->statusbar = GTK_STATUSBAR(statusbar);
	gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

	gtk_widget_grab_focus(frame->entry->widget);
	if (show) gtk_widget_show_all(window);

	frames = g_slist_prepend(frames, frame);

	signal_emit("gui frame created", 1, frame);
	return frame;
}

void gui_frame_set_active(Frame *frame)
{
	active_frame = frame;
	if (frame != NULL && frame->active_tab != NULL &&
	    frame->active_tab->active_win != NULL)
		window_set_active(frame->active_tab->active_win);
}

void gui_frame_set_active_window(Frame *frame, Window *window)
{
	gui_entry_set_window(frame->entry, window);
}
