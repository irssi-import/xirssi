/*
 setup-aliases.c : irssi

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

#include "gui.h"

static void alias_signals_init(void);
static void alias_signals_deinit(void);

enum {
	COL_ALIAS,
	COL_VALUE,
	COL_EDITABLE,

	N_COLUMNS
};

static GtkTreeView *alias_view = NULL;
static GtkTreeStore *alias_store = NULL;

static int tree_find_alias(GtkTreeModel *model, GtkTreeIter *iter,
			   GtkTreePath *skip_path, const char *alias)
{
	GtkTreePath *path;
	char *iter_alias;
	int match;

	if (!gtk_tree_model_get_iter_first(model, iter))
		return FALSE;

	do {
		gtk_tree_model_get(model, iter, COL_ALIAS, &iter_alias, -1);
		match = strcasecmp(iter_alias, alias) == 0;
		g_free(iter_alias);

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
	alias_signals_deinit();
	alias_store = NULL;
	alias_view = NULL;
	return FALSE;
}

static void alias_store_fill(GtkTreeStore *store)
{
	GtkTreeIter iter;
	CONFIG_NODE *node;
	GSList *tmp;

	node = iconfig_node_traverse("aliases", FALSE);
	if (node == NULL)
		return;

	for (tmp = config_node_first(node->value); tmp != NULL;
	     tmp = config_node_next(tmp)) {
		node = tmp->data;

		if (!has_node_value(node))
			continue;

		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store, &iter,
				   COL_ALIAS, node->key,
				   COL_VALUE, node->value,
				   COL_EDITABLE, TRUE,
				   -1);
	}
}

static void alias_edited(GtkCellRendererText *cell, char *path_string,
			 char *new_text)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	char *old_text, *value;

	if (*new_text == '\0') {
		gui_popup_error("Alias name can't be empty");
		return;
	}

	/* check for duplicate */
	model = GTK_TREE_MODEL(alias_store);
        path = gtk_tree_path_new_from_string(path_string);

	if (tree_find_alias(model, &iter, path, new_text))
		gui_popup_error("Duplicate alias name");
	else {
		/* get old name + value */
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter,
				   COL_ALIAS, &old_text,
				   COL_VALUE, &value, -1);

		/* set new */
		gtk_tree_store_set(alias_store, &iter,
				   COL_ALIAS, new_text, -1);

		/* update config */
		iconfig_set_str("aliases", old_text, NULL);
		iconfig_set_str("aliases", new_text, value);
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
	char *alias;

	if (*new_text == '\0') {
		gui_popup_error("Alias value can't be empty");
		return;
	}

	model = GTK_TREE_MODEL(alias_store);
	path = gtk_tree_path_new_from_string(path_string);

	/* update store */
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_store_set(alias_store, &iter,
			   COL_VALUE, new_text, -1);

	/* update config */
	gtk_tree_model_get(model, &iter, COL_ALIAS, &alias, -1);
	iconfig_set_str("aliases", alias, new_text);
	g_free(alias);

	gtk_tree_path_free(path);
}

static int timeout_edit_alias(GtkTreePath *path)
{
	GtkTreeViewColumn *column;

	column = gtk_tree_view_get_column(alias_view, COL_ALIAS);
	gtk_tree_view_set_cursor(alias_view, path, column, TRUE);
	gtk_tree_path_free(path);

	return 0;
}

static gboolean event_add_alias(GtkWidget *widget, GtkTreeView *tree_view)
{
	GtkTreePath *path;
	GtkTreeIter iter;

	gtk_tree_store_prepend(alias_store, &iter, NULL);
	gtk_tree_store_set(alias_store, &iter,
			   COL_ALIAS, "<name>",
			   COL_VALUE, "<value>",
			   COL_EDITABLE, TRUE, -1);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(alias_store), &iter);

	/* FIXME: this is quite a horrible kludge, but I can't see any
	   other way to do it - even 0 timeout doesn't work.
	   gtk_tree_view_set_cursor() is buggy with newly prepended items
	   for some reason - hopefully will be fixed soon. And since our
	   alias list is sorted, we can't append it either. */
	g_timeout_add(100, (GSourceFunc) timeout_edit_alias, path);
	return FALSE;
}

static gboolean event_remove_alias(GtkWidget *widget, GtkTreeView *tree_view)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GSList *paths;
	char *alias;

	model = GTK_TREE_MODEL(alias_store);

	paths =  gui_tree_selection_get_paths(tree_view);
	while (paths != NULL) {
		GtkTreePath *path = paths->data;

		/* remove from tree */
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, COL_ALIAS, &alias, -1);
		gtk_tree_store_remove(alias_store, &iter);

		/* remove from config */
		iconfig_set_str("aliases", alias, NULL);
		g_free(alias);

		paths = g_slist_remove(paths, path);
		gtk_tree_path_free(path);
	}
	return FALSE;
}

void setup_aliases_init(GtkWidget *dialog)
{
	GtkWidget *button;
	GtkTreeView *tree_view;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	g_signal_connect(G_OBJECT(dialog), "destroy",
			 G_CALLBACK(event_destroy), NULL);

	/* tree store */
	alias_store = gtk_tree_store_new(N_COLUMNS,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_BOOLEAN);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(alias_store), 0,
					gui_tree_strcase_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(alias_store), 0,
					     GTK_SORT_ASCENDING);
	alias_store_fill(alias_store);

	/* view */
	alias_view = tree_view =
		g_object_get_data(G_OBJECT(dialog), "alias_tree");
	gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(alias_store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tree_view),
				    GTK_SELECTION_MULTIPLE);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	g_signal_connect(G_OBJECT(renderer), "edited",
			 G_CALLBACK(alias_edited), NULL);
	column = gtk_tree_view_column_new_with_attributes("Alias", renderer,
							  "text", COL_ALIAS,
							  "editable", COL_EDITABLE,
							  NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	g_signal_connect(G_OBJECT(renderer), "edited",
			 G_CALLBACK(value_edited), NULL);
	column = gtk_tree_view_column_new_with_attributes("Value", renderer,
							  "text", COL_VALUE,
							  "editable", COL_EDITABLE,
							  NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* buttons */
	button = g_object_get_data(G_OBJECT(dialog), "alias_add");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_add_alias), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "alias_remove");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_remove_alias), tree_view);

	alias_signals_init();
}

static void tree_remove_alias(const char *alias)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = GTK_TREE_MODEL(alias_store);
	if (tree_find_alias(model, &iter, NULL, alias))
		gtk_tree_store_remove(alias_store, &iter);
}

static void sig_alias_added(const char *alias, const char *value)
{
	GtkTreeIter iter;

        tree_remove_alias(alias);

	gtk_tree_store_append(alias_store, &iter, NULL);
	gtk_tree_store_set(alias_store, &iter,
			   COL_ALIAS, alias,
			   COL_VALUE, value,
			   COL_EDITABLE, TRUE,
			   -1);
}

static void sig_alias_removed(const char *alias)
{
        tree_remove_alias(alias);
}

static void alias_signals_init(void)
{
	signal_add("alias added", (SIGNAL_FUNC) sig_alias_added);
	signal_add("alias removed", (SIGNAL_FUNC) sig_alias_removed);
}

static void alias_signals_deinit(void)
{
	signal_remove("alias added", (SIGNAL_FUNC) sig_alias_added);
	signal_remove("alias removed", (SIGNAL_FUNC) sig_alias_removed);
}
