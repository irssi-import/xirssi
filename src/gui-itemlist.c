/*
 gui-itemlist.c : irssi

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
#include "servers.h"

#include "window-items.h"

#include "gui-frame.h"
#include "gui-tab.h"
#include "gui-window.h"
#include "gui-window-view.h"
#include "gui-itemlist.h"

static gboolean event_destroy(GtkWidget *widget, Itemlist *itemlist)
{
	gtk_widget_unref(itemlist->short_menu);
	gtk_widget_unref(itemlist->long_menu);
	g_free(itemlist);
	return FALSE;
}

static gboolean event_button_press(GtkWidget *widget, GdkEventButton *event,
				   Itemlist *itemlist)
{
	if (event->type != GDK_BUTTON_PRESS || event->button != 1)
		return FALSE;
	gtk_option_menu_set_menu(itemlist->menu, itemlist->long_menu);
	return FALSE;
}

static gboolean event_selection_done(GtkWidget *widget, Itemlist *itemlist)
{
	Window *window;
	WindowItem *witem;
	Server *server;
	int index;

	index = gtk_option_menu_get_history(itemlist->menu);
	gtk_option_menu_set_menu(itemlist->menu, itemlist->short_menu);
	gtk_option_menu_set_history(itemlist->menu, index);

	if (itemlist->window_items) {
		window = g_slist_nth_data(itemlist->items, index*2);
		if (g_slist_find(windows, window) == NULL)
			return FALSE;

		witem = g_slist_nth_data(itemlist->items, index*2 + 1);
		if (g_slist_find(window->items, witem) == NULL)
			return FALSE;

		window_set_active(window);
		window_item_set_active(window, witem);
	} else {
		window = itemlist->frame->active_tab->active_win;

		server = g_slist_nth_data(itemlist->items, index);
		if (g_slist_find(servers, server) != NULL)
			window_change_server(window, server);
	}

	return FALSE;
}

Itemlist *gui_itemlist_new(Frame *frame)
{
	Itemlist *itemlist;

	itemlist = g_new0(Itemlist, 1);
	itemlist->frame = frame;

	itemlist->widget = gtk_option_menu_new();
	itemlist->menu = GTK_OPTION_MENU(itemlist->widget);
	g_signal_connect(G_OBJECT(itemlist->menu), "destroy",
			 G_CALLBACK(event_destroy), itemlist);
	g_signal_connect(G_OBJECT(itemlist->menu), "button_press_event",
			 G_CALLBACK(event_button_press), itemlist);

	return itemlist;
}

static char *witem_get_name(WindowItem *witem, int long_view)
{
	return witem->server == NULL || !long_view ? g_strdup(witem->name) :
		g_strdup_printf("%s (%s)", witem->name, witem->server->tag);
}

static char *server_get_name(Server *server, int long_view)
{
	return !long_view ? g_strdup(server->tag) :
		g_strdup_printf("%s (%s:%d)", server->tag,
				server->connrec->address,
				server->connrec->port);
}

static void add_window_items(Window *window, Itemlist *itemlist)
{
	GtkMenuShell *short_menu, *long_menu;
	GtkWidget *item;
	GSList *tmp;
	char *str;

	short_menu = GTK_MENU_SHELL(itemlist->short_menu);
	long_menu = GTK_MENU_SHELL(itemlist->long_menu);

	for (tmp = window->items; tmp != NULL; tmp = tmp->next) {
		WindowItem *witem = tmp->data;

		/* short */
		str = witem_get_name(witem, FALSE);
		item = gtk_menu_item_new_with_label(str);
		gtk_menu_shell_append(short_menu, item);
		g_free(str);

		/* long */
		str = witem_get_name(witem, TRUE);
		item = gtk_menu_item_new_with_label(str);
		gtk_menu_shell_append(long_menu, item);
		g_free(str);

		itemlist->items = g_slist_append(itemlist->items, window);
		itemlist->items = g_slist_append(itemlist->items, witem);
	}
}

static void add_server_items(Itemlist *itemlist)
{
	GtkMenuShell *short_menu, *long_menu;
	GtkWidget *item;
	GSList *tmp;
	char *str;

	short_menu = GTK_MENU_SHELL(itemlist->short_menu);
	long_menu = GTK_MENU_SHELL(itemlist->long_menu);

	for (tmp = servers; tmp != NULL; tmp = tmp->next) {
		Server *server = tmp->data;

		/* short */
		str = server_get_name(server, FALSE);
		item = gtk_menu_item_new_with_label(str);
		gtk_menu_shell_append(short_menu, item);
		g_free(str);

		/* long */
		str = server_get_name(server, TRUE);
		item = gtk_menu_item_new_with_label(str);
		gtk_menu_shell_append(long_menu, item);
		g_free(str);

		itemlist->items = g_slist_append(itemlist->items, server);
	}
}

