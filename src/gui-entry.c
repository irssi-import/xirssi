/*
 gui-entry.c : irssi

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

#include "gui-entry.h"
#include "gui-frame.h"

static gboolean event_destroy(GtkWidget *widget, Entry *entry)
{
	signal_emit("gui entry destroyed", 1, entry);

	entry->widget = NULL;
	entry->entry = NULL;

	gui_entry_unref(entry);
	return FALSE;
}

static gboolean event_activate(GtkWidget *widget, Entry *entry)
{
	key_pressed(entry->keyboard, "return");
	return FALSE;
}

Entry *gui_entry_new(Frame *frame)
{
	Entry *entry;

	entry = g_new0(Entry, 1);
	entry->refcount = 1;

	entry->frame = frame;
	gui_frame_ref(entry->frame);

	entry->keyboard = keyboard_create(entry);

	entry->widget = gtk_entry_new();
	entry->entry = GTK_ENTRY(entry->widget);

	g_signal_connect(G_OBJECT(entry->widget), "destroy",
			 G_CALLBACK(event_destroy), entry);
	g_signal_connect(G_OBJECT(entry->widget), "activate",
			 G_CALLBACK(event_activate), entry);

	signal_emit("gui entry created", 1, entry);
	return entry;
}

void gui_entry_ref(Entry *entry)
{
	entry->refcount++;
}

int gui_entry_unref(Entry *entry)
{
	if (--entry->refcount > 0)
		return TRUE;

	keyboard_destroy(entry->keyboard);
	gui_frame_unref(entry->frame);

	g_free(entry);
	return FALSE;
}

void gui_entry_set_window(Entry *entry, Window *window)
{
	entry->active_win = window;
}
