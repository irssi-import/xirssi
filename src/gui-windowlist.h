#ifndef GUI_WINDOWLIST_H
#define GUI_WINDOWLIST_H

typedef struct {
	GtkWidget *widget;
	GtkWidget *treeview;
	GtkListStore *store;
	Frame *frame;
	gint changed_sig;
} WindowList;

#include "gui-frame.h"

WindowList *gui_windowlist_new(Frame *frame);

void gui_windowlist_init(void);
void gui_windowlist_deinit(void);

#endif
