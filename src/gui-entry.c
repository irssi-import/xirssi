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

	keyboard_destroy(entry->keyboard);
	g_object_set_data(G_OBJECT(entry->widget), "Entry", NULL);
	g_free(entry);
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
	entry->keyboard = keyboard_create(entry);
	entry->widget = gtk_entry_new();
	entry->entry = GTK_ENTRY(entry->widget);

	g_object_set_data(G_OBJECT(entry->widget), "Entry", entry);

	g_signal_connect(G_OBJECT(entry->widget), "destroy",
			 G_CALLBACK(event_destroy), entry);
	g_signal_connect(G_OBJECT(entry->widget), "activate",
			 G_CALLBACK(event_activate), entry);

	signal_emit("gui entry created", 1, entry);
	return entry;
}

void gui_entry_set_window(Entry *entry, Window *window)
{
	entry->active_win = window;
}
