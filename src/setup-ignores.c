/*
 setup-ignores.c : irssi

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
#include "ignore.h"
#include "levels.h"

#include "gui.h"

void ignore_dialog_show(Ignore *ignore);

static void setup_ignore_signals_init(void);
static void setup_ignore_signals_deinit(void);

enum {
	COL_PTR,
	COL_EXCEPT,
	COL_NICK,
	COL_CHANNELS,
	COL_LEVEL,
	COL_PATTERN,

	N_COLUMNS
};

static GtkListStore *ignore_store = NULL;

static gint ignore_sort_func(GtkTreeModel *model,
			     GtkTreeIter *a, GtkTreeIter *b,
			     gpointer user_data)
{
	Ignore *ignore1, *ignore2;
	int ret;

	gtk_tree_model_get(model, a, COL_PTR, &ignore1, -1);
	gtk_tree_model_get(model, b, COL_PTR, &ignore2, -1);

	ret = strcasecmp(ignore1->mask == NULL ? "" : ignore1->mask,
			 ignore2->mask == NULL ? "" : ignore2->mask);
	if (ret != 0)
		return ret;

	return strcasecmp(ignore1->pattern == NULL ? "" : ignore1->pattern,
			  ignore2->pattern == NULL ? "" : ignore2->pattern);
}

static gboolean event_destroy(GtkWidget *widget)
{
	setup_ignore_signals_deinit();
	ignore_store = NULL;
	return FALSE;
}

static void ignore_store_fill(GtkListStore *store)
{
	GtkTreeIter iter;
	GSList *tmp;

	/* add channels - add the networks on the way, they may not even
	   exist in the network list so use just their names */
	for (tmp = ignores; tmp != NULL; tmp = tmp->next) {
		Ignore *ignore = tmp->data;

		gtk_list_store_prepend(store, &iter);
		gtk_list_store_set(store, &iter, COL_PTR, ignore, -1);
	}
}

static void exception_toggled(GtkCellRendererToggle *cell, char *path_string)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	Ignore *ignore;

	model = GTK_TREE_MODEL(ignore_store);
	path = gtk_tree_path_new_from_string(path_string);

	/* update config */
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, COL_PTR, &ignore, -1);
	ignore->exception = !gtk_cell_renderer_toggle_get_active(cell);

	/* update tree view */
        gtk_tree_model_row_changed(model, path, &iter);
}

static gboolean event_add(GtkWidget *widget, GtkTreeView *view)
{
	ignore_dialog_show(NULL);
	return FALSE;
}

static void selection_edit(GtkTreeModel *model, GtkTreePath *path,
			   GtkTreeIter *iter, gpointer data)
{
	Ignore *ignore;

	gtk_tree_model_get(model, iter, COL_PTR, &ignore, -1);
	ignore_dialog_show(ignore);
}

static gboolean event_edit(GtkWidget *widget, GtkTreeView *view)
{
	GtkTreeSelection *sel;

	sel = gtk_tree_view_get_selection(view);
        gtk_tree_selection_selected_foreach(sel, selection_edit, NULL);
	return FALSE;
}

static gboolean event_remove(GtkWidget *widget, GtkTreeView *view)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GSList *paths;
	Ignore *ignore;

	model = GTK_TREE_MODEL(ignore_store);

	paths =  gui_tree_selection_get_paths(view);
	while (paths != NULL) {
		GtkTreePath *path = paths->data;

		/* remove from tree */
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, COL_PTR, &ignore, -1);
		gtk_list_store_remove(ignore_store, &iter);

		/* remove from config */
		ignore->level = 0;
                ignore_update_rec(ignore);

		paths = g_slist_remove(paths, path);
		gtk_tree_path_free(path);
	}

	return FALSE;
}

static void except_set_func(GtkTreeViewColumn *column,
			    GtkCellRenderer   *cell,
			    GtkTreeModel      *model,
			    GtkTreeIter       *iter,
			    gpointer           data)
{
	Ignore *ignore;

	gtk_tree_model_get(model, iter, COL_PTR, &ignore, -1);
	g_object_set(G_OBJECT(cell), "active", ignore->exception, NULL);
}

static void nick_set_func(GtkTreeViewColumn *column,
			  GtkCellRenderer   *cell,
			  GtkTreeModel      *model,
			  GtkTreeIter       *iter,
			  gpointer           data)
{
	Ignore *ignore;

	gtk_tree_model_get(model, iter, COL_PTR, &ignore, -1);
	g_object_set(G_OBJECT(cell),
		     "text", ignore->mask == NULL ? "" : ignore->mask,
		     NULL);
}

