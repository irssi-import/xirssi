#ifndef GUI_WINDOWLIST_H
#define GUI_WINDOWLIST_H

#include "gui-frame.h"

typedef struct {
	GtkWidget *widget;
	GtkListStore *store;
} WindowList;

WindowList *gui_windowlist_new(Frame *frame);

#endif
