#ifndef __GUI_NICKLIST_VIEW_H
#define __GUI_NICKLIST_VIEW_H

struct _NicklistView {
	Tab *tab;

	GtkWidget *widget;
	GtkTreeView *view;

	Nicklist *nicklist;
};

NicklistView *gui_nicklist_view_new(Tab *tab);

void gui_nicklist_view_set(NicklistView *view, Nicklist *nicklist);

#endif
