/*
 gui-window-activity.c : irssi

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
#include "gui-window.h"
#include "gui-window-view.h"

/* Don't send window activity if window is already visible in
   another split window or frame */
static void sig_activity(Window *window)
{
	GSList *tmp;

	if (!gui_window_is_visible(window) || window->data_level == 0)
		return;

	window->data_level = 0;
        window->hilight_color = 0;

	for (tmp = window->items; tmp != NULL; tmp = tmp->next) {
		WI_ITEM_REC *item = tmp->data;

		item->data_level = 0;
		item->hilight_color = 0;
	}

	signal_stop();
}

static void tab_update_activity(Tab *tab)
{
	GtkWidget *label;
	GdkColor color;
	GSList *tmp;
	int data_level;

	if (tab->frame->notebook == NULL) {
		/* destroying the frame */
		return;
	}

	/* get highest data level */
	data_level = 0;
	for (tmp = tab->views; tmp != NULL; tmp = tmp->next) {
		WindowView *view = tmp->data;

		if (view->window->window->data_level > data_level)
			data_level = view->window->window->data_level;
	}

	/* get the color */
	switch (data_level) {
	case DATA_LEVEL_NONE:
		gdk_color_parse("black", &color);
		break;
	case DATA_LEVEL_TEXT:
		gdk_color_parse("dark red", &color);
		break;
	case DATA_LEVEL_MSG:
		gdk_color_parse("red", &color);
		break;
	default:
		gdk_color_parse("magenta", &color);
		break;
	}

	/* change the tab's color */
	label = gtk_notebook_get_tab_label(tab->frame->notebook, tab->widget);
	gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &color);
}

static void sig_activity_update(Window *window)
{
	GSList *tmp;

	for (tmp = WINDOW_GUI(window)->views; tmp != NULL; tmp = tmp->next) {
		WindowView *view = tmp->data;

		if (window->data_level == 0 ||
		    view->tab->data_level < window->data_level)
			tab_update_activity(view->tab);
	}
}

void gui_window_activities_init(void)
{
	signal_add_first("window hilight", (SIGNAL_FUNC) sig_activity);
	signal_add_first("window activity", (SIGNAL_FUNC) sig_activity);

	signal_add("window hilight", (SIGNAL_FUNC) sig_activity_update);
}

void gui_window_activities_deinit(void)
{
	signal_remove("window hilight", (SIGNAL_FUNC) sig_activity);
	signal_remove("window activity", (SIGNAL_FUNC) sig_activity);

	signal_remove("window hilight", (SIGNAL_FUNC) sig_activity_update);
}
