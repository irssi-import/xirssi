#ifndef __GUI_WINDOW_VIEW_H
#define __GUI_WINDOW_VIEW_H

struct _WindowView {
	TabPane *pane;
	WindowGui *window;

	GtkWidget *widget, *title;
	GtkTextView *view;

	GtkAdjustment *adj;

	int font_width, font_height;
	int approx_width, approx_height; /* as characters */

	gulong sig_changed;

	unsigned int bottom:1;
	unsigned int cursor_link:1;
};

WindowView *gui_window_view_new(TabPane *pane, WindowGui *window,
				GtkTextBuffer *buffer);

void gui_window_view_set_title(WindowView *view);

void gui_window_views_init(void);
void gui_window_views_deinit(void);

#endif
