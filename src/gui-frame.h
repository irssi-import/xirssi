#ifndef __GUI_FRAME_H
#define __GUI_FRAME_H

struct _Frame {
	int refcount;

	GtkWindow *window;
	GtkNotebook *notebook;
	GtkStatusbar *statusbar;
	Entry *entry;

	Tab *active_tab;

	unsigned int destroying:1;
};

extern GSList *frames;
extern Frame *active_frame;

Frame *gui_frame_new(void);

void gui_frame_ref(Frame *frame);
void gui_frame_unref(Frame *frame);

Tab *gui_frame_new_tab(Frame *frame);
Tab *gui_frame_get_tab(Frame *frame, int page);

void gui_frame_set_active(Frame *frame);
void gui_frame_set_active_window(Frame *frame, Window *window);
void gui_frame_set_active_tab(Tab *tab);

#endif
