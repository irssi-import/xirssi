/*
 gui-tab.c : irssi

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

#include "gui-frame.h"
#include "gui-tab.h"
#include "gui-tab-move.h"
#include "gui-window.h"
#include "gui-channel.h"
#include "gui-nicklist-view.h"
#include "gui-window.h"
#include "gui-window-view.h"

#include "move.xpm"

static GdkPixbuf *move_pixbuf;

static gboolean event_destroy(GtkWidget *window, Tab *tab)
{
	signal_emit("gui tab destroyed", 1, tab);

	tab->destroying = TRUE;

	/* destroy panes first */
	while (tab->panes != NULL) {
		TabPane *pane = tab->panes->data;
		gtk_widget_destroy(pane->widget);
	}

	/* make sure frame->active_tab isn't invalid even for the
	   short time it takes for frame to notice the tab change. */
	if (tab->frame->active_tab == tab)
		tab->frame->active_tab = NULL;

	g_object_set_data(G_OBJECT(tab->widget), "Tab", NULL);
	g_free(tab);
	return FALSE;
}

static gboolean event_notify(GtkWidget *widget, GParamSpec *spec, Tab *tab)
{
	Frame *frame;

	if (strcmp(spec->name, "parent") == 0) {
		frame = gui_widget_find_data(widget, "Frame");
		if (frame != NULL)
			tab->frame = frame;
	}
	return FALSE;
}

static GtkWidget *gui_tab_label_new(GtkWidget *label, Tab *tab)
{
	GtkWidget *eventbox;

	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), label);
	gtk_widget_show_all(eventbox);

        gui_tab_move_label_init(eventbox);
	return eventbox;
}

Tab *gui_tab_new(Frame *frame)
{
	GtkWidget *vbox, *hpane, *vpane, *label;
	Tab *tab;

	tab = g_new0(Tab, 1);
	tab->frame = frame;

	/* create the window */
	tab->widget = vbox = gtk_vbox_new(FALSE, 0);
	g_object_set_data(G_OBJECT(tab->widget), "Tab", tab);

	g_signal_connect(G_OBJECT(vbox), "destroy",
			 G_CALLBACK(event_destroy), tab);
	g_signal_connect(G_OBJECT(vbox), "notify",
			 G_CALLBACK(event_notify), tab);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	hpane = gtk_hpaned_new();
	gtk_box_pack_start(GTK_BOX(vbox), hpane, TRUE, TRUE, 0);

	/* tab's label */
	label = gtk_label_new(NULL);
	tab->label = GTK_LABEL(label);

	tab->tab_label = gui_tab_label_new(label, tab);
	g_object_set_data(G_OBJECT(tab->tab_label), "Tab", tab);

	/* nicklist */
	tab->nicklist = gui_nicklist_view_new(tab);
	gtk_paned_pack2(GTK_PANED(hpane), tab->nicklist->widget, FALSE, TRUE);

	/* vertical pane */
	vpane = gtk_vpaned_new();
	gtk_paned_pack1(GTK_PANED(hpane), vpane, TRUE, TRUE);
	tab->first_paned = tab->last_paned = GTK_PANED(vpane);

	gtk_widget_show_all(vbox);
	gtk_widget_hide(tab->nicklist->widget);

	gtk_notebook_append_page(frame->notebook, tab->widget, tab->tab_label);

	signal_emit("gui tab created", 1, tab);
	return tab;
}

static void event_pane_close(GtkWidget *widget, TabPane *pane)
{
	if (windows->next != NULL)
		gtk_widget_destroy(pane->widget);
}

static TabPane *tab_find_pane(Tab *tab, GtkWidget *paned)
{
	GList *tmp;

	/* find the pane */
	for (tmp = tab->panes; tmp != NULL; tmp = tmp->next) {
		TabPane *pane = tmp->data;

		if (pane->widget->parent == paned)
			return pane;
	}

	return NULL;
}

