/*
 gui-tab-move.c : irssi

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

#include "arrow-up.xpm"
#include "arrow-down.xpm"
#include "arrow-left.xpm"
#include "arrow-right.xpm"
#include "arrow-up-twist.xpm"
#include "arrow-down-twist.xpm"
#include "arrow-left-twist.xpm"
#include "arrow-right-twist.xpm"
#include "detach-box.xpm"

#include "gui-frame.h"
#include "gui-tab.h"
#include "gui-window.h"
#include "gui-window-view.h"

#define MAGIC_PADDING 4

static GtkWidget *arrow_up = NULL, *arrow_down = NULL;
static GtkWidget *arrow_up_twist = NULL, *arrow_down_twist = NULL;

static GtkWidget *arrow_left = NULL, *arrow_right = NULL;
static GtkWidget *arrow_left_twist = NULL, *arrow_right_twist = NULL;

static GtkWidget *detach_box = NULL;

static void create_pixmap_popup(GtkWidget **widget, char **data)
{
	GtkWidget *win, *image;
	GdkPixbuf *pixbuf;
        GdkPixmap *pixmap;
	GdkBitmap *bitmap;

	if (*widget != NULL)
		return;

	pixbuf = gdk_pixbuf_new_from_xpm_data((const char **) data);
        gdk_pixbuf_render_pixmap_and_mask(pixbuf, &pixmap, &bitmap, 128);
        gdk_pixbuf_unref(pixbuf);

	win = gtk_window_new(GTK_WINDOW_POPUP);
	image = gtk_image_new_from_pixmap(pixmap, bitmap);
	gtk_container_add(GTK_CONTAINER(win), image);
	gtk_widget_shape_combine_mask(win, bitmap, 0, 0);

        gdk_pixmap_unref(pixmap);
	gdk_bitmap_unref(bitmap);

	gtk_widget_show_all(image);
	gtk_widget_realize(win);

	*widget = win;
}
/* Returns pos number*2, +1 if in center */
static int notebook_get_pos_at(GtkNotebook *notebook, int x, int y)
{
	GtkWidget *tab, *child;
	int horiz, pos, start, size, i, pagepos;

	horiz = notebook->tab_pos == GTK_POS_TOP ||
		notebook->tab_pos == GTK_POS_BOTTOM;
	pos = horiz ? x : y;

	pagepos = 0;
	for (i = 0; ; i++) {
		child = gtk_notebook_get_nth_page(notebook, i);
		if (child == NULL)
			break;

                tab = gtk_notebook_get_tab_label(notebook, child);
		start = horiz ? tab->allocation.x : tab->allocation.y;
		size = horiz ? tab->allocation.width : tab->allocation.height;

		if (start >= pos)
			break;

		if (pos < start+size/3) {
			/* cursor in 1/3 of tab, put it left */
			pagepos = i*2;
		} else if (pos < start+size*2/3) {
			/* cursor in 2/3 of tab, put it middle */
			pagepos = i*2 + 1;
		} else {
			/* cursor in 3/3 of tab (or after), put it right */
			pagepos = i*2 + 2;
		}
	}

	return pagepos;
}

static void hide_arrows(void)
{
	if (arrow_up != NULL) gtk_widget_hide(arrow_up);
	if (arrow_down != NULL) gtk_widget_hide(arrow_down);
	if (arrow_left != NULL) gtk_widget_hide(arrow_left);
	if (arrow_right != NULL) gtk_widget_hide(arrow_right);

	if (arrow_up_twist != NULL) gtk_widget_hide(arrow_up_twist);
	if (arrow_down_twist != NULL) gtk_widget_hide(arrow_down_twist);
	if (arrow_left_twist != NULL) gtk_widget_hide(arrow_left_twist);
	if (arrow_right_twist != NULL) gtk_widget_hide(arrow_right_twist);

	if (detach_box != NULL) gtk_widget_hide(detach_box);
}

static void show_arrows_horiz(GtkNotebook *notebook, GtkWidget *label, int pos)
{
	int x, y;

	gdk_window_get_origin(label->window, &x, &y);
	if (pos == 0)
		x -= MAGIC_PADDING;
	else
		x += label->allocation.width + MAGIC_PADDING;

	create_pixmap_popup(&arrow_down, arrow_down_xpm);
	create_pixmap_popup(&arrow_up, arrow_up_xpm);

	gtk_window_move(GTK_WINDOW(arrow_down),
			x - arrow_down->allocation.width / 2,
			y - arrow_down->allocation.height - MAGIC_PADDING);
	gtk_window_move(GTK_WINDOW(arrow_up),
			x - arrow_up->allocation.width / 2,
			y + label->allocation.height + MAGIC_PADDING);

	gtk_widget_show(arrow_down);
	gtk_widget_show(arrow_up);
}

