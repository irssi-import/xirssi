/*
 gui-nicklist-view.c : irssi

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
#include "channels.h"

#include "gui-tab.h"
#include "gui-nicklist.h"
#include "gui-nicklist-view.h"
#include "gui-nicklist-cell-renderer.h"
#include "gui-menu.h"

static gboolean event_destroy(GtkWidget *widget, NicklistView *view)
{
	signal_emit("gui nicklist view destroyed", 1, view);

	g_object_set_data(G_OBJECT(view->widget), "NicklistView", NULL);
	g_free(view);
	return FALSE;
}

static Nick *tree_get_nick(GtkTreeModel *model, GtkTreeIter *iter)
{
	GValue value;
	Nick *nick;

	memset(&value, 0, sizeof(value));
	gtk_tree_model_get_value(model, iter, NICKLIST_COL_PTR, &value);
	nick = value.data[0].v_pointer;
	g_value_unset(&value);

	return nick;
}

static void sel_get_nicks(GtkTreeModel *model, GtkTreePath *path,
			  GtkTreeIter *iter, gpointer data)
{
	GSList **list = data;
	Nick *nick;

	nick = tree_get_nick(model, iter);
	if (nick != NULL)
		*list = g_slist_prepend(*list, nick->nick);
}

static gboolean event_button_press(GtkTreeView *tree, GdkEventButton *event,
				   NicklistView *view)
{
	GtkTreePath *path;
	GtkTreeModel *model;
        GtkTreeIter iter;
	Nick *nick;

	if (!gtk_tree_view_get_path_at_pos(tree, event->x, event->y,
					   &path, NULL, NULL, NULL))
		return FALSE;

	model = GTK_TREE_MODEL(view->nicklist->store);
	if (!gtk_tree_model_get_iter(model, &iter, path))
		return FALSE;

	if (event->type == GDK_BUTTON_PRESS && !gtk_tree_path_prev(path)) {
		/* clicked on first row - don't allow it */
		return TRUE;
	} else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		/* left-doubleclick - open the nick under mouse in query */
		nick = tree_get_nick(model, &iter);
		if (nick != NULL) {
			signal_emit("command query", 2, nick->nick,
				    view->nicklist->channel->server);
		}
	}
	return FALSE;
}

static gboolean event_button_release(GtkTreeView *tree, GdkEventButton *event,
				     NicklistView *view)
{
	GtkTreeSelection *sel;
	GSList *nicks;

	if (event->button != 3)
		return FALSE;

	/* right mouse click - show the popup menu. */

	/* first get selected nicks to linked list */
	nicks = NULL;
	sel = gtk_tree_view_get_selection(view->view);
        gtk_tree_selection_selected_foreach(sel, sel_get_nicks, &nicks);

	if (nicks == NULL) {
		/* no nicks selected */
		return FALSE;
	}

	/* popup menu - FIXME: try to get this in button_press_event.
	   it just has the problem of not updating the selections yet,
	   one possible solution would be to read the selected nicks later
	   which might actually be very good idea since selected nicks might
	   leave the channel while popup menu is being open.. */
	gui_menu_nick_popup(view->nicklist->channel->server,
			    view->nicklist->channel, nicks, 0);
	g_slist_free(nicks);
	return FALSE;
}

static Nick *get_nick_at(NicklistView *view, int x, int y)
{
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter iter;

	/* get path to item under mouse */
	if (!gtk_tree_view_get_path_at_pos(view->view, x, y,
					   &path, NULL, NULL, NULL))
		return NULL;

	/* get iterator */
	model = GTK_TREE_MODEL(view->nicklist->store);
	if (!gtk_tree_model_get_iter(model, &iter, path))
		return NULL;

	return tree_get_nick(model, &iter);
}

static gboolean event_motion(GtkWidget *tree, GdkEventButton *event,
			     NicklistView *view)
{
	Nick *nick, *old_nick;

	/* needed to call this so the motion events won't get stuck */
	gdk_window_get_pointer(tree->window, NULL, NULL, NULL);

	nick = get_nick_at(view, event->x, event->y);
	old_nick = g_object_get_data(G_OBJECT(tree), "nick");

	if (old_nick == nick) {
		/* same as last time - don't do anything */
		return FALSE;
	}
	g_object_set_data(G_OBJECT(tree), "nick", nick);

	if (nick != NULL)
		signal_emit("gui nicklist enter", 2, view, nick);
	else
		signal_emit("gui nicklist leave", 2, view, old_nick);

	return FALSE;
}

