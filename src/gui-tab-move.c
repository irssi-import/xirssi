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
#include "signals.h"

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
#include "gui-tab-move.h"
#include "gui-window.h"
#include "gui-window-view.h"

#define MAGIC_PADDING 4

static GtkWidget *arrow_up = NULL, *arrow_down = NULL;
static GtkWidget *arrow_up_twist = NULL, *arrow_down_twist = NULL;

static GtkWidget *arrow_left = NULL, *arrow_right = NULL;
static GtkWidget *arrow_left_twist = NULL, *arrow_right_twist = NULL;

static GtkWidget *detach_box = NULL;

typedef struct {
	Tab *orig_tab;
	TabPane *orig_pane;

	Frame *dest_frame;
	int dest_pos;

	unsigned int dragging:1;
	unsigned int detaching:1;
} TabDrag;

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

/* Returns Frame at specified location */
static Frame *frame_find_at(int x, int y)
{
	GSList *tmp;
	int winx, winy;

	for (tmp = frames; tmp != NULL; tmp = tmp->next) {
		Frame *frame = tmp->data;
		GtkAllocation *alloc = &GTK_WIDGET(frame->window)->allocation;

		gtk_window_get_position(frame->window, &winx, &winy);
		if (x >= winx && x < winx + alloc->width &&
		    y >= winy && y < winy + alloc->height)
			return frame;
	}

	return NULL;
}

