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
#include "gui-menu-nick.h"

static gboolean event_destroy(GtkWidget *widget, NicklistView *view)
{
	signal_emit("gui nicklist view destroyed", 1, view);

	view->widget = NULL;
	view->label = NULL;
	view->view = NULL;

	gui_nicklist_view_unref(view);
	gui_tab_unref(view->tab);
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

	if (event->button != 1 || event->type != GDK_2BUTTON_PRESS)
		return FALSE;

	/* left-doubleclick - open the nick under mouse in query */
	if (!gtk_tree_view_get_path_at_pos(tree, event->x, event->y,
					   &path, NULL, NULL, NULL))
		return FALSE;

	model = GTK_TREE_MODEL(view->nicklist->store);
	if (gtk_tree_model_get_iter(model, &iter, path)) {
		nick = tree_get_nick(model, &iter);
		if (nick != NULL) {
			signal_emit("command query", 2, nick,
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

	/* popup menu */
	gui_menu_nick_popup(view->nicklist->channel->server,
			    view->nicklist->channel, nicks, event);
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

NicklistView *gui_nicklist_view_new(Tab *tab)
{
	GtkWidget *sw, *list, *vbox, *frame;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	NicklistView *view;

	view = g_new0(NicklistView, 1);
	view->refcount = 1;

	view->tab = tab;
	gui_tab_ref(view->tab);

	view->widget = vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_set_usize(vbox, 150, -1);
	g_signal_connect(G_OBJECT(vbox), "destroy",
			 G_CALLBACK(event_destroy), view);

	view->label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(vbox), view->label, FALSE, FALSE, 0);

	frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

	/* scrolled window where to place nicklist */
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), sw);

	/* view */
	list = gtk_tree_view_new();
	gtk_signal_connect(GTK_OBJECT(list), "button_press_event",
			   GTK_SIGNAL_FUNC(event_button_press), view);
	gtk_signal_connect(GTK_OBJECT(list), "button_release_event",
			   GTK_SIGNAL_FUNC(event_button_release), view);
	gtk_signal_connect(GTK_OBJECT(list), "motion_notify_event",
			   GTK_SIGNAL_FUNC(event_motion), view);
	gtk_signal_connect(GTK_OBJECT(list), "leave_notify_event",
			   GTK_SIGNAL_FUNC(event_leave), view);
	view->view = GTK_TREE_VIEW(list);
	gtk_container_add(GTK_CONTAINER(sw), list);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(view->view),
				    GTK_SELECTION_MULTIPLE);

	/* nick mode pixmap */
	renderer = gtk_cell_renderer_pixbuf_new();
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
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	signal_emit("gui nicklist view created", 1, view);
	return view;
}

void gui_nicklist_view_ref(NicklistView *view)
{
	view->refcount++;
}

void gui_nicklist_view_unref(NicklistView *view)
{
	if (--view->refcount > 0)
		return;

	if (view->nicklist != NULL)
		gui_nicklist_unref(view->nicklist);
	g_free(view);
}

void gui_nicklist_view_set(NicklistView *view, Nicklist *nicklist)
{
	if (view->nicklist != NULL) {
		view->nicklist->views =
			g_slist_remove(view->nicklist->views, view);
		gui_nicklist_unref(view->nicklist);
	}
	if (nicklist != NULL) {
		gui_nicklist_ref(nicklist);
		nicklist->views = g_slist_prepend(nicklist->views, view);
	}
	view->nicklist = nicklist;

	gtk_tree_view_set_model(GTK_TREE_VIEW(view->view),
				nicklist == NULL ? NULL :
				GTK_TREE_MODEL(nicklist->store));
}

void gui_nicklist_view_label_updated(NicklistView *view)
{
	gtk_label_set(GTK_LABEL(view->label), view->nicklist->label);
}
