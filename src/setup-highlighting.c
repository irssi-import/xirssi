/*
 setup-highlighting.c : irssi

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
#include "misc.h"

#include "hilight-text.h"

#include "gui.h"

void highlight_dialog_show(Highlight *highlight);

static void setup_highlight_signals_init(void);
static void setup_highlight_signals_deinit(void);

enum {
	COL_PTR,
	COL_TEXT,
	COL_CHANNELS,
	COL_LEVEL,
	COL_PRIORITY,

	N_COLUMNS
};

static GtkListStore *highlight_store = NULL;

static gint highlight_sort_func(GtkTreeModel *model,
				GtkTreeIter *a, GtkTreeIter *b,
				gpointer user_data)
{
	Highlight *hi1, *hi2;

	gtk_tree_model_get(model, a, COL_PTR, &hi1, -1);
	gtk_tree_model_get(model, b, COL_PTR, &hi2, -1);

	if (hi1->priority < hi2->priority)
		return -1;
	if (hi1->priority > hi2->priority)
		return 1;

	return strcasecmp(hi1->text == NULL ? "" : hi1->text,
			  hi2->text == NULL ? "" : hi2->text);
}

static gboolean event_destroy(GtkWidget *widget)
{
	setup_highlight_signals_deinit();
	highlight_store = NULL;
	return FALSE;
}

static void highlight_store_fill(GtkListStore *store)
{
	GtkTreeIter iter;
	GSList *tmp;

	for (tmp = hilights; tmp != NULL; tmp = tmp->next) {
		Highlight *highlight = tmp->data;

		gtk_list_store_prepend(store, &iter);
		gtk_list_store_set(store, &iter, COL_PTR, highlight, -1);
	}
}

static gboolean event_add(GtkWidget *widget, GtkTreeView *view)
{
	highlight_dialog_show(NULL);
	return FALSE;
}

static void selection_edit(GtkTreeModel *model, GtkTreePath *path,
			   GtkTreeIter *iter, gpointer data)
{
	Highlight *highlight;

	gtk_tree_model_get(model, iter, COL_PTR, &highlight, -1);
	highlight_dialog_show(highlight);
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
	Highlight *highlight;

	/* remove from config */
	gtk_tree_model_get(model, iter, COL_PTR, &highlight, -1);
	hilight_remove(highlight);

	return TRUE;
}

static gboolean event_remove(GtkWidget *widget, GtkTreeView *view)
{
	g_object_set_data(G_OBJECT(highlight_store), "removing",
			  GINT_TO_POINTER(1));
	gui_tree_selection_delete(GTK_TREE_MODEL(highlight_store), view,
				  remove_iter_func, NULL);
	g_object_set_data(G_OBJECT(highlight_store), "removing", NULL);
	return FALSE;
}

static void text_set_func(GtkTreeViewColumn *column,
			  GtkCellRenderer   *cell,
			  GtkTreeModel      *model,
			  GtkTreeIter       *iter,
			  gpointer           data)
{
	Highlight *highlight;

	gtk_tree_model_get(model, iter, COL_PTR, &highlight, -1);
	g_object_set(G_OBJECT(cell), "text", highlight->text, NULL);
}

static void channels_set_func(GtkTreeViewColumn *column,
			      GtkCellRenderer   *cell,
			      GtkTreeModel      *model,
			      GtkTreeIter       *iter,
			      gpointer           data)
{
	Highlight *highlight;
	char *channels;

	gtk_tree_model_get(model, iter, COL_PTR, &highlight, -1);

	channels = highlight->channels == NULL ? NULL :
		g_strjoinv("\n", highlight->channels);
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
	Highlight *highlight;
	char *level;

	gtk_tree_model_get(model, iter, COL_PTR, &highlight, -1);

	level = bits2level(highlight->level == 0 ? MSGLEVEL_ALL : highlight->level);
	g_object_set(G_OBJECT(cell), "text", level, NULL);
	g_free(level);
}

static void priority_set_func(GtkTreeViewColumn *column,
			      GtkCellRenderer   *cell,
			      GtkTreeModel      *model,
			      GtkTreeIter       *iter,
			      gpointer           data)
{
	Highlight *highlight;
	char str[MAX_INT_STRLEN];

	gtk_tree_model_get(model, iter, COL_PTR, &highlight, -1);

	if (highlight->priority == 0)
		str[0] = '\0';
	else
		ltoa(str, highlight->priority);

	g_object_set(G_OBJECT(cell), "text", str, NULL);
}

void setup_highlighting_init(GtkWidget *dialog)
{
	GtkWidget *button;
	GtkTreeView *tree_view;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	g_signal_connect(G_OBJECT(dialog), "destroy",
			 G_CALLBACK(event_destroy), NULL);

	/* tree store */
	highlight_store = gtk_list_store_new(N_COLUMNS,
					     G_TYPE_POINTER,
					     G_TYPE_STRING,
					     G_TYPE_STRING,
					     G_TYPE_STRING,
					     G_TYPE_STRING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(highlight_store), 0,
					highlight_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(highlight_store),
					     COL_PTR, GTK_SORT_ASCENDING);
        highlight_store_fill(highlight_store);

	/* view */
	tree_view = g_object_get_data(G_OBJECT(dialog), "highlight_tree");
	gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(highlight_store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tree_view),
				    GTK_SELECTION_MULTIPLE);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "yalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes("Text", renderer,
							  "text", COL_TEXT,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						text_set_func, NULL, NULL);
	gtk_tree_view_column_set_min_width(column, 150);
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
	g_object_set(G_OBJECT(renderer), "yalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes("Level", renderer,
							  "text", COL_LEVEL,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						level_set_func, NULL, NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "yalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes("Priority", renderer,
							  "text", COL_PRIORITY,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						priority_set_func, NULL, NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* buttons */
	button = g_object_get_data(G_OBJECT(dialog), "highlight_add");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_add), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "highlight_edit");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_edit), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "highlight_remove");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_remove), tree_view);

	setup_highlight_signals_init();
}

static int store_find_highlight(GtkListStore *store, GtkTreeIter *iter,
			     Highlight *highlight)
{
	return gui_tree_model_find(GTK_TREE_MODEL(store),
				   COL_PTR, iter, highlight);
}

static void sig_highlight_created(Highlight *highlight)
{
	GtkTreeIter iter;

	if (!store_find_highlight(highlight_store, &iter, highlight))
		gtk_list_store_prepend(highlight_store, &iter);

	gtk_list_store_set(highlight_store, &iter,
			   COL_PTR, highlight,
			   -1);
}

static void sig_highlight_destroyed(Highlight *highlight)
{
	GtkTreeIter iter;

	if (g_object_get_data(G_OBJECT(highlight_store), "removing") != NULL)
		return;

	if (store_find_highlight(highlight_store, &iter, highlight))
		gtk_list_store_remove(highlight_store, &iter);
}

static void setup_highlight_signals_init(void)
{
	signal_add("hilight created", (SIGNAL_FUNC) sig_highlight_created);
	signal_add("hilight destroyed", (SIGNAL_FUNC) sig_highlight_destroyed);
}

static void setup_highlight_signals_deinit(void)
{
	signal_remove("hilight created", (SIGNAL_FUNC) sig_highlight_created);
	signal_remove("hilight destroyed", (SIGNAL_FUNC) sig_highlight_destroyed);
}
