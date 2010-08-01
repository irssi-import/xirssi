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
#include "gui-menu.h"

#include "ball-blue.h"
#include "ball-pink.h"
#include "ball-red.h"
#include "ball-green.h"
#include "ball-yellow.h"
#include "ball-lightblue.h"
#include "ball-orange.h"

static GdkPixbuf *status_pixbufs[256];

static gboolean event_destroy(GtkWidget *widget, NicklistView *view)
{
	signal_emit("gui nicklist view destroyed", 1, view);

	g_object_set_data(G_OBJECT(view->widget), "NicklistView", NULL);
	g_free(view);
	return FALSE;
}

static void sel_get_nicks(GtkTreeModel *model, GtkTreePath *path,
			  GtkTreeIter *iter, gpointer data)
{
	GString *nicks = data;
	Nick *nick;

	gtk_tree_model_get(model, iter, 0, &nick, -1);
	if (nick != NULL) {
		if (nicks->len > 0)
			g_string_append_c(nicks, ' ');
		g_string_append(nicks, nick->nick);
	}
}

static const char *get_nicks(Server **server, Channel **channel,
			     void *user_data)
{
	NicklistView *view = user_data;
	GtkTreeSelection *sel;
	GString *nicks;

	if (server == NULL) {
		/* destroy */
		nicks = g_object_get_data(G_OBJECT(view->widget),
					  "selected_nicks");
		g_string_free(nicks, TRUE);

		gtk_widget_unref(GTK_WIDGET(view->view));
		gtk_widget_unref(view->widget);
		return NULL;
	}

	if (view->nicklist == NULL) {
		/* channel was destroyed */
		return NULL;
	}

	/* get selected nicks */
	nicks = g_string_new(NULL);
	sel = gtk_tree_view_get_selection(view->view);
	gtk_tree_selection_selected_foreach(sel, sel_get_nicks, nicks);

	if (nicks->len == 0) {
		/* no nicks selected */
		g_string_free(nicks, TRUE);
		return NULL;
	}

	*channel = view->nicklist->channel;
	*server = (*channel)->server;
	g_object_set_data(G_OBJECT(view->widget), "selected_nicks", nicks);
	return nicks->str;
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

	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		/* left-doubleclick - open the nick under mouse in query */
		gtk_tree_model_get(model, &iter, 0, &nick, -1);
		if (nick != NULL) {
			signal_emit("command whois", 2, nick->nick,
				    view->nicklist->channel->server);
		}
	} else if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
		/* right mouse click - show the popup menu. */
		gtk_widget_ref(view->widget);
		gtk_widget_ref(GTK_WIDGET(view->view));
		gui_menu_nick_popup(get_nicks, view, event->button);
	}
	return FALSE;
}

static Nick *get_nick_at(NicklistView *view, int x, int y)
{
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter iter;
	Nick *nick;

	/* get path to item under mouse */
	if (!gtk_tree_view_get_path_at_pos(view->view, x, y,
					   &path, NULL, NULL, NULL))
		return NULL;

	/* get iterator */
	model = GTK_TREE_MODEL(view->nicklist->store);
	if (!gtk_tree_model_get_iter(model, &iter, path))
		return NULL;

	gtk_tree_model_get(model, &iter, 0, &nick, -1);
	return nick;
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

static void nick_set_func_pixbuf(GtkTreeViewColumn *column,
				 GtkCellRenderer   *cell,
				 GtkTreeModel      *model,
				 GtkTreeIter       *iter,
				 gpointer           data)
{
	Nick *nick;
	GdkPixbuf *pixbuf;

	gtk_tree_model_get(model, iter, 0, &nick, -1);

	pixbuf = status_pixbufs[(unsigned char) *nick->prefixes];

	g_object_set(GTK_CELL_RENDERER(cell), "pixbuf", pixbuf, NULL);
}

static void nick_set_func_text(GtkTreeViewColumn *column,
			       GtkCellRenderer   *cell,
			       GtkTreeModel      *model,
			       GtkTreeIter       *iter,
			       gpointer           data)
{
	Nick *nick;

	gtk_tree_model_get(model, iter, 0, &nick, -1);

	g_object_set(G_OBJECT(cell), "background-gdk", NULL, NULL);
	g_object_set(G_OBJECT(cell), "text", nick->nick, NULL);
}

NicklistView *gui_nicklist_view_new(Tab *tab)
{
	GtkWidget *space, *sw, *list, *vbox, *frame;
	GtkCellRenderer *renderer;
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
	g_signal_connect(G_OBJECT(list), "motion_notify_event",
			 G_CALLBACK(event_motion), view);
	g_signal_connect(G_OBJECT(list), "leave_notify_event",
			 G_CALLBACK(event_leave), view);
	view->view = GTK_TREE_VIEW(list);
	gtk_container_add(GTK_CONTAINER(sw), list);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(view->view),
				    GTK_SELECTION_MULTIPLE);

	/* nick name text */
	view->column = gtk_tree_view_column_new();
        gtk_tree_view_column_set_alignment(view->column, 0.5);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start(view->column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(view->column, renderer,
						nick_set_func_pixbuf,
						NULL, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(view->column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(view->column, renderer,
						nick_set_func_text,
						NULL, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), view->column);

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

void gui_nicklist_view_update_label(NicklistView *view, const char *label)
{
	gtk_tree_view_column_set_title(view->column, label);
}

void gui_nicklist_views_init(void)
{
	memset(&status_pixbufs, 0, sizeof(status_pixbufs));

	status_pixbufs['~'] = gdk_pixbuf_new_from_inline(sizeof(ball_pink),
						ball_pink, FALSE, NULL);
	status_pixbufs['!'] = gdk_pixbuf_new_from_inline(sizeof(ball_red),
						ball_red, FALSE, NULL);
	status_pixbufs['&'] = gdk_pixbuf_new_from_inline(sizeof(ball_red),
						ball_red, FALSE, NULL);
	status_pixbufs['@'] = gdk_pixbuf_new_from_inline(sizeof(ball_green),
					       ball_green, FALSE, NULL);
	status_pixbufs['%'] = gdk_pixbuf_new_from_inline(sizeof(ball_lightblue),
						   ball_lightblue, FALSE, NULL);
	status_pixbufs['+'] = gdk_pixbuf_new_from_inline(sizeof(ball_yellow),
						  ball_yellow, FALSE, NULL);

	/* inspircd/conferenceroom halfvoice... */
	status_pixbufs['-'] = gdk_pixbuf_new_from_inline(sizeof(ball_orange),
						  ball_orange, FALSE, NULL);

	/* inspircd operprefix, bleah. */
	status_pixbufs['*'] = gdk_pixbuf_new_from_inline(sizeof(ball_blue),
						ball_blue, FALSE, NULL);
}

void gui_nicklist_views_deinit(void)
{
	gint i;

	for (i = 0; i < 256; i++)
	{
		if (status_pixbufs[i] != NULL)
			gdk_pixbuf_unref(status_pixbufs[i]);
	}
}
