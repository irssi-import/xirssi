#ifndef __GUI_TAB_H
#define __GUI_TAB_H

struct _Tab {
	int refcount;
	Frame *frame;

	GtkWidget *widget;
	GtkWidget *tab_label;
	GtkLabel *label;

	GList *panes;
	GSList *views;

	NicklistView *nicklist;
	Window *active_win;
	int data_level;

	Frame *drag_frame;
	int drag_pos;
	unsigned int pressing:1;
	unsigned int dragging:1;
	unsigned int detaching:1;

	unsigned int destroying:1;
};

Tab *gui_tab_new(Frame *frame);

void gui_tab_ref(Tab *tab);
void gui_tab_unref(Tab *tab);

GtkBox *gui_tab_add_widget(Tab *tab, GtkWidget *widget);
void gui_tab_remove_widget(Tab *tab, GtkWidget *widget);

void gui_tab_set_active_window(Tab *tab, Window *window);
void gui_tab_set_active_window_item(Tab *tab, Window *window);

#endif