static void show_arrows_horiz_into(GtkNotebook *notebook, GtkWidget *label)
{
	GtkWidget *arrow;
	int x, y;

	gdk_window_get_origin(label->window, &x, &y);
	x += label->allocation.width / 2;

	if (notebook->tab_pos == GTK_POS_TOP) {
		create_pixmap_popup(&arrow_up_twist, arrow_up_twist_xpm);
		arrow = arrow_up_twist;

		x -= arrow->allocation.width;
		y += arrow->allocation.height + MAGIC_PADDING;
	} else {
		create_pixmap_popup(&arrow_down_twist, arrow_down_twist_xpm);
		arrow = arrow_down_twist;

		x -= arrow->allocation.width / 2;
		y -= arrow->allocation.height + MAGIC_PADDING;
	}

	gtk_window_move(GTK_WINDOW(arrow), x, y);
	gtk_widget_show(arrow);
}

static void show_arrows_vert(GtkNotebook *notebook, GtkWidget *label, int pos)
{
	int x, y;

	gdk_window_get_origin(label->window, &x, &y);
	if (pos == 0)
		y -= MAGIC_PADDING;
	else
		y += label->allocation.height + MAGIC_PADDING;

	create_pixmap_popup(&arrow_right, arrow_right_xpm);
	create_pixmap_popup(&arrow_left, arrow_left_xpm);

	gtk_window_move(GTK_WINDOW(arrow_right),
                        x - arrow_right->allocation.width - MAGIC_PADDING,
			y - arrow_right->allocation.height / 2);
	gtk_window_move(GTK_WINDOW(arrow_left),
			x + label->allocation.width + MAGIC_PADDING,
			y - arrow_left->allocation.height / 2);

	gtk_widget_show(arrow_right);
	gtk_widget_show(arrow_left);
}

static void show_arrows_vert_into(GtkNotebook *notebook, GtkWidget *label)
{
	GtkWidget *arrow;
	int x, y;

	gdk_window_get_origin(label->window, &x, &y);
	y += label->allocation.height / 2;

	if (notebook->tab_pos == GTK_POS_LEFT) {
		create_pixmap_popup(&arrow_left_twist, arrow_left_twist_xpm);
		arrow = arrow_left_twist;

		x += label->allocation.width + MAGIC_PADDING;
		y -= arrow->allocation.height/3;
	} else {
		create_pixmap_popup(&arrow_right_twist, arrow_right_twist_xpm);
		arrow = arrow_right_twist;

		x -= arrow->allocation.width + MAGIC_PADDING;
		y -= arrow->allocation.height;
	}

	gtk_window_move(GTK_WINDOW(arrow), x, y);
	gtk_widget_show(arrow);
}

static void move_arrows(GtkNotebook *notebook, int pos)
{
	GtkWidget *child, *label;

	child = gtk_notebook_get_nth_page(notebook, pos < 2 ? 0 : (pos-1)/2);
	label = gtk_notebook_get_tab_label(notebook, child);

	hide_arrows();

	if (notebook->tab_pos == GTK_POS_TOP ||
	    notebook->tab_pos == GTK_POS_BOTTOM) {
		if ((pos & 1) == 0)
			show_arrows_horiz(notebook, label, pos);
		else
			show_arrows_horiz_into(notebook, label);
	} else {
		if ((pos & 1) == 0)
			show_arrows_vert(notebook, label, pos);
		else
			show_arrows_vert_into(notebook, label);
	}
}

