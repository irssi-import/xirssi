/*
 setup-keyboard.c : irssi

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
#include "signals.h"
#include "levels.h"

#include "keyboard.h"
#include "gui.h"

void keyboard_dialog_show(Key *keyboard);

static void setup_keyboard_signals_init(void);
static void setup_keyboard_signals_deinit(void);

enum {
	COL_PTR,
	COL_KEY,
	COL_ID,
	COL_DATA,

	N_COLUMNS
};

static GtkListStore *keyboard_store = NULL;

static gint keyboard_sort_func(GtkTreeModel *model,
			       GtkTreeIter *a, GtkTreeIter *b,
			       gpointer user_data)
{
	Key *key1, *key2;
	int ret;

	gtk_tree_model_get(model, a, COL_PTR, &key1, -1);
	gtk_tree_model_get(model, b, COL_PTR, &key2, -1);

	ret = strcmp(key1->key, key2->key);
	if (ret != 0)
		return ret;

	return strcasecmp(key1->data == NULL ? "" : key1->data,
			  key2->data == NULL ? "" : key2->data);
}

static gboolean event_destroy(GtkWidget *widget)
{
	setup_keyboard_signals_deinit();
	keyboard_store = NULL;
	return FALSE;
}

static void keyboard_store_fill(GtkListStore *store)
{
	GtkTreeIter iter;
	GSList *tmp, *tmp2;

	for (tmp = keyinfos; tmp != NULL; tmp = tmp->next) {
		KeyAction *action = tmp->data;

		for (tmp2 = action->keys; tmp2 != NULL; tmp2 = tmp2->next) {
			Key *key = tmp2->data;

			gtk_list_store_prepend(store, &iter);
			gtk_list_store_set(store, &iter, COL_PTR, key, -1);
		}
	}
}

static gboolean event_add(GtkWidget *widget, GtkTreeView *view)
{
	keyboard_dialog_show(NULL);
	return FALSE;
}

static void selection_edit(GtkTreeModel *model, GtkTreePath *path,
			   GtkTreeIter *iter, gpointer data)
{
	Key *key;

	gtk_tree_model_get(model, iter, COL_PTR, &key, -1);
	keyboard_dialog_show(key);
}

static gboolean event_edit(GtkWidget *widget, GtkTreeView *view)
{
	GtkTreeSelection *sel;

	sel = gtk_tree_view_get_selection(view);
        gtk_tree_selection_selected_foreach(sel, selection_edit, NULL);
	return FALSE;
}

static gboolean remove_iter_func(GtkTreeModel *model, GtkTreeIter *iter,
				 GtkTreePath *path, void *user_data)
{
	Key *key;

	/* remove from config */
	gtk_tree_model_get(model, iter, COL_PTR, &key, -1);
        key_configure_remove(key->key);

	return TRUE;
}

static gboolean event_remove(GtkWidget *widget, GtkTreeView *view)
{
	g_object_set_data(G_OBJECT(keyboard_store), "removing",
			  GINT_TO_POINTER(1));
	gui_tree_selection_delete(GTK_TREE_MODEL(keyboard_store), view,
				  remove_iter_func, NULL);
	g_object_set_data(G_OBJECT(keyboard_store), "removing", NULL);
	return FALSE;
}

static void key_set_func(GtkTreeViewColumn *column,
			 GtkCellRenderer   *cell,
			 GtkTreeModel      *model,
			 GtkTreeIter       *iter,
			 gpointer           data)
{
	Key *key;

	gtk_tree_model_get(model, iter, COL_PTR, &key, -1);
	g_object_set(G_OBJECT(cell), "text", key->key, NULL);
}

static void id_set_func(GtkTreeViewColumn *column,
			GtkCellRenderer   *cell,
			GtkTreeModel      *model,
			GtkTreeIter       *iter,
			gpointer           data)
{
	Key *key;

	gtk_tree_model_get(model, iter, COL_PTR, &key, -1);
	g_object_set(G_OBJECT(cell), "text", key->info->id, NULL);
}

static void data_set_func(GtkTreeViewColumn *column,
			  GtkCellRenderer   *cell,
			  GtkTreeModel      *model,
			  GtkTreeIter       *iter,
			  gpointer           data)
{
	Key *key;

	gtk_tree_model_get(model, iter, COL_PTR, &key, -1);
	g_object_set(G_OBJECT(cell),
		     "text", key->data == NULL ? "" : key->data,
		     NULL);
}

void setup_keyboard_init(GtkWidget *dialog)
{
	GtkWidget *button;
	GtkTreeView *tree_view;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	g_signal_connect(G_OBJECT(dialog), "destroy",
			 G_CALLBACK(event_destroy), NULL);

	/* tree store */
	keyboard_store = gtk_list_store_new(N_COLUMNS,
					    G_TYPE_POINTER,
					    G_TYPE_STRING,
					    G_TYPE_STRING,
					    G_TYPE_STRING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(keyboard_store), 0,
					keyboard_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(keyboard_store),
					     COL_PTR, GTK_SORT_ASCENDING);
        keyboard_store_fill(keyboard_store);

	/* view */
	tree_view = g_object_get_data(G_OBJECT(dialog), "bind_tree");
	gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(keyboard_store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tree_view),
				    GTK_SELECTION_MULTIPLE);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Key", renderer,
							  "text", COL_KEY,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						key_set_func, NULL, NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("ID", renderer,
							  "text", COL_ID,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						id_set_func, NULL, NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Data", renderer,
							  "text", COL_DATA,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						data_set_func, NULL, NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* buttons */
	button = g_object_get_data(G_OBJECT(dialog), "bind_add");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_add), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "bind_edit");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_edit), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "bind_remove");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_remove), tree_view);

	setup_keyboard_signals_init();
}

static int store_find_key(GtkListStore *store, GtkTreeIter *iter, Key *key)
{
	return gui_tree_model_find(GTK_TREE_MODEL(store), COL_PTR, iter, key);
}

static void sig_key_created(Key *key)
{
	GtkTreeIter iter;

	if (!store_find_key(keyboard_store, &iter, key))
		gtk_list_store_prepend(keyboard_store, &iter);

	gtk_list_store_set(keyboard_store, &iter, COL_PTR, key, -1);
}

static void sig_key_destroyed(Key *key)
{
	GtkTreeIter iter;

	if (g_object_get_data(G_OBJECT(keyboard_store), "removing") != NULL)
		return;

	if (store_find_key(keyboard_store, &iter, key))
		gtk_list_store_remove(keyboard_store, &iter);
}

static void setup_keyboard_signals_init(void)
{
	signal_add("key created", (SIGNAL_FUNC) sig_key_created);
	signal_add("key destroyed", (SIGNAL_FUNC) sig_key_destroyed);
}

static void setup_keyboard_signals_deinit(void)
{
	signal_remove("key created", (SIGNAL_FUNC) sig_key_created);
	signal_remove("key destroyed", (SIGNAL_FUNC) sig_key_destroyed);
}
