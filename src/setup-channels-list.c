/*
 setup-channels-list.c : irssi

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

#include "gui.h"
#include "setup.h"

static int timeout_edit_channel(GtkTreeView *view)
{
	GtkTreeViewColumn *column;
        GtkTreePath *path;

	path = g_object_get_data(G_OBJECT(view), "path");

	column = gtk_tree_view_get_column(view, 0);
	gtk_tree_view_set_cursor(view, path, column, TRUE);
	gtk_tree_path_free(path);

	return 0;
}

static gboolean event_channel_add(GtkWidget *widget, GtkTreeView *view)
{
        GtkListStore *store;
	GtkTreePath *path;
	GtkTreeIter iter;

        store = g_object_get_data(G_OBJECT(view), "store");
	gtk_list_store_prepend(store, &iter);
	gtk_list_store_set(store, &iter, 0, "#channel", -1);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);

	/* FIXME: this is quite a horrible kludge, but I can't see any
	   other way to do it - even 0 timeout doesn't work.
	   gtk_tree_view_set_cursor() is buggy with newly prepended items
	   for some reason - hopefully will be fixed soon. And since our
	   list is sorted, we can't append it either. */
	g_object_set_data(G_OBJECT(view), "path", path);
	g_timeout_add(100, (GSourceFunc) timeout_edit_channel, view);
	return FALSE;
}

static gboolean event_channel_remove(GtkWidget *widget, GtkTreeView *view)
{
	gui_tree_selection_delete(g_object_get_data(G_OBJECT(view), "store"),
				  view, NULL, NULL);
	return FALSE;
}

static gboolean channel_edited(GtkCellRendererText *cell, char *path_string,
			       char *new_text, GtkListStore *store)
{
	GtkTreePath *path;
	GtkTreeIter iter;

	if (*new_text == '\0') {
		gui_popup_error("Channel name can't be empty");
		return FALSE;
	}

	path = gtk_tree_path_new_from_string(path_string);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
	gtk_tree_path_free(path);

	/* update store */
	gtk_list_store_set(store, &iter, 0, new_text, -1);
	return FALSE;
}

GtkListStore *setup_channels_list_init(GtkTreeView *view,
				       GtkWidget *add_button,
				       GtkWidget *remove_button)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;

	/* store */
	store = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), 0,
					gui_tree_strcase_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
					     0, GTK_SORT_ASCENDING);

	/* view */
	g_object_set_data(G_OBJECT(view), "store", store);
	gtk_tree_view_set_model(view, GTK_TREE_MODEL(store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(view),
				    GTK_SELECTION_MULTIPLE);
        gtk_tree_view_set_headers_visible(view, FALSE);

	/* channel column */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(renderer), "edited",
			 G_CALLBACK(channel_edited), store);
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
							  "text", 0,
							  NULL);
	gtk_tree_view_append_column(view, column);

	/* channel buttons */
	g_signal_connect(G_OBJECT(add_button), "clicked",
			 G_CALLBACK(event_channel_add), view);
	g_signal_connect(G_OBJECT(remove_button), "clicked",
			 G_CALLBACK(event_channel_remove), view);

	return store;
}
