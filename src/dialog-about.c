/*
 dialog-about.c : irssi

    Copyright (C) 2002 Timo Sirainen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "module.h"
#include "irssi-version.h"

#include "gui-menu.h"

#define IRSSI_COPYRIGHT "Copyright (C) 1999-2002 Timo Sirainen"

static GtkWidget *dialog = NULL;

static void event_destroy(void)
{
	dialog = NULL;
}

static gboolean event_mailto_press(GtkWidget *widget, GdkEventButton *event)
{
	gui_menu_url_popup("mailto:"IRSSI_AUTHOR_EMAIL, event->button);
	return FALSE;
}

static gboolean event_website_press(GtkWidget *widget, GdkEventButton *event)
{
	gui_menu_url_popup(IRSSI_WEBSITE, event->button);
	return FALSE;
}

void dialog_about_show(void)
{
	GtkWidget *vbox, *hbox, *eventbox1, *eventbox2, *label, *img, *hsep;
        PangoFontDescription *font_desc;
	GdkCursor *cursor;
	char str[100];

	if (dialog != NULL) {
		gdk_window_raise(dialog->window);
		return;
	}

	dialog = gtk_dialog_new_with_buttons("About", NULL, 0,
					     GTK_STOCK_OK, GTK_RESPONSE_OK,
					     NULL);
	g_signal_connect(G_OBJECT(dialog), "destroy",
			 GTK_SIGNAL_FUNC(event_destroy), NULL);
	g_signal_connect(G_OBJECT(dialog), "response",
			 GTK_SIGNAL_FUNC(gtk_widget_destroy), NULL);

	/* name */
	label = gtk_label_new(PACKAGE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,
			   FALSE, FALSE, 10);

	font_desc = pango_font_description_from_string("helvetica bold 25");
	gtk_widget_modify_font(label, font_desc);
	pango_font_description_free(font_desc);

	/* separator */
	/* name */
	hsep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hsep,
			   FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
			   TRUE, TRUE, 0);

	/* image */
	img = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,
				       GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(hbox), img, FALSE, FALSE, 0);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	/* version */
	g_snprintf(str, sizeof(str), "version " IRSSI_VERSION " (%d %04d)",
		   IRSSI_VERSION_DATE, IRSSI_VERSION_TIME);

	label = gtk_label_new(str);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/* copyright / email */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new(IRSSI_COPYRIGHT " <");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	eventbox1 = gtk_event_box_new();
	g_signal_connect(G_OBJECT(eventbox1), "button_press_event",
			 G_CALLBACK(event_mailto_press), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), eventbox1, FALSE, FALSE, 0);

	label = gtk_label_new("<u>"IRSSI_AUTHOR_EMAIL"</u>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_container_add(GTK_CONTAINER(eventbox1), label);

	label = gtk_label_new(">");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	/* web page */
	eventbox2 = gtk_event_box_new();
	g_signal_connect(G_OBJECT(eventbox2), "button_press_event",
			 G_CALLBACK(event_website_press), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), eventbox2, FALSE, FALSE, 0);

	label = gtk_label_new("<u>"IRSSI_WEBSITE"</u>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_container_add(GTK_CONTAINER(eventbox2), label);

	gtk_widget_show_all(dialog);

	/* use hand cursor over link */
	cursor = gdk_cursor_new(GDK_HAND2);
	gdk_window_set_cursor(eventbox1->window, cursor);
	gdk_window_set_cursor(eventbox2->window, cursor);
	gdk_cursor_unref(cursor);
}
