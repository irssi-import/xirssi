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
#include "gui-tab-label.h"
#include "gui-window.h"
#include "gui-channel.h"
#include "gui-nicklist-view.h"

static gboolean event_destroy(GtkWidget *window, Tab *tab)
{
	signal_emit("gui tab destroyed", 1, tab);

	if (tab->frame->active_tab == tab)
		tab->frame->active_tab = NULL;

	tab->destroying = TRUE;
	tab->widget = NULL;
	tab->tab_label = NULL;
	tab->label = NULL;

	gui_tab_unref(tab);
	return FALSE;
}

Tab *gui_tab_new(Frame *frame)
{
	GtkWidget *vbox, *hpane, *vpane, *label;
	Tab *tab;

	tab = g_new0(Tab, 1);
	tab->refcount = 1;

	tab->frame = frame;
	gui_frame_ref(frame);

	/* create the window */
	tab->widget = vbox = gtk_vbox_new(FALSE, 0);
	g_signal_connect(G_OBJECT(vbox), "destroy",
			 G_CALLBACK(event_destroy), tab);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	hpane = gtk_hpaned_new();
	gtk_box_pack_start(GTK_BOX(vbox), hpane, TRUE, TRUE, 0);

	/* tab's label */
	label = gtk_label_new(NULL);
	tab->label = GTK_LABEL(label);
	tab->tab_label = gui_tab_label_new(tab, label);

	/* nicklist */
	tab->nicklist = gui_nicklist_view_new(tab);
	gui_nicklist_view_ref(tab->nicklist);
	gtk_paned_pack2(GTK_PANED(hpane), tab->nicklist->widget, FALSE, TRUE);

	/* vertical pane */
	vpane = gtk_vpaned_new();
	gtk_paned_pack1(GTK_PANED(hpane), vpane, TRUE, TRUE);
	tab->panes = g_list_prepend(tab->panes, vpane);

	gtk_widget_show_all(vbox);
	gtk_widget_hide(tab->nicklist->widget);

	signal_emit("gui tab created", 1, tab);
	return tab;
}

void gui_tab_ref(Tab *tab)
{
	tab->refcount++;
}

void gui_tab_unref(Tab *tab)
{
	if (--tab->refcount > 0)
		return;

	gui_nicklist_view_unref(tab->nicklist);
	gui_frame_unref(tab->frame);
	g_free(tab);
}

typedef struct {
	Tab *tab;

	GtkWidget *widget;
	GtkWidget *box;
	GtkPaned *paned;

	GtkWidget *titlebox, *title;
} PanedWidget;

static void sig_pane_close(GtkWidget *widget, GtkWidget *paned)
{
	PanedWidget *pw;

	pw = g_object_get_data(G_OBJECT(paned), "irssi paned widget");
	if (windows->next != NULL)
		gui_tab_remove_widget(pw->tab, pw->widget);
}

static void panes_swap(PanedWidget *pw1, PanedWidget *pw2)
{
	GtkWidget *widget, *tempbox;

	if (pw1 == NULL || pw2 == NULL)
		return;

	tempbox = gtk_event_box_new();

	widget = pw1->widget;
	gtk_widget_reparent(widget, tempbox);

	gtk_widget_reparent(pw2->widget, pw1->box);
	pw1->widget = pw2->widget;
	g_object_set_data(G_OBJECT(pw1->widget), "irssi pane", pw1->paned);

	gtk_widget_reparent(widget, pw2->box);
	pw2->widget = widget;
	g_object_set_data(G_OBJECT(pw2->widget), "irssi pane", pw2->paned);

	gtk_widget_destroy(tempbox);
}

static void sig_pane_move_down(GtkWidget *widget, GtkWidget *paned)
{
	PanedWidget *pw1, *pw2;

	pw1 = g_object_get_data(G_OBJECT(paned), "irssi paned widget");
	pw2 = g_object_get_data(G_OBJECT(paned->parent), "irssi paned widget");

        panes_swap(pw1, pw2);
}

static void sig_pane_move_up(GtkWidget *widget, GtkPaned *paned)
{
	PanedWidget *pw1, *pw2;

	pw1 = g_object_get_data(G_OBJECT(paned), "irssi paned widget");
	pw2 = g_object_get_data(G_OBJECT(paned->child1), "irssi paned widget");

        panes_swap(pw1, pw2);
}

static GtkWidget *
stock_button_create(const char *stock, GCallback callback, void *data)
{
	GtkWidget *button, *pix;

	pix = gtk_image_new_from_stock(stock, GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_size_request(pix, 10, 10);

	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), pix);

	if (callback != NULL) {
		g_signal_connect(G_OBJECT(button), "clicked",
				 callback, data);
	}

	return button;
}

static void paned_set_move_buttons(GtkWidget *paned, int show)
{
	GtkWidget *upbutton, *downbutton;

	upbutton = g_object_get_data(G_OBJECT(paned), "irssi but move up");
	downbutton = g_object_get_data(G_OBJECT(paned), "irssi but move down");

	if (show) {
		gtk_widget_show(upbutton);
		gtk_widget_show(downbutton);
	} else {
		gtk_widget_hide(upbutton);
		gtk_widget_hide(downbutton);
	}

	/*gtk_widget_set_sensitive(upbutton, show);
        gtk_widget_set_sensitive(downbutton, show);*/
}

