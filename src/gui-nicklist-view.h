#ifndef __GUI_NICKLIST_VIEW_H
#define __GUI_NICKLIST_VIEW_H

struct _NicklistView {
	int refcount;
	Tab *tab;

	GtkWidget *widget, *label;
	GtkTreeView *view;

	Nicklist *nicklist;
};

NicklistView *gui_nicklist_view_new(Tab *tab);

void gui_nicklist_view_ref(NicklistView *view);
void gui_nicklist_view_unref(NicklistView *view);

void gui_nicklist_view_set(NicklistView *view, Nicklist *nicklist);
void gui_nicklist_view_label_updated(NicklistView *view);

#endif