static gboolean event_pane_moved(GtkPaned *paned, GdkEventButton *event,
				 Tab *tab)
{
	TabPane *pane;
	int pos;

	pane = tab_find_pane(tab, GTK_WIDGET(paned));
	pos = gtk_paned_get_position(paned);
	if (pane == tab->panes->data) {
		/* moving the uppest pane - check if we want to
		   create new split window */
		if (pos <= 1)
			return FALSE;

		if (pos <= 20) {
			gtk_paned_set_position(paned, 0);
			return FALSE;
		}


		signal_emit("command window new", 2, "split",
			    tab->active_win == NULL ? NULL :
			    tab->active_win->active_server);
	} else if (pos <= 20 && tab->panes->next != NULL &&
		   pane == tab->panes->next->data) {
		/* shrinked the second uppest pane - destroy the uppest pane */
		pane = tab->panes->data;
		gtk_widget_destroy(pane->widget);
	}

	return FALSE;
}

static GtkWidget *gui_close_button_new(TabPane *pane)
{
	GtkWidget *button, *image;

	button = gtk_button_new();
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_pane_close), pane);

	image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_size_request(image, 13, 13);
	gtk_container_add(GTK_CONTAINER(button), image);

	return button;
}

static GtkWidget *gui_move_button_new(Tab *tab, TabPane *pane)
{
	GtkWidget *button, *image;

	button = gtk_button_new();
	gui_tab_move_label_init(button);

	image = gtk_image_new_from_pixbuf(move_pixbuf);
	gtk_container_add(GTK_CONTAINER(button), image);

	return button;
}

static gboolean event_destroy_pane(GtkWidget *widget, TabPane *pane)
{
	Tab *tab = pane->tab;

	tab->panes = g_list_remove(tab->panes, pane);

	if (pane->view != NULL)
		gtk_widget_destroy(pane->view->widget);

	g_object_set_data(G_OBJECT(pane->widget), "TabPane", NULL);
	g_free(pane);

	if (tab->panes == NULL) {
		/* last pane, kill the tab */
                gtk_widget_destroy(tab->widget);
	} else {
		gui_tab_pack_panes(tab);
		gui_tab_update_active_window(tab);
	}
	return FALSE;
}

static gboolean event_notify_pane(GtkWidget *widget, GParamSpec *spec,
				  TabPane *pane)
{
	Tab *tab;

	if (strcmp(spec->name, "parent") == 0) {
		tab = gui_widget_find_data(widget, "Tab");
		if (tab != NULL)
			pane->tab = tab;
	}
	return FALSE;
}

GtkPaned *gui_tab_add_paned(Tab *tab)
{
	GtkWidget *new_paned;
	GtkPaned *paned, *parent_paned;

	paned = tab->last_paned;

	/* add new empty pane */
	new_paned = gtk_vpaned_new();
	gtk_paned_add1(paned, new_paned);
	gtk_widget_show(new_paned);

	parent_paned = GTK_PANED(GTK_WIDGET(paned)->parent);
	if (paned != tab->first_paned &&
	    gtk_paned_get_position(parent_paned) < 100)
		gtk_paned_set_position(parent_paned, 100);
	g_signal_connect(G_OBJECT(paned), "button_release_event",
			 G_CALLBACK(event_pane_moved), tab);
	tab->last_paned = GTK_PANED(new_paned);
	return paned;
}

