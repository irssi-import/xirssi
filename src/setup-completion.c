/*
 setup-servers.c : irssi

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
#include "settings.h"
#include "lib-config/iconfig.h"

#include "setup.h"
#include "gui.h"

static void completion_signals_init(void);
static void completion_signals_deinit(void);

enum {
	COL_KEY,
	COL_VALUE,
	COL_AUTO,

	N_COLUMNS
};

static GtkTreeView *completion_view = NULL;
static GtkTreeStore *completion_store = NULL;

static int tree_find_completion(GtkTreeModel *model, GtkTreeIter *iter,
				GtkTreePath *skip_path, const char *key)
{
	GtkTreePath *path;
	char *iter_key;
	int match;

	if (!gtk_tree_model_get_iter_first(model, iter))
		return FALSE;

	do {
		gtk_tree_model_get(model, iter, COL_KEY, &iter_key, -1);
		match = strcasecmp(iter_key, key) == 0;
		g_free(iter_key);

		if (match && skip_path != NULL) {
			path = gtk_tree_model_get_path(model, iter);
			match = gtk_tree_path_compare(path, skip_path) != 0;
			gtk_tree_path_free(path);
		}

		if (match)
			return TRUE;
	} while (gtk_tree_model_iter_next(model, iter));

	return FALSE;
}

static gboolean event_destroy(GtkWidget *widget)
{
	completion_signals_deinit();
	completion_store = NULL;
	completion_view = NULL;
	return FALSE;
}

static void completion_store_fill(GtkTreeStore *store)
{
	GtkTreeIter iter;
	CONFIG_NODE *node;
	GSList *tmp;

	node = iconfig_node_traverse("completions", FALSE);
	if (node == NULL)
		return;

	for (tmp = node->value; tmp != NULL; tmp = tmp->next) {
		node = tmp->data;

		if (node->type != NODE_TYPE_BLOCK)
			continue;

		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store, &iter,
				   COL_KEY, node->key,
				   COL_VALUE, config_node_get_str(node, "value", ""),
				   COL_AUTO, config_node_get_bool(node, "auto", FALSE),
				   -1);
	}
}

static CONFIG_NODE *completion_find_node(const char *key, int create)
{
	CONFIG_NODE *node;

	node = iconfig_node_traverse("completions", create);
	if (node == NULL)
		return NULL;

	return config_node_section(node, key, create ? NODE_TYPE_BLOCK : -1);
}

static void completion_edited(GtkCellRendererText *cell, char *path_string,
			      char *new_text)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	CONFIG_NODE *node;
	char *old_text, *value;
	gboolean automatic;

	if (*new_text == '\0') {
		gui_popup_error("Completion key can't be empty");
		return;
	}

	/* check for duplicate */
	model = GTK_TREE_MODEL(completion_store);
        path = gtk_tree_path_new_from_string(path_string);

	if (tree_find_completion(model, &iter, path, new_text))
		gui_popup_error("Duplicate completion");
	else {
		/* get old name + value */
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter,
				   COL_KEY, &old_text,
				   COL_VALUE, &value,
				   COL_AUTO, &automatic, -1);

		/* set new */
		gtk_tree_store_set(completion_store, &iter,
				   COL_KEY, new_text, -1);

		/* update config */
		iconfig_set_str("completions", old_text, NULL);

		node = completion_find_node(new_text, TRUE);
		iconfig_node_set_str(node, "value", value);
		if (automatic)
			iconfig_node_set_bool(node, "auto", TRUE);

		g_free(old_text);
		g_free(value);
	}

	gtk_tree_path_free(path);
}

static void value_edited(GtkCellRendererText *cell, char *path_string,
			 char *new_text)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	char *key;

	if (*new_text == '\0') {
		gui_popup_error("Completion value can't be empty");
		return;
	}

	model = GTK_TREE_MODEL(completion_store);
	path = gtk_tree_path_new_from_string(path_string);

	/* update store */
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_store_set(completion_store, &iter,
			   COL_VALUE, new_text, -1);

	/* update config */
	gtk_tree_model_get(model, &iter, COL_KEY, &key, -1);
	iconfig_node_set_str(completion_find_node(key, TRUE),
			     "value", new_text);
	g_free(key);

	gtk_tree_path_free(path);
}

static void auto_toggled(GtkCellRendererToggle *cell, char *path_string)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	gboolean active;
	char *key;

	model = GTK_TREE_MODEL(completion_store);
	path = gtk_tree_path_new_from_string(path_string);

	active = !gtk_cell_renderer_toggle_get_active(cell);

	/* update store */
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_store_set(completion_store, &iter,
			   COL_AUTO, active, -1);

	/* update config */
	gtk_tree_model_get(model, &iter, COL_KEY, &key, -1);
	iconfig_node_set_bool(completion_find_node(key, TRUE), "auto", active);
	g_free(key);
}