static void channels_set_func(GtkTreeViewColumn *column,
			      GtkCellRenderer   *cell,
			      GtkTreeModel      *model,
			      GtkTreeIter       *iter,
			      gpointer           data)
{
	Ignore *ignore;
	char *channels;

	gtk_tree_model_get(model, iter, COL_PTR, &ignore, -1);

	channels = ignore->channels == NULL ? NULL :
		g_strjoinv("\n", ignore->channels);
	g_object_set(G_OBJECT(cell),
		     "text", channels == NULL ? "" : channels,
		     NULL);
	g_free(channels);
}

static void level_set_func(GtkTreeViewColumn *column,
			   GtkCellRenderer   *cell,
			   GtkTreeModel      *model,
			   GtkTreeIter       *iter,
			   gpointer           data)
{
	Ignore *ignore;
	char *level;

	gtk_tree_model_get(model, iter, COL_PTR, &ignore, -1);

	level = bits2level(ignore->level == 0 ? MSGLEVEL_ALL : ignore->level);
	g_object_set(G_OBJECT(cell), "text", level, NULL);
	g_free(level);
}

static void pattern_set_func(GtkTreeViewColumn *column,
			     GtkCellRenderer   *cell,
			     GtkTreeModel      *model,
			     GtkTreeIter       *iter,
			     gpointer           data)
{
	Ignore *ignore;

	gtk_tree_model_get(model, iter, COL_PTR, &ignore, -1);
	g_object_set(G_OBJECT(cell),
		     "text", ignore->pattern == NULL ? "" : ignore->pattern,
		     NULL);
}

void setup_ignores_init(GtkWidget *dialog)
{
	GtkWidget *button;
	GtkTreeView *tree_view;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	g_signal_connect(G_OBJECT(dialog), "destroy",
			 G_CALLBACK(event_destroy), NULL);

	/* tree store */
	ignore_store = gtk_list_store_new(N_COLUMNS,
					  G_TYPE_POINTER,
					  G_TYPE_BOOLEAN,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(ignore_store), 0,
					ignore_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ignore_store),
					     COL_PTR, GTK_SORT_ASCENDING);
        ignore_store_fill(ignore_store);

	/* view */
	tree_view = g_object_get_data(G_OBJECT(dialog), "ignore_tree");
	gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(ignore_store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tree_view),
				    GTK_SELECTION_MULTIPLE);

	/* -- */
	renderer = gtk_cell_renderer_toggle_new();
	g_object_set(G_OBJECT(renderer), "activatable", TRUE, NULL);
	g_signal_connect(G_OBJECT(renderer), "toggled",
			 G_CALLBACK(exception_toggled), NULL);
	column = gtk_tree_view_column_new_with_attributes("Except", renderer,
							  "active", COL_EXCEPT,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						except_set_func,
						NULL, NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Nick Mask", renderer,
							  "text", COL_NICK,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						nick_set_func, NULL, NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Channels", renderer,
							  "text", COL_CHANNELS,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						channels_set_func, NULL, NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Level", renderer,
							  "text", COL_LEVEL,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						level_set_func, NULL, NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Pattern", renderer,
							  "text", COL_PATTERN,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						pattern_set_func, NULL, NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* buttons */
	button = g_object_get_data(G_OBJECT(dialog), "ignore_add");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_add), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "ignore_edit");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_edit), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "ignore_remove");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_remove), tree_view);

	setup_ignore_signals_init();
}

static int store_find_ignore(GtkListStore *store, GtkTreeIter *iter,
			     Ignore *ignore)
{
	return gui_tree_model_find(GTK_TREE_MODEL(store),
				   COL_PTR, iter, ignore);
}

static void sig_ignore_created(Ignore *ignore)
{
	GtkTreeIter iter;

	if (!store_find_ignore(ignore_store, &iter, ignore))
		gtk_list_store_prepend(ignore_store, &iter);

	gtk_list_store_set(ignore_store, &iter,
			   COL_PTR, ignore,
			   -1);
}

static void sig_ignore_destroyed(Ignore *ignore)
{
	GtkTreeIter iter;

	if (store_find_ignore(ignore_store, &iter, ignore))
		gtk_list_store_remove(ignore_store, &iter);
}

static void setup_ignore_signals_init(void)
{
	signal_add("ignore created", (SIGNAL_FUNC) sig_ignore_created);
	signal_add("ignore changed", (SIGNAL_FUNC) sig_ignore_created);
	signal_add("ignore destroyed", (SIGNAL_FUNC) sig_ignore_destroyed);
}

static void setup_ignore_signals_deinit(void)
{
	signal_remove("ignore created", (SIGNAL_FUNC) sig_ignore_created);
	signal_remove("ignore changed", (SIGNAL_FUNC) sig_ignore_created);
	signal_remove("ignore destroyed", (SIGNAL_FUNC) sig_ignore_destroyed);
}
