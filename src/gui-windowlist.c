/*
 gui-windowlist.c : irssi

    Copyright (C) 2010 William Pitcock

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

#include "printtext.h"

#include "gui-colors.h"
#include "gui-frame.h"
#include "gui-tab.h"
#include "gui-window.h"
#include "gui-window-view.h"
#include "gui-window-context.h"
#include "gui-windowlist.h"

extern char *window_get_label(Window *window);

static const gchar *data_level_get_color(int data_level)
{
        /* get the color */
        switch (data_level) {
        case DATA_LEVEL_NONE:
		return NULL;
                break;
        case DATA_LEVEL_TEXT:
		return "dark red";
                break;
        case DATA_LEVEL_MSG:
		return "red";
                break;
        default:
                break;
        }

	return "magenta";
}

static gint gui_windowlist_sort_func(GtkTreeModel *model,
				     GtkTreeIter *a, GtkTreeIter *b,
				     gpointer user_data)
{
	Tab *tab_a, *tab_b;
	gint pos_a, pos_b;

	gtk_tree_model_get(model, a, 0, &tab_a, -1);
	gtk_tree_model_get(model, b, 0, &tab_b, -1);

	pos_a = gtk_notebook_page_num(GTK_NOTEBOOK(tab_a->frame->notebook), tab_a->widget);
	pos_b = gtk_notebook_page_num(GTK_NOTEBOOK(tab_b->frame->notebook), tab_b->widget);

	return (pos_a - pos_b);
}

static void tab_id_set_func(GtkTreeViewColumn *column,
			    GtkCellRenderer   *cell,
			    GtkTreeModel      *model,
			    GtkTreeIter       *iter,
			    gpointer           data)
{
	Tab *tab;
	gchar tabid[20];
	const gchar *color = NULL;

	gtk_tree_model_get(model, iter, 0, &tab, -1);

	g_snprintf(tabid, 20, "%d:", gtk_notebook_page_num(GTK_NOTEBOOK(tab->frame->notebook), tab->widget) + 1);

	color = data_level_get_color(tab->data_level);

	g_object_set(G_OBJECT(cell), "text", tabid, "foreground", color, NULL);
}

static void tab_name_set_func(GtkTreeViewColumn *column,
			      GtkCellRenderer   *cell,
			      GtkTreeModel      *model,
			      GtkTreeIter       *iter,
			      gpointer           data)
{
	Tab *tab;
	gchar *tabid;
	const gchar *color = NULL;

	gtk_tree_model_get(model, iter, 0, &tab, -1);

	tabid = window_get_label(tab->active_win);
	color = data_level_get_color(tab->data_level);

	g_object_set(G_OBJECT(cell), "text", tabid, "foreground", color, NULL);

	g_free(tabid);
}

static gboolean gui_windowlist_notebook_page_switched(GtkNotebook *notebook,
						      GtkNotebookPage *page,
						      gint index,
						      Frame *frame)
{
	GtkTreePath *path;
	GtkTreeSelection *sel;

	path = gtk_tree_path_new_from_indices(index, -1);
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(frame->winlist->treeview));

	g_signal_handler_block(sel, frame->winlist->changed_sig);
	gtk_tree_selection_select_path(sel, path);
	g_signal_handler_unblock(sel, frame->winlist->changed_sig);

	gtk_tree_path_free(path);

	return TRUE;
}

static void gui_windowlist_switch_tab(Frame *frame,
				      GtkTreeSelection *selection)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	Tab *tab;
	gint index;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	gtk_tree_model_get(model, &iter, 0, &tab, -1);

	index = gtk_notebook_page_num(GTK_NOTEBOOK(tab->frame->notebook), tab->widget);
	gtk_notebook_set_current_page(tab->frame->notebook, index);
}

WindowList *gui_windowlist_new(Frame *frame)
{
	WindowList *winlist;
	GtkWidget *win;
	GtkListStore *store;
	GtkWidget *tv;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;

	winlist = g_new0(WindowList, 1);

	winlist->frame = frame;
	winlist->store = store = gtk_list_store_new(1, G_TYPE_POINTER);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), 0,
					gui_windowlist_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), 0,
					     GTK_SORT_ASCENDING);

	winlist->widget = win = gtk_scrolled_window_new(0, 0);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(win), GTK_SHADOW_IN);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(win), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	winlist->treeview = tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(winlist->store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), FALSE);
	gtk_container_add(GTK_CONTAINER(win), tv);

	col = gtk_tree_view_column_new();

        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, tab_id_set_func, NULL, NULL);

        renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_set_alignment(renderer, 1.0, 0.5);
	gtk_tree_view_column_pack_end(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, tab_name_set_func, NULL, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	winlist->changed_sig = g_signal_connect_swapped(selection, "changed",
							G_CALLBACK(gui_windowlist_switch_tab), frame);
	g_signal_connect(frame->notebook, "switch-page",
			 G_CALLBACK(gui_windowlist_notebook_page_switched), frame);

	return winlist;
}

static void gui_windowlist_new_tab(Tab *tab)
{
	GtkTreeIter iter;

	gtk_list_store_append(tab->frame->winlist->store, &iter);	
	gtk_list_store_set(tab->frame->winlist->store, &iter, 0, tab, -1);
}

static void gui_windowlist_destroy_tab(Tab *tab)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	Tab *val_tab;

	model = GTK_TREE_MODEL(tab->frame->winlist->store);
	gtk_tree_model_get_iter_first(model, &iter);
	do {
		gtk_tree_model_get(model, &iter, 0, &val_tab, -1);
		if (val_tab == tab) {
			gtk_list_store_remove(tab->frame->winlist->store, &iter);
			break;
		}
	} while (gtk_tree_model_iter_next(model, &iter));
}

static void gui_windowlist_update_window_activity(Window *window)
{
	WindowGui *gui;
	WindowView *view;
	TabPane *pane;
	Tab *tab;

	if (!gui_window_is_visible(window) || window->data_level == 0)
                return;

	gui = WINDOW_GUI(window);
	g_return_if_fail(gui != NULL);

	view = gui->views->data;
	g_return_if_fail(view != NULL);

	pane = view->pane;
	g_return_if_fail(pane != NULL);

	tab = pane->tab;
	g_return_if_fail(tab != NULL);

	gtk_widget_queue_draw(tab->frame->winlist->treeview);
}

void gui_windowlist_init(void)
{
	signal_add("gui tab created", (SIGNAL_FUNC) gui_windowlist_new_tab);
	signal_add("gui tab destroyed", (SIGNAL_FUNC) gui_windowlist_destroy_tab);

	signal_add("window hilight", (SIGNAL_FUNC) gui_windowlist_update_window_activity);
	signal_add("window changed", (SIGNAL_FUNC) gui_windowlist_update_window_activity);
}

void gui_windowlist_deinit(void)
{
	signal_remove("gui tab created", (SIGNAL_FUNC) gui_windowlist_new_tab);
	signal_remove("gui tab destroyed", (SIGNAL_FUNC) gui_windowlist_destroy_tab);

	signal_remove("window hilight", (SIGNAL_FUNC) gui_windowlist_update_window_activity);
	signal_remove("window changed", (SIGNAL_FUNC) gui_windowlist_update_window_activity);
}