static int timeout_edit_completion(GtkTreePath *path)
{
	GtkTreeViewColumn *column;

	column = gtk_tree_view_get_column(completion_view, COL_KEY);
	gtk_tree_view_set_cursor(completion_view, path, column, TRUE);
	gtk_tree_path_free(path);

	return 0;
}

static gboolean event_add_completion(GtkWidget *widget, GtkTreeView *tree_view)
{
	GtkTreePath *path;
	GtkTreeIter iter;

	gtk_tree_store_prepend(completion_store, &iter, NULL);
	gtk_tree_store_set(completion_store, &iter,
			   COL_KEY, "<key>",
			   COL_VALUE, "<value>", -1);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(completion_store), &iter);

	/* FIXME: this is quite a horrible kludge, but I can't see any
	   other way to do it - even 0 timeout doesn't work.
	   gtk_tree_view_set_cursor() is buggy with newly prepended items
	   for some reason - hopefully will be fixed soon. And since our
	   completion list is sorted, we can't append it either. */
	g_timeout_add(100, (GSourceFunc) timeout_edit_completion, path);
	return FALSE;
}

static gboolean event_remove_completion(GtkWidget *widget,
					GtkTreeView *tree_view)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GSList *paths;
	char *key;

	model = GTK_TREE_MODEL(completion_store);

	paths =  gui_tree_selection_get_paths(tree_view);
	while (paths != NULL) {
		GtkTreePath *path = paths->data;

		/* remove from tree */
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, COL_KEY, &key, -1);
		gtk_tree_store_remove(completion_store, &iter);

		/* remove from config */
		iconfig_set_str("completions", key, NULL);
		g_free(key);

		paths = g_slist_remove(paths, path);
		gtk_tree_path_free(path);
	}
	return FALSE;
}

void setup_completions_init(GtkWidget *dialog)
{
	GtkWidget *button;
	GtkTreeView *tree_view;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	g_signal_connect(G_OBJECT(dialog), "destroy",
			 G_CALLBACK(event_destroy), NULL);

	/* tree store */
	completion_store = gtk_tree_store_new(N_COLUMNS,
					      G_TYPE_STRING,
					      G_TYPE_STRING,
					      G_TYPE_BOOLEAN);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(completion_store), 0,
					gui_tree_strcase_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(completion_store),
					     0, GTK_SORT_ASCENDING);
	completion_store_fill(completion_store);

	/* view */
	completion_view = tree_view =
		g_object_get_data(G_OBJECT(dialog), "completion_tree");
	gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(completion_store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tree_view),
				    GTK_SELECTION_MULTIPLE);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(renderer), "edited",
			 G_CALLBACK(completion_edited), NULL);
	column = gtk_tree_view_column_new_with_attributes("Key", renderer,
							  "text", COL_KEY,
							  NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(renderer), "edited",
			 G_CALLBACK(value_edited), NULL);
	column = gtk_tree_view_column_new_with_attributes("Value", renderer,
							  "text", COL_VALUE,
							  NULL);
	gtk_tree_view_column_set_min_width(column, 250);
	gtk_tree_view_append_column(tree_view, column);

	/* -- */
	renderer = gtk_cell_renderer_toggle_new();
	g_object_set(G_OBJECT(renderer), "activatable", TRUE, NULL);
	g_signal_connect(G_OBJECT(renderer), "toggled",
			 G_CALLBACK(auto_toggled), NULL);
	column = gtk_tree_view_column_new_with_attributes("Auto", renderer,
							  "active", COL_AUTO,
							  NULL);
	gtk_tree_view_column_set_alignment(column, 0.5);
	gtk_tree_view_append_column(tree_view, column);

	/* buttons */
	button = g_object_get_data(G_OBJECT(dialog), "completion_add");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_add_completion), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "completion_remove");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_remove_completion), tree_view);

	completion_signals_init();
}

static void tree_remove_completion(const char *key)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = GTK_TREE_MODEL(completion_store);
	if (tree_find_completion(model, &iter, NULL, key))
		gtk_tree_store_remove(completion_store, &iter);
}

static void sig_completion_added(const char *key)
{
	GtkTreeIter iter;
	CONFIG_NODE *node;

	node = completion_find_node(key, FALSE);
	if (node == NULL)
		return;

        tree_remove_completion(key);

	gtk_tree_store_append(completion_store, &iter, NULL);
	gtk_tree_store_set(completion_store, &iter,
			   COL_KEY, key,
			   COL_VALUE, config_node_get_str(node, "value", ""),
			   COL_AUTO, config_node_get_bool(node, "auto", FALSE),
			   -1);
}

static void sig_completion_removed(const char *key)
{
        tree_remove_completion(key);
}

static void completion_signals_init(void)
{
	signal_add("completion added", (SIGNAL_FUNC) sig_completion_added);
	signal_add("completion removed", (SIGNAL_FUNC) sig_completion_removed);
}

static void completion_signals_deinit(void)
{
	signal_remove("completion added", (SIGNAL_FUNC) sig_completion_added);
	signal_remove("completion removed", (SIGNAL_FUNC) sig_completion_removed);
}
