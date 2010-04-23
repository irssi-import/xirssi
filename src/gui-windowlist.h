#ifndef GUI_WINDOWLIST_H
#define GUI_WINDOWLIST_H

#include "gui-frame.h"

typedef struct {
	GtkWidget *widget;
	GtkListStore *store;
} WindowList;

WindowList *gui_windowlist_new(Frame *frame);

void gui_windowlist_init(void);
void gui_windowlist_deinit(void);

#endif
