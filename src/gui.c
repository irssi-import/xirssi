/*
 gui.c : some misc GUI-related functions

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

void gui_popup_error(const char *format, ...)
{
	GtkWidget *dialog;
	va_list va;
	char *str;

	va_start(va, format);
	str = g_strdup_vprintf(format, va);
	va_end(va);

	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK, "%s", str);
	g_signal_connect(G_OBJECT(dialog), "response",
			 G_CALLBACK(gtk_widget_destroy), NULL);
	g_free(str);

	gtk_widget_show(dialog);
}

GtkWidget *gui_table_add_entry(GtkTable *table, int x, int y,
			       const char *label_text,
			       const char *name, const char *value)
{
	GtkWidget *label, *entry;

	label = gtk_label_new(label_text);
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(GTK_TABLE(table), label, x, x+1, y, y+1,
			 GTK_FILL, 0, 0, 0);

	entry = gtk_entry_new();
	gtk_table_attach_defaults(GTK_TABLE(table), entry, x+1, x+2, y, y+1);

	if (value != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), value);

	g_object_set_data(G_OBJECT(table), name, entry);
	return entry;
}

void gui_entry_update(GObject *table, const char *name, char **value)
{
	GtkEntry *entry;
	const char *entry_text;

	entry = g_object_get_data(table, name);
        entry_text = gtk_entry_get_text(entry);

	g_free(*value);
	*value = *entry_text == '\0' ? NULL : g_strdup(entry_text);
}