static gboolean event_pane_moved(GtkPaned *paned, GdkEventButton *event,
				 Tab *tab)
{
	PanedWidget *pw;
	int pos;

	pos = gtk_paned_get_position(paned);
	if (paned == tab->panes->next->data) {
		/* create a new pane? */
		if (pos <= 1)
			return FALSE;

		if (pos <= 20) {
			gtk_paned_set_position(paned, 0);
			return FALSE;
		}

		signal_emit("command window new", 3, "split",
			    tab->active_win->active_server,
			    tab->active_win->active);
	} else if (pos <= 20 && paned == tab->panes->next->next->data) {
		/* destroy the pane */
		pw = g_object_get_data(G_OBJECT(paned->child1), "irssi paned widget");
		gui_tab_remove_widget(tab, pw->widget);
	}

	return FALSE;
}

GtkBox *gui_tab_add_widget(Tab *tab, GtkWidget *widget)
{
	PanedWidget *pw;
	GtkPaned *paned;
	GtkWidget *new_paned, *vbox, *hbox, *space, *button;

	pw = g_new0(PanedWidget, 1);
	pw->widget = widget;
	pw->tab = tab;

	pw->paned = paned = tab->panes->data;
        g_object_set_data(G_OBJECT(paned), "irssi paned widget", pw);
        g_object_set_data(G_OBJECT(widget), "irssi pane", paned);

	pw->box = vbox = gtk_vbox_new(FALSE, 5);
	gtk_paned_add2(paned, vbox);

	pw->titlebox = hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* close and move/down buttons */
	button = stock_button_create(GTK_STOCK_CLOSE,
				     GTK_SIGNAL_FUNC(sig_pane_close), paned);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

	space = gtk_label_new(NULL);
	gtk_widget_set_size_request(space, 5, -1);
	gtk_box_pack_start(GTK_BOX(hbox), space, FALSE, FALSE, 0);

	button = stock_button_create(GTK_STOCK_GO_UP, GTK_SIGNAL_FUNC(sig_pane_move_up), paned);
	g_object_set_data(G_OBJECT(paned), "irssi but move up", button);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

	button = stock_button_create(GTK_STOCK_GO_DOWN, GTK_SIGNAL_FUNC(sig_pane_move_down), paned);
	g_object_set_data(G_OBJECT(paned), "irssi but move down", button);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

	space = gtk_label_new(NULL);
	gtk_widget_set_size_request(space, 5, -1);
	gtk_box_pack_start(GTK_BOX(hbox), space, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);

	if (tab->panes->next == NULL) {
		/* first pane in tab, don't show up/down buttons */
		paned_set_move_buttons(GTK_WIDGET(paned), FALSE);
	} else if (tab->panes->next->next == NULL) {
		/* second pane in tab, show the old
		   window's up/down buttons */
		paned_set_move_buttons(tab->panes->next->data, TRUE);
	}

	/* add new empty pane */
	new_paned = gtk_vpaned_new();
	gtk_paned_add1(paned, new_paned);
	gtk_widget_show(new_paned);

	g_signal_connect(G_OBJECT(paned), "button_release_event",
			 G_CALLBACK(event_pane_moved), tab);

	tab->panes = g_list_prepend(tab->panes, new_paned);

	return GTK_BOX(hbox);
}

void gui_tab_remove_widget(Tab *tab, GtkWidget *widget)
{
	PanedWidget *pw1, *pw2;
	GtkPaned *paned;

	paned = g_object_get_data(G_OBJECT(widget), "irssi pane");
	for (;;) {
		pw1 = g_object_get_data(G_OBJECT(paned), "irssi paned widget");
		pw2 = g_object_get_data(G_OBJECT(paned->child1), "irssi paned widget");

		if (pw2 == NULL) {
			g_assert(pw1->widget == widget);

			gtk_widget_destroy(pw1->box);
			g_free(pw1);

			g_object_set_data(G_OBJECT(paned), "irssi paned widget", NULL);
			gtk_paned_set_position(GTK_PANED(GTK_WIDGET(paned)->parent), 0);

			paned = GTK_PANED(paned->child1);
			tab->panes = g_list_remove(tab->panes, paned);
			gtk_widget_destroy(GTK_WIDGET(paned));
			break;
		}

		panes_swap(pw1, pw2);
		paned = GTK_PANED(paned->child1);
	}

	if (g_list_length(tab->panes) == 2) {
		/* only one pane left, hide move up/down buttons */
		paned_set_move_buttons(tab->panes->next->data, FALSE);
	} else if (tab->panes->next == NULL) {
		/* last pane, kill the tab */
                gtk_widget_destroy(tab->widget);
	}
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

	witem = window->active;
	if (witem == NULL) {
		/* empty window */
		gtk_label_set_text(tab->label, window->name != NULL ?
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