/* Returns pos number*2, +1 if in center */
static int notebook_get_pos_at(GtkNotebook *notebook, int x, int y)
{
	GtkWidget *tab, *child;
	int winx, winy, horiz, pos, start, size, i, pagepos;

	gdk_window_get_origin(GTK_WIDGET(notebook)->window, &winx, &winy);

	horiz = notebook->tab_pos == GTK_POS_TOP ||
		notebook->tab_pos == GTK_POS_BOTTOM;
	pos = horiz ? x-winx : y-winy;

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

static void tab_pane_move(Tab *dest_tab, TabPane *pane)
{
	GtkPaned *paned;

	paned = gui_tab_add_paned(dest_tab);
	dest_tab->panes = g_list_prepend(dest_tab->panes, pane);
	pane->tab->panes = g_list_remove(pane->tab->panes, pane);

	/* update tabs' active windows */
	if (dest_tab->active_win == NULL && pane->view != NULL)
                gui_tab_set_active_window(dest_tab, pane->view->window->window);
        gui_tab_update_active_window(pane->tab);

	/* remove */
	gtk_widget_ref(pane->widget);
	gtk_widget_hide(pane->widget);
	gtk_container_remove(GTK_CONTAINER(pane->widget->parent), pane->widget);

	/* add */
	gtk_paned_add2(paned, pane->widget);
	gtk_widget_show(pane->widget);
	gtk_widget_unref(pane->widget);

	g_object_notify(G_OBJECT(pane->widget), "parent");
}

static void tab_drop_real(TabDrag *drag, int x, int y)
{
	GtkWidget *child;
	Frame *orig_frame;
	Tab *newtab;
	int current_page, new_page;

	child = drag->orig_tab->widget;
	current_page = gtk_notebook_page_num(drag->orig_tab->frame->notebook,
					     child);
	new_page = drag->detaching ? -1 : drag->dest_pos/2;

	orig_frame = drag->orig_tab->frame;
	if (!drag->detaching && (drag->dest_pos & 1)) {
		/* move into tab as split window */
		if (drag->dest_frame == orig_frame && new_page == current_page)
			return;

		newtab = gui_tab_get_page(drag->dest_frame, new_page);
		g_return_if_fail(newtab != NULL);

		if (drag->orig_pane != NULL) {
			/* move specified pane */
			tab_pane_move(newtab, drag->orig_pane);
		} else {
			/* move all panes in tab */
			while (drag->orig_tab->panes != NULL)
				tab_pane_move(newtab, drag->orig_tab->panes->data);
		}
	} else if (!drag->detaching && drag->dest_frame == orig_frame) {
		if (new_page == current_page || new_page == current_page+1)
			return;
		/* move the tab */
		gtk_notebook_reorder_child(orig_frame->notebook,
					   child, new_page);
	} else {
		/* move to another frame, or detach as new window */
		if (drag->detaching &&
		    g_list_length(orig_frame->notebook->children) == 1) {
			/* only tab in this window - no point in detaching */
			return;
		}

		/* remove from old frame */
		gtk_widget_ref(child);
		gtk_widget_hide(child);

		gtk_widget_ref(drag->orig_tab->tab_label);
		gtk_widget_hide(drag->orig_tab->tab_label);

		gtk_notebook_remove_page(orig_frame->notebook, current_page);

		/* create new frame if needed */
		if (drag->detaching) {
			drag->dest_frame = gui_frame_new(FALSE);
			gtk_window_move(drag->dest_frame->window, x, y);
			gtk_widget_show_all(GTK_WIDGET(drag->dest_frame->window));
		}

		/* move into new frame */
		drag->orig_tab->frame = drag->dest_frame;
		gtk_notebook_insert_page(drag->dest_frame->notebook,
					 drag->orig_tab->widget,
					 drag->orig_tab->tab_label, new_page);

		gtk_widget_show(drag->orig_tab->tab_label);
		gtk_widget_unref(drag->orig_tab->tab_label);

		gtk_widget_show(child);
		gtk_widget_unref(child);
	}
}

static void tab_drop(TabDrag *drag, int x, int y)
{
	Frame *orig_frame;
	Tab *newtab;

	orig_frame = drag->orig_tab->frame;

	if (drag->orig_pane != NULL && drag->orig_tab->panes->next == NULL) {
		/* only one pane in tab - treat the drag as if we're
		   moving the whole tab */
		drag->orig_pane = NULL;
	}

	if (drag->orig_pane != NULL &&
	    (drag->detaching || (drag->dest_pos & 1) == 0)) {
		/* extract pane into tab, so it's easier to move it */
		newtab = gui_tab_new(drag->orig_tab->frame);
		gtk_label_set_text(newtab->label,
				   gtk_label_get_text(drag->orig_tab->label));
		tab_pane_move(newtab, drag->orig_pane);

		gui_tab_pack_panes(drag->orig_tab);
		drag->orig_tab = newtab;
	}

	tab_drop_real(drag, x, y);

	/* kill tab if it doesn't have panes left */
	if (drag->orig_tab->panes == NULL)
		gtk_widget_destroy(drag->orig_tab->widget);
	else
                gui_tab_pack_panes(drag->orig_tab);

	/* kill frame if we moved it's last tab */
	if (orig_frame->notebook->children == NULL)
		gtk_widget_destroy(GTK_WIDGET(orig_frame->window));
}

static void drag_init(GtkWidget *widget, TabDrag *drag)
{
	static GdkCursor *cursor = NULL;

	/* setup dragging */
	drag->dragging = TRUE;
	drag->dest_pos = -1;

	/* grab the pointer */
	if (!cursor)
		cursor = gdk_cursor_new(GDK_FLEUR);

	gtk_grab_add(widget);
	gdk_pointer_grab(widget->window, FALSE,
			 GDK_BUTTON1_MOTION_MASK |
			 GDK_BUTTON_RELEASE_MASK,
			 NULL, cursor, 0);
}

static void drag_deinit(GtkWidget *widget)
{
	if (gdk_pointer_is_grabbed()) {
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		gtk_grab_remove(widget);
	}

	hide_arrows();
}

static gboolean event_motion(GtkWidget *widget, GdkEventButton *event,
			     TabDrag *drag)
{
	Frame *frame;
	int pos, x, y;

	if (!drag->dragging) {
		/* when we've moved outside tab label, we've started dragging */
		if (event->x >= 0 && event->y >= 0 &&
		    event->x < widget->allocation.width &&
		    event->y < widget->allocation.height)
			return FALSE;

		drag_init(widget, drag);
	}

	/* get x/y relative to root */
	gdk_window_get_pointer(gdk_get_default_root_window(),
			       &x, &y, NULL);

	frame = frame_find_at(x, y);
	if (frame != NULL) {
		/* dragging inside notebook */
		pos = notebook_get_pos_at(frame->notebook, x, y);
		if (pos == drag->dest_pos && !drag->detaching)
			return FALSE;

		/* moved */
		drag->dest_frame = frame;
		drag->dest_pos = pos;
		drag->detaching = FALSE;
		move_arrows(frame->notebook, pos);
	} else {
		/* cursor outside notebook - detaching window */
		if (!drag->detaching) {
			drag->detaching = TRUE;
			hide_arrows();
		}

		create_pixmap_popup(&detach_box, detach_box_xpm);

		gtk_window_move(GTK_WINDOW(detach_box), x, y);
		gtk_widget_show(detach_box);
	}
	return FALSE;
}

static gboolean event_button_release(GtkWidget *widget, GdkEventButton *event,
				     TabDrag *drag)
{
	GtkWidget *tab_widget, *pane_widget;
	int rootx, rooty;

	if (event->button != 1)
		return FALSE;

	g_signal_handlers_disconnect_by_func(widget,
					     G_CALLBACK(event_motion),
					     drag);
	g_signal_handlers_disconnect_by_func(widget,
					     G_CALLBACK(event_button_release),
					     drag);

	tab_widget = drag->orig_tab->widget;
	pane_widget = drag->orig_pane == NULL ? NULL :
		drag->orig_pane->widget;

	if (drag->dragging) {
		drag_deinit(widget);

		if (drag->detaching || drag->dest_pos != -1) {
			gdk_window_get_root_origin(widget->window,
						   &rootx, &rooty);
			tab_drop(drag, event->x + rootx, event->y + rooty);
		}
	}

	if (pane_widget != NULL)
		gtk_widget_unref(pane_widget);
	gtk_widget_unref(tab_widget);
	g_free(drag);
	return FALSE;
}

static gboolean event_button_press(GtkWidget *widget, GdkEventButton *event)
{
        TabDrag *drag;

	if (event->button != 1)
		return FALSE;

	drag = g_new0(TabDrag, 1);
	drag->orig_tab = gui_widget_find_data(widget, "Tab");
	drag->orig_pane = gui_widget_find_data(widget, "TabPane");

	gtk_widget_ref(drag->orig_tab->widget);
	if (drag->orig_pane != NULL) {
		gtk_widget_ref(drag->orig_pane->widget);

		/* it's a drag-button, start drag immediately */
		drag_init(widget, drag);
	}

	g_signal_connect(G_OBJECT(widget), "motion_notify_event",
			 G_CALLBACK(event_motion), drag);
	g_signal_connect(G_OBJECT(widget), "button_release_event",
			 G_CALLBACK(event_button_release), drag);
	return FALSE;
}

void gui_tab_move_label_init(GtkWidget *widget)
{
	g_signal_connect(G_OBJECT(widget), "button_press_event",
			 G_CALLBACK(event_button_press), NULL);
}
