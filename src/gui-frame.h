#ifndef __GUI_FRAME_H
#define __GUI_FRAME_H

struct _Frame {
	GtkWidget *widget;

	GtkWindow *window;
	GtkNotebook *notebook;
	GtkStatusbar *statusbar;
	Entry *entry;

	Tab *active_tab;
	unsigned int destroying:1;
};

extern GSList *frames;
extern Frame *active_frame;

Frame *gui_frame_new(int show);

void gui_frame_set_active(Frame *frame);
void gui_frame_set_active_window(Frame *frame, Window *window);

#endif
