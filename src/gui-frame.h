#ifndef __GUI_FRAME_H
#define __GUI_FRAME_H

struct _Frame {
	GtkWidget *widget;

	GtkWindow *window;
	GtkNotebook *notebook;
	GtkWidget *menubar;
	GtkStatusbar *statusbar;
	Entry *entry;
	Itemlist *itemlist;

	GSList *focusable_widgets;

	Tab *active_tab;
	unsigned int destroying:1;
};

extern GSList *frames;
extern Frame *active_frame;

Frame *gui_frame_new(int show);

void gui_frame_add_focusable_widget(Frame *frame, GtkWidget *widget);
void gui_frame_remove_focusable_widget(Frame *frame, GtkWidget *widget);

void gui_frame_set_active(Frame *frame);
void gui_frame_set_active_window(Frame *frame, Window *window);

#endif
