/*
 gui-window-view.c : irssi

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
#include "modules.h"
#include "signals.h"

#include "printtext.h"
#include "window-items.h"

#include "gui-frame.h"
#include "gui-tab.h"
#include "gui-window.h"
#include "gui-window-view.h"
#include "gui-window-context.h"

static void view_set_size(WindowView *view, GtkAllocation *alloc)
{
	view->approx_width = alloc->width / view->font_width;
	view->approx_height = alloc->height / view->font_height;
	gui_window_update_width(view->window);
}

static gboolean event_destroy(GtkWidget *widget, WindowView *view)
{
	signal_emit("gui window view destroyed", 1, view);

	g_signal_handler_disconnect(G_OBJECT(view->window->buffer),
				    view->sig_changed);
	gui_window_remove_view(view);
	gtk_widget_destroy(view->title);

	g_free(view);
	return FALSE;
}

static gboolean event_button_press(GtkWidget *widget, GdkEventButton *event,
				   WindowView *view)
{
	/* disable internal popup menu */
	return event->button == 3;
}

static gboolean event_button_release(GtkWidget *widget, GdkEventButton *event,
				     WindowView *view)
{
	view->window->active_view = view;
	window_set_active(view->window->window);
	return FALSE;
}

static gboolean event_resize(GtkWidget *widget, GtkAllocation *alloc,
			     WindowView *view)
{
	view_set_size(view, alloc);
	if (view->bottom) {
		/* scroll position goes up when window is shrinked,
		   make it go back down */
		gtk_text_view_scroll_mark_onscreen(view->view,
						   gtk_text_buffer_get_insert(view->window->buffer));
	}
	return FALSE;
}

static gboolean event_changed(GtkWidget *widget, WindowView *view)
{
	/* new text added - save bottom status */
	view->bottom = view->adj->value ==
		view->adj->upper - view->adj->page_size;
	return FALSE;
}

static void get_font_size(GtkWidget *widget, PangoFontDescription *font_desc,
			  int *width, int *height)
{
      PangoContext *context;
      PangoFontMetrics *metrics;

      context = gtk_widget_get_pango_context(widget);
      metrics = pango_context_get_metrics(context, font_desc,
					  pango_context_get_language(context));
      *width = PANGO_PIXELS(pango_font_metrics_get_approximate_digit_width(metrics));
      *height = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics) +
			     pango_font_metrics_get_descent(metrics));

}

WindowView *gui_window_view_new(TabPane *pane, WindowGui *window,
				GtkTextBuffer *buffer)
{
        WindowView *view;
	GtkWidget *sw, *text_view;
	/*PangoFontDescription *font_desc;*/
	GdkColor color;

	view = g_new0(WindowView, 1);
	view->window = window;
	view->pane = pane;

	view->sig_changed = g_signal_connect(G_OBJECT(buffer), "changed",
					     G_CALLBACK(event_changed), view);

	/* scrolled window where to place text view */
	view->widget = sw = gtk_scrolled_window_new(NULL, NULL);
	g_signal_connect(G_OBJECT(sw), "destroy",
			 G_CALLBACK(event_destroy), view);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	view->adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(sw));

	text_view = gtk_text_view_new_with_buffer(buffer);
	view->view = GTK_TEXT_VIEW(text_view);
	g_signal_connect(G_OBJECT(text_view), "button_press_event",
			 G_CALLBACK(event_button_press), view);
	g_signal_connect(G_OBJECT(text_view), "button_release_event",
			 G_CALLBACK(event_button_release), view);
	g_signal_connect_after(G_OBJECT(text_view), "size_allocate",
			       G_CALLBACK(event_resize), view);
	g_signal_connect(G_OBJECT(text_view), "motion_notify_event",
			 G_CALLBACK(gui_window_context_event_motion), view);
	gtk_container_add(GTK_CONTAINER(sw), text_view);

	/* FIXME: configurable */
	/*font_desc = pango_font_description_from_string("fixed 10");
	gtk_widget_modify_font(text_view, font_desc);
	pango_font_description_free(font_desc);*/

	gtk_text_view_set_cursor_visible(view->view, FALSE);
	gtk_text_view_set_wrap_mode(view->view, GTK_WRAP_WORD);
        gtk_text_view_set_editable(view->view, FALSE);
	gtk_text_view_set_left_margin(view->view, 2);
	gtk_text_view_set_right_margin(view->view, 2);
	gtk_text_view_set_indent(view->view, -50);

	/* FIXME: configurable */
	gdk_color_parse("black", &color);
	gtk_widget_modify_base(text_view, GTK_STATE_NORMAL, &color);
	gdk_color_parse("grey", &color);
	gtk_widget_modify_text(text_view, GTK_STATE_NORMAL, &color);

	gtk_widget_show_all(view->widget);

	/* update pane */
	pane->view = view;
	gtk_box_pack_start(pane->box, view->widget, TRUE, TRUE, 0);

	get_font_size(text_view, window->font_monospace,
		      &view->font_width, &view->font_height);

	gui_window_view_set_title(view);

	signal_emit("gui window view created", 1, view);
	return view;
}

static void event_title_destroy(GtkWidget *widget, WindowView *view)
{
	view->title = NULL;
}

void gui_window_view_set_title(WindowView *view)
{
	Window *window;
	GtkWidget *title;

	if (view->pane->titlebox == NULL)
		return;

	window = view->window->window;
	if (window->active == NULL) {
		/* empty window */
		title = gtk_label_new(window->name);
	} else {
		/* window item */
		WindowItemGui *witem_gui;

		witem_gui = MODULE_DATA(window->active);
		title = witem_gui != NULL && witem_gui->get_title != NULL ?
			witem_gui->get_title(window->active) : NULL;
		if (title == NULL)
			title = gtk_label_new(window->active->name);
	}

	if (view->title != NULL)
		gtk_widget_destroy(view->title);
	view->title = title;

	gtk_widget_show(title);
	g_signal_connect(G_OBJECT(title), "destroy",
			 G_CALLBACK(event_title_destroy), view);

	gtk_box_pack_start(view->pane->titlebox, title, TRUE, TRUE, 0);
}

static void gui_window_views_set_title(Window *window)
{
	g_slist_foreach(WINDOW_GUI(window)->views,
			(GFunc) gui_window_view_set_title, NULL);
}

static void sig_window_name_changed(Window *window)
{
	if (window->items == NULL)
		gui_window_views_set_title(window);
}

static void sig_window_item_name_changed(WindowItem *item)
{
	gui_window_views_set_title(window_item_window(item));
}

void gui_window_views_init(void)
{
	signal_add("window name changed", (SIGNAL_FUNC) sig_window_name_changed);
	signal_add("window item name changed", (SIGNAL_FUNC) sig_window_item_name_changed);
}

void gui_window_views_deinit(void)
{
	signal_remove("window name changed", (SIGNAL_FUNC) sig_window_name_changed);
	signal_remove("window item name changed", (SIGNAL_FUNC) sig_window_item_name_changed);
}
