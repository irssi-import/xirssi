#ifndef __ENTRY_H
#define __ENTRY_H

#include "keyboard.h"

struct _Entry {
	int refcount;

	GtkWidget *widget;
	GtkEntry *entry;
	Frame *frame;

	Window *active_win;
	Keyboard *keyboard;
};

Entry *gui_entry_new(Frame *frame);

void gui_entry_ref(Entry *entry);
int gui_entry_unref(Entry *entry);

void gui_entry_set_window(Entry *entry, Window *window);

#endif
