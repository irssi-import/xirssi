#ifndef __GUI_ITEMLIST_H
#define __GUI_ITEMLIST_H

struct _Itemlist {
	Frame *frame;

	GtkWidget *widget;
	GtkWidget *short_menu, *long_menu;
	GtkOptionMenu *menu;

	GSList *items;

	unsigned int window_items:1;
};

Itemlist *gui_itemlist_new(Frame *frame);

void gui_itemlists_init(void);
void gui_itemlists_deinit(void);

#endif
