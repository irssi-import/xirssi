#ifndef __GUI_WINDOW_CONTEXT_H
#define __GUI_WINDOW_CONTEXT_H

void gui_window_print_mark_context(WindowGui *window, TextDest *dest,
				   GtkTextIter *iter, const char *text);

/* motion_notify_event handler */
gboolean gui_window_context_event_motion(GtkWidget *widget, GdkEvent *event,
					 WindowView *view);

void gui_window_contexts_init(void);
void gui_window_contexts_deinit(void);

#endif
