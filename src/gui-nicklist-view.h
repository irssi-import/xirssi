#ifndef __GUI_NICKLIST_VIEW_H
#define __GUI_NICKLIST_VIEW_H

struct _NicklistView {
	Tab *tab;

	GtkWidget *widget;
	GtkTreeView *view;
        GtkTreeViewColumn *column;

	Nicklist *nicklist;
};

NicklistView *gui_nicklist_view_new(Tab *tab);

void gui_nicklist_view_set(NicklistView *view, Nicklist *nicklist);
void gui_nicklist_view_update_label(NicklistView *view, const char *label);

void gui_nicklist_views_init(void);
void gui_nicklist_views_deinit(void);

#endif