static void gui_itemlist_update_items(Itemlist *itemlist, Window *window)
{
	GList *tmp;

	if (window->items == NULL && servers == NULL) {
		/* nothing - hide the itemlist */
		gtk_widget_hide(itemlist->widget);
		return;
	}

	if (itemlist->short_menu != NULL) {
		gtk_widget_unref(itemlist->short_menu);
		gtk_widget_unref(itemlist->long_menu);
	}

	itemlist->short_menu = gtk_menu_new();
	itemlist->long_menu = gtk_menu_new();
	gtk_widget_ref(itemlist->short_menu);
	gtk_widget_ref(itemlist->long_menu);

	g_signal_connect(G_OBJECT(itemlist->long_menu), "selection_done",
			 G_CALLBACK(event_selection_done), itemlist);

	g_slist_free(itemlist->items);
	itemlist->items = NULL;

	if (window->items != NULL) {
		/* update items list from window items */
		itemlist->window_items = TRUE;

		/* add items in active window */
		add_window_items(window, itemlist);

		/* add items in other views in active tab */
                tmp = itemlist->frame->active_tab->panes;
		for (; tmp != NULL; tmp = tmp->next) {
			TabPane *pane = tmp->data;

			if (pane->view != NULL &&
			    pane->view->window->window != window) {
				add_window_items(pane->view->window->window,
						 itemlist);
			}
		}
	} else {
		/* no window items - servers then */
		itemlist->window_items = FALSE;
		add_server_items(itemlist);
	}

	gtk_widget_show_all(itemlist->short_menu);
	gtk_widget_show_all(itemlist->long_menu);

	gtk_option_menu_set_menu(itemlist->menu, itemlist->short_menu);

	if (itemlist->items->next == NULL ||
	    (window->items && itemlist->items->next->next == NULL)) {
		/* only one item, hide */
		gtk_widget_hide(itemlist->widget);
	} else {
		gtk_widget_show(itemlist->widget);
	}
}

static void gui_itemlist_set_active(Itemlist *itemlist, Window *window)
{
	int index;

	index = -1;
	if (itemlist->window_items) {
		if (window->active != NULL) {
			index = g_slist_index(itemlist->items, window->active);
			if (index != -1)
				index /= 2;
		}
	} else {
		if (window->active_server != NULL) {
			index = g_slist_index(itemlist->items,
					      window->active_server);
		}
	}

	if (index != -1)
                gtk_option_menu_set_history(itemlist->menu, index);
}

static void gui_itemlist_update_window(Window *window, int update_items)
{
	GSList *tmp;

	for (tmp = WINDOW_GUI(window)->views; tmp != NULL; tmp = tmp->next) {
		WindowView *view = tmp->data;
		Tab *tab = view->pane->tab;

		if (tab->frame->active_tab != tab)
			continue;

		/* update selection */
		if (update_items)
			gui_itemlist_update_items(tab->frame->itemlist, window);
		gui_itemlist_set_active(tab->frame->itemlist, window);
	}
}

static void sig_window_changed(Window *window)
{
	gui_itemlist_update_window(window, TRUE);
}

static void sig_window_item_changed(Window *window)
{
	/* window item/server changed */
	gui_itemlist_update_window(window, FALSE);
}

static void sig_server_connected(Server *server)
{
	GSList *tmp;

	for (tmp = windows; tmp != NULL; tmp = tmp->next) {
		Window *window = tmp->data;

		if (window->active_server == server)
			sig_window_changed(window);
	}
}

void gui_itemlists_init(void)
{
	signal_add("window changed", (SIGNAL_FUNC) sig_window_changed);
	signal_add("window item new", (SIGNAL_FUNC) sig_window_changed);
	signal_add("window item remove", (SIGNAL_FUNC) sig_window_changed);
	signal_add("window item name changed", (SIGNAL_FUNC) sig_window_changed);
	signal_add("window item changed", (SIGNAL_FUNC) sig_window_item_changed);
	signal_add("window server changed", (SIGNAL_FUNC) sig_window_item_changed);
	signal_add("server connected", (SIGNAL_FUNC) sig_server_connected);
}

void gui_itemlists_deinit(void)
{
	signal_remove("window changed", (SIGNAL_FUNC) sig_window_changed);
	signal_remove("window item new", (SIGNAL_FUNC) sig_window_changed);
	signal_remove("window item remove", (SIGNAL_FUNC) sig_window_changed);
	signal_remove("window item name changed", (SIGNAL_FUNC) sig_window_changed);
	signal_remove("window item changed", (SIGNAL_FUNC) sig_window_item_changed);
	signal_remove("window server changed", (SIGNAL_FUNC) sig_window_item_changed);
	signal_remove("server connected", (SIGNAL_FUNC) sig_server_connected);
}