TabPane *gui_tab_pane_new(Tab *tab)
{
	GtkWidget *vbox, *hbox, *space, *button;
	GtkPaned *paned;
	TabPane *pane;

	pane = g_new0(TabPane, 1);
	pane->tab = tab;
        paned = gui_tab_add_paned(tab);

	pane->widget = vbox = gtk_vbox_new(FALSE, 5);
	pane->box = GTK_BOX(vbox);
	gtk_paned_add2(paned, vbox);

	g_signal_connect(G_OBJECT(vbox), "destroy",
			 G_CALLBACK(event_destroy_pane), pane);
	g_signal_connect(G_OBJECT(vbox), "notify",
			 G_CALLBACK(event_notify_pane), pane);
	g_object_set_data(G_OBJECT(vbox), "TabPane", pane);

	hbox = gtk_hbox_new(FALSE, 0);
	pane->titlebox = GTK_BOX(hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* close button */
	button = gui_close_button_new(pane);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

	space = gtk_label_new(NULL);
	gtk_widget_set_size_request(space, 5, -1);
	gtk_box_pack_start(GTK_BOX(hbox), space, FALSE, FALSE, 0);

	/* move button */
	button = gui_move_button_new(tab, pane);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

	space = gtk_label_new(NULL);
	gtk_widget_set_size_request(space, 5, -1);
	gtk_box_pack_start(GTK_BOX(hbox), space, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);

	tab->panes = g_list_prepend(tab->panes, pane);
	return pane;
}

static void paned_reparent_child(GtkPaned *dest, GtkWidget *child)
{
	gtk_widget_ref(child);
	gtk_widget_hide(child);

	gtk_container_remove(GTK_CONTAINER(child->parent), child);
	gtk_paned_add2(dest, child);

	gtk_widget_show(child);
	gtk_widget_unref(child);
}

void gui_tab_pack_panes(Tab *tab)
{
	GtkPaned *empty_paned, *paned;

	if (tab->destroying || tab->panes == NULL)
		return;

	/* find the first paned with empty child */
        empty_paned = tab->first_paned;
	while (empty_paned->child1 != NULL && empty_paned->child2 != NULL)
		empty_paned = GTK_PANED(empty_paned->child1);

	if (empty_paned->child1 == NULL)
		return;

	/* find the next paned with a child */
        paned = empty_paned;
	while (paned->child1 != NULL && paned->child2 == NULL)
		paned = GTK_PANED(paned->child1);

	/* now move the childs to highest empty panes */
	while (paned->child1 != NULL) {
		if (paned->child2 != NULL) {
			paned_reparent_child(empty_paned, paned->child2);
			empty_paned = GTK_PANED(empty_paned->child1);
		}
		paned = GTK_PANED(paned->child1);
	}

	/* now destroy the extra panes */
	tab->last_paned = empty_paned;
	g_signal_handlers_disconnect_by_func(tab->last_paned,
					     G_CALLBACK(event_pane_moved), tab);
	gtk_widget_destroy(tab->last_paned->child1);

	/* set the paned position at top to 0 */
	gtk_paned_set_position(GTK_PANED(GTK_WIDGET(tab->last_paned)->parent),
			       0);
}

Tab *gui_tab_get_page(Frame *frame, int page)
{
	GtkWidget *child;

	child = gtk_notebook_get_nth_page(frame->notebook, page);
	return child == NULL ? NULL :
		g_object_get_data(G_OBJECT(child), "Tab");
}

void gui_tab_set_active(Tab *tab)
{
	int page;

	if (tab->frame->destroying)
		return;

	page = gtk_notebook_page_num(tab->frame->notebook, tab->widget);
	gtk_notebook_set_current_page(tab->frame->notebook, page);
}

void gui_tab_set_active_window(Tab *tab, Window *window)
{
	if (tab->active_win != window) {
		tab->active_win = window;
		gui_tab_set_active_window_item(tab, window);
	}

	gui_frame_set_active_window(tab->frame, window);
}

void gui_tab_set_active_window_item(Tab *tab, Window *window)
{
	WindowItem *witem;
	ChannelGui *gui;

	if (tab->destroying)
		return;

	witem = window == NULL ? NULL : window->active;
	if (witem == NULL) {
		/* empty window */
		gtk_label_set_text(tab->label,
				   window != NULL && window->name != NULL ?
				   window->name : "(empty)");
	} else {
		gtk_label_set_text(tab->label, witem->name);
	}

	if (!IS_CHANNEL(witem)) {
		/* clear nicklist */
		gtk_widget_hide(tab->nicklist->widget);
		gui_nicklist_view_set(tab->nicklist, NULL);
	} else {
                gui = CHANNEL_GUI(CHANNEL(witem));
		gui_nicklist_view_set(tab->nicklist, gui->nicklist);
		gtk_widget_show(tab->nicklist->widget);
	}
}

void gui_tab_update_active_window(Tab *tab)
{
	Window *window;
	GList *tmp;

	window = NULL;
	for (tmp = tab->panes; tmp != NULL; tmp = tmp->next) {
		TabPane *pane = tmp->data;

		if (pane->view == NULL)
			continue;

		if (pane->view->window->window == tab->active_win) {
			window = pane->view->window->window;
			break;
		}

		if (window == NULL)
			window = pane->view->window->window;
	}

        gui_tab_set_active_window(tab, window);
}

void gui_tabs_init(void)
{
	move_pixbuf = gdk_pixbuf_new_from_xpm_data((const char **) move_xpm);
}

void gui_tabs_deinit(void)
{
	gdk_pixbuf_unref(move_pixbuf);
}
