/*
 setup-keyboard-edit.c : irssi

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

#include "setup.h"
#include "gui.h"
#include "gui-keyboard.h"
#include "glade/interface.h"

#include <gdk/gdkkeysyms.h>

enum {
	COL_PTR,
	COL_ID,
	COL_DESCRIPTION,

	N_COLUMNS
};

/* one common action store for open key binding settings */
static GtkListStore *action_store = NULL;

static void action_store_fill(GtkListStore *store)
{
	GtkTreeIter iter;
	GSList *tmp;

	for (tmp = keyinfos; tmp != NULL; tmp = tmp->next) {
		KeyAction *action = tmp->data;

		gtk_list_store_prepend(store, &iter);
		gtk_list_store_set(store, &iter, COL_PTR, action, -1);
	}
}

static void sig_store_destroyed(gpointer data, GObject *where_the_object_was)
{
	action_store = NULL;
}

static GtkListStore *action_store_create(void)
{
	GtkListStore *store;

	store = gtk_list_store_new(N_COLUMNS,
				   G_TYPE_POINTER,
				   G_TYPE_STRING,
				   G_TYPE_STRING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
					     COL_ID, GTK_SORT_ASCENDING);
	action_store_fill(store);

	g_object_weak_ref(G_OBJECT(store), sig_store_destroyed, NULL);
	return store;
}

static void keyboard_save(GObject *obj, Key *key)
{
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	const char *key_name, *data;
	KeyAction *action;

	key_name = gtk_entry_get_text(g_object_get_data(obj, "key"));
	if (*key_name == '\0') {
		gui_popup_error("Key can't be empty");
		return;
	}

	data = gtk_entry_get_text(g_object_get_data(obj, "data"));

	/* get selected action */
	sel = gtk_tree_view_get_selection(g_object_get_data(obj, "action_tree"));
	if (!gtk_tree_selection_get_selected(sel, &model, &iter)) {
		gui_popup_error("No action was selected");
		return;
	}

	gtk_tree_model_get(model, &iter, COL_PTR, &action, -1);

	if (key != NULL && strcmp(key->key, key_name) != 0)
		key_configure_remove(key->key);
	key_configure_add(action->id, key_name, data);
}

static gboolean event_destroy(GtkWidget *widget)
{
	g_object_unref(action_store);
	return FALSE;
}

static gboolean event_response(GtkWidget *widget, int response_id,
			       Key *key)
{
	/* ok / cancel pressed */
	if (response_id == GTK_RESPONSE_OK)
		keyboard_save(G_OBJECT(widget), key);

	gtk_widget_destroy(widget);
	return FALSE;
}

static void id_set_func(GtkTreeViewColumn *column,
			GtkCellRenderer   *cell,
			GtkTreeModel      *model,
			GtkTreeIter       *iter,
			gpointer           data)
{
	KeyAction *action;

	gtk_tree_model_get(model, iter, COL_PTR, &action, -1);
	g_object_set(G_OBJECT(cell), "text", action->id, NULL);
}

static void description_set_func(GtkTreeViewColumn *column,
				 GtkCellRenderer   *cell,
				 GtkTreeModel      *model,
				 GtkTreeIter       *iter,
				 gpointer           data)
{
	KeyAction *action;

	gtk_tree_model_get(model, iter, COL_PTR, &action, -1);
	g_object_set(G_OBJECT(cell), "text",
		     action->description == NULL ? "" : action->description,
		     NULL);
}

static gboolean event_key_pressed(GtkWidget *widget, GdkEventKey *event,
				  GtkToggleButton *toggle)
{
	GtkEntry *entry;
	char *str;

	if (!gtk_toggle_button_get_active(toggle))
		return FALSE;

	if (gui_is_modifier(event->keyval))
		return FALSE;

	entry = g_object_get_data(G_OBJECT(widget), "key");
	if (!GTK_WIDGET_HAS_FOCUS(entry))
		return FALSE;

	str = gui_keyboard_get_event_string(event);
	gtk_entry_set_text(entry, str);
	g_free(str);
	return TRUE;
}

void keyboard_dialog_show(Key *key)
{
	GtkWidget *dialog;
	GtkTreeView *view;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *sel;
	GtkTreePath *path;
	GtkTreeIter iter;
	GObject *obj;

	dialog = create_dialog_keyboard_settings();
	obj = G_OBJECT(dialog);

	g_signal_connect(obj, "destroy",
			 G_CALLBACK(event_destroy), NULL);
	g_signal_connect(obj, "response",
			 G_CALLBACK(event_response), key);

	/* key */
	g_signal_connect(obj, "key_press_event",
			 G_CALLBACK(event_key_pressed),
			 g_object_get_data(obj, "key_button"));

	/* action store */
	if (action_store == NULL)
		action_store = action_store_create();
	else
		g_object_ref(action_store);

	/* action view */
	view = g_object_get_data(obj, "action_tree");
	gtk_tree_view_set_model(view, GTK_TREE_MODEL(action_store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(view),
				    GTK_SELECTION_SINGLE);


	/* -- */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("ID", renderer,
							  "text", COL_ID,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						id_set_func, NULL, NULL);
	gtk_tree_view_append_column(view, column);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Description", renderer,
							  "text", COL_DESCRIPTION,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						description_set_func, NULL,
						NULL);
	gtk_tree_view_append_column(view, column);

	gtk_widget_show(dialog);

	if (key == NULL)
		return;

	/* set old values */
	gui_entry_set_from(obj, "key", key->key);
	gui_entry_set_from(obj, "data", key->data);

	/* select the action */
	if (gui_tree_model_find(GTK_TREE_MODEL(action_store),
				COL_PTR, &iter, key->info)) {
		sel = gtk_tree_view_get_selection(view);
		gtk_tree_selection_select_iter(sel, &iter);

		/* scroll it visible */
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(action_store),
					       &iter);
		gtk_tree_view_scroll_to_cell(view, path, column, TRUE, 0, 0);
	}
}
