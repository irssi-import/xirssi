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

static GtkTreeStore *alias_store = NULL;

static gboolean event_destroy(GtkWidget *widget)
{
	alias_signals_deinit();
	alias_store = NULL;
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
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_string(path_string);

	gtk_tree_model_get_iter(GTK_TREE_MODEL(alias_store), &iter, path);
	gtk_tree_store_set(alias_store, &iter,
			   COL_ALIAS, new_text, -1);
	gtk_tree_path_free(path);

	/* FIXME: update config file, check for dupes */
}

static void value_edited(GtkCellRendererText *cell, char *path_string,
			 char *new_text)
{
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_string(path_string);

	gtk_tree_model_get_iter(GTK_TREE_MODEL(alias_store), &iter, path);
	gtk_tree_store_set(alias_store, &iter,
			   COL_VALUE, new_text, -1);
	gtk_tree_path_free(path);

	/* FIXME: update config file */
}

void setup_aliases_init(GtkWidget *dialog)
{
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
	tree_view = g_object_get_data(G_OBJECT(dialog), "alias_tree");
	/*g_signal_connect(G_OBJECT(tree_view), "button_press_event",
			 G_CALLBACK(event_button_press), NULL);*/

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

	alias_signals_init();
}

static void tree_remove_alias(const char *alias)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *iter_alias;

        model = GTK_TREE_MODEL(alias_store);
	if (!gtk_tree_model_get_iter_first(model, &iter))
		return;

	do {
		gtk_tree_model_get(model, &iter, COL_ALIAS, &iter_alias, NULL);
		if (strcasecmp(iter_alias, alias) == 0) {
			g_free(iter_alias);
			gtk_tree_store_remove(alias_store, &iter);
			break;
		}
		g_free(iter_alias);
	} while (gtk_tree_model_iter_next(model, &iter));
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