static gboolean event_leave(GtkWidget *tree, GdkEventCrossing *event,
			    NicklistView *view)
{
	Nick *old_nick;

	old_nick = g_object_get_data(G_OBJECT(tree), "nick");
	if (old_nick != NULL) {
		g_object_set_data(G_OBJECT(tree), "nick", NULL);
		signal_emit("gui nicklist leave", 2, view, old_nick);
	}
	return FALSE;
}

static void render_nickname(GtkTreeViewColumn *column,
			    GtkCellRenderer   *cell,
			    GtkTreeModel      *model,
			    GtkTreeIter       *iter,
			    gpointer           data)
{
	NicklistView *view = data;
	Nick *nick;

	nick = tree_get_nick(model, iter);
	if (nick == NULL) {
		g_object_set(G_OBJECT(cell), "background-gdk",
			     &view->widget->style->bg[GTK_STATE_NORMAL], NULL);
	} else {
		g_object_set(G_OBJECT(cell), "background-gdk", NULL, NULL);
	}
}

NicklistView *gui_nicklist_view_new(Tab *tab)
{
	GtkWidget *space, *sw, *list, *vbox, *frame;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	NicklistView *view;
	int handle_size;

	view = g_new0(NicklistView, 1);
	view->tab = tab;

	view->widget = vbox = gtk_vbox_new(FALSE, 0);
	g_object_set_data(G_OBJECT(view->widget), "NicklistView", view);

	gtk_widget_set_size_request(vbox, 150, -1);
	g_signal_connect(G_OBJECT(vbox), "destroy",
			 G_CALLBACK(event_destroy), view);

	/* add some empty space so it'll be at the same
	   position as topic entry */
	space = gtk_label_new(NULL);
	gtk_widget_style_get(GTK_WIDGET(tab->last_paned), "handle_size",
			     &handle_size, NULL);
	/* +1? .. no idea, hopefully ok with non-default theme.. */
	gtk_widget_set_size_request(space, -1, handle_size+1);
	gtk_box_pack_start(GTK_BOX(vbox), space, FALSE, FALSE, 0);

	frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE,
			   GTK_WIDGET(tab->first_paned)->allocation.height);

	/* scrolled window where to place nicklist */
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), sw);

	/* view */
	list = gtk_tree_view_new();
	g_signal_connect(G_OBJECT(list), "button_press_event",
			 G_CALLBACK(event_button_press), view);
	g_signal_connect(G_OBJECT(list), "button_release_event",
			 G_CALLBACK(event_button_release), view);
	g_signal_connect(G_OBJECT(list), "motion_notify_event",
			 G_CALLBACK(event_motion), view);
	g_signal_connect(G_OBJECT(list), "leave_notify_event",
			 G_CALLBACK(event_leave), view);
	view->view = GTK_TREE_VIEW(list);
	gtk_container_add(GTK_CONTAINER(sw), list);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(view->view),
				    GTK_SELECTION_MULTIPLE);

	/* nick mode pixmap */
	renderer = gtk_cell_renderer_nicklist_pixmap_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
							  "pixbuf",
							  NICKLIST_COL_PIXMAP,
							  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	/* nick name text */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
							  "text",
							  NICKLIST_COL_NAME,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						render_nickname, view, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	signal_emit("gui nicklist view created", 1, view);
	return view;
}

void gui_nicklist_view_set(NicklistView *view, Nicklist *nicklist)
{
	if (view->nicklist == nicklist)
		return;

	if (view->nicklist != NULL) {
		view->nicklist->views =
			g_slist_remove(view->nicklist->views, view);
	}
	if (nicklist != NULL)
		nicklist->views = g_slist_prepend(nicklist->views, view);
	view->nicklist = nicklist;

	gtk_tree_view_set_model(view->view, nicklist == NULL ? NULL :
				GTK_TREE_MODEL(nicklist->store));
}