static void tab_drop(Tab *tab)
{
	GtkWidget *child;
	GSList *tmp;
	Tab *newtab;
	int current_page, new_page;

	current_page = gtk_notebook_get_current_page(tab->frame->notebook);
	child = gtk_notebook_get_nth_page(tab->frame->notebook, current_page);

	if (tab->drag_pos < 0) {
		/* detaching as new window */
		if (g_list_length(tab->frame->notebook->children) == 1) {
			/* only tab in this window - no point in detaching */
			return;
		}

		/* remove from old frame */
		g_object_ref(G_OBJECT(child));
		gtk_widget_hide(child);
		gtk_notebook_remove_page(tab->frame->notebook, current_page);

		/* create and move into new frame */
		gui_frame_set_active(gui_frame_new());
		tab->frame = active_frame;
		gtk_notebook_append_page(tab->frame->notebook, tab->widget,
					 tab->tab_label);

		gtk_widget_show(child);
		g_object_unref(G_OBJECT(child));
		return;
	}

	new_page = tab->drag_pos/2;
	if (new_page == current_page)
		return;

	if ((tab->drag_pos & 1) == 0) {
		/* just move the tab */
		gtk_notebook_reorder_child(tab->frame->notebook,
					   child, new_page);
	} else {
		/* move into tab as split window */
		newtab = gui_frame_get_tab(tab->frame, new_page);
		g_return_if_fail(newtab != NULL);

		for (tmp = tab->views; tmp != NULL; tmp = tmp->next) {
			WindowView *view = tmp->data;

                        gui_window_add_view(view->window, newtab);
		}
                gtk_notebook_remove_page(tab->frame->notebook, current_page);
	}
}

static gboolean event_button_press(GtkWidget *widget, GdkEventButton *event,
				   Tab *tab)
{
	if (event->button != 1)
		return FALSE;

	tab->pressing = TRUE;
	tab->dragging = FALSE;
	return FALSE;
}

static gboolean event_button_release(GtkWidget *widget, GdkEventButton *event,
				     Tab *tab)
{
	if (event->button != 1 || !tab->dragging)
		return FALSE;

	if (gdk_pointer_is_grabbed()) {
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		gtk_grab_remove(widget);
	}

	tab->pressing = FALSE;
	tab->dragging = FALSE;
	hide_arrows();

	tab_drop(tab);
	return FALSE;
}

static gboolean event_motion(GtkWidget *widget, GdkEventButton *event,
			     Tab *tab)
{
	static GdkCursor *cursor = NULL;
	int pos, x, y, rootx, rooty;

	if (!tab->pressing)
		return FALSE;

	/* needed for motion to work, plus used when dragging */
	gdk_window_get_pointer(GTK_WIDGET(tab->frame->window)->window,
			       &x, &y, NULL);

	if (!tab->dragging) {
		/* when we've moved outside tab label, we've started dragging */
		if (event->x >= 0 && event->y >= 0 &&
		    event->x < widget->allocation.width &&
		    event->y < widget->allocation.height)
			return FALSE;

		/* setup dragging */
		tab->dragging = TRUE;
		tab->drag_pos = -1;

		/* grab the pointer */
		if (!cursor)
			cursor = gdk_cursor_new(GDK_FLEUR);

		gtk_grab_add(widget);
		gdk_pointer_grab(widget->window, FALSE,
				 GDK_BUTTON1_MOTION_MASK |
				 GDK_BUTTON_RELEASE_MASK,
				 NULL, cursor, 0);
	}

	if (x < 0 || y < 0 ||
	    x > GTK_WIDGET(tab->frame->window)->allocation.width ||
	    y > GTK_WIDGET(tab->frame->window)->allocation.height) {
		/* detaching window */
		if (!tab->detaching) {
			tab->detaching = TRUE;
			tab->drag_pos = -1;
			hide_arrows();
		}

		create_pixmap_popup(&detach_box, detach_box_xpm);

		gtk_window_get_position(tab->frame->window, &rootx, &rooty);
		gtk_window_move(GTK_WINDOW(detach_box), x + rootx, y + rooty);
		gtk_widget_show(detach_box);
	} else {
		/* notebook dragging - get position */
		pos = notebook_get_pos_at(tab->frame->notebook, x, y);
		if (pos == tab->drag_pos)
			return FALSE;

		/* moved */
		tab->drag_pos = pos;
		tab->detaching = FALSE;
		move_arrows(tab->frame->notebook, pos);
	}
	return FALSE;
}

GtkWidget *gui_tab_label_new(Tab *tab, GtkWidget *label)
{
	GtkWidget *eventbox;

	eventbox = gtk_event_box_new();
	g_signal_connect(G_OBJECT(eventbox), "button_press_event",
			 G_CALLBACK(event_button_press), tab);
	g_signal_connect(G_OBJECT(eventbox), "button_release_event",
			 G_CALLBACK(event_button_release), tab);
	g_signal_connect(G_OBJECT(eventbox), "motion_notify_event",
			 G_CALLBACK(event_motion), tab);
	gtk_container_add(GTK_CONTAINER(eventbox), label);
	gtk_widget_show_all(eventbox);
	return eventbox;
}
