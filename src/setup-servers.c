/*
 setup-servers.c : irssi

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

#include "setup.h"

static GtkWidget *server_dialog = NULL;

static gboolean event_destroy(GtkWidget *widget, Setup *setup)
{
	setup_destroy(setup);
	server_dialog = NULL;
	return FALSE;
}

static void event_changed(Setup *setup, void *user_data)
{
}

static void event_response(GtkWidget *widget, int response_id, Setup *setup)
{
	if (response_id == GTK_RESPONSE_OK)
                setup_apply_changes(setup);

	gtk_widget_destroy(widget);
}

static GtkWidget *server_tree_new(void)
{
	GtkWidget *frame, *hbox, *sw, *view, *buttonbox, *button;
	GtkTreeView *tree_view;

	hbox = gtk_hbox_new(FALSE, 7);

	/* scrolled window */
	frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), sw);

	/* tree view */
	view = gtk_tree_view_new();
	tree_view = GTK_TREE_VIEW(view);
	gtk_container_add(GTK_CONTAINER(sw), view);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tree_view),
				    GTK_SELECTION_MULTIPLE);

	/* add/edit/remove */
	buttonbox = gtk_vbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox),
				  GTK_BUTTONBOX_START);
	gtk_box_set_spacing(GTK_BOX(buttonbox), 7);
	gtk_box_pack_start(GTK_BOX(hbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new_with_label("Add");
	gtk_container_add(GTK_CONTAINER(buttonbox), button);

	button = gtk_button_new_with_label("Edit");
	gtk_container_add(GTK_CONTAINER(buttonbox), button);

	button = gtk_button_new_with_label("Remove");
	gtk_container_add(GTK_CONTAINER(buttonbox), button);

	return hbox;
}

void setup_servers_show(void)
{
	GtkWidget *dialog, *vbox, *vbox2, *hbox, *frame, *table;
	GtkWidget *tree, *label, *checkbox, *spin, *entry;
	Setup *setup;

	if (server_dialog != NULL) {
		gdk_window_raise(server_dialog->window);
		return;
	}

	server_dialog = dialog =
		gtk_dialog_new_with_buttons("Server Settings", NULL, 0,
					    GTK_STOCK_OK, GTK_RESPONSE_OK,
					    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					    NULL);
	setup = setup_create(event_changed, dialog);

	g_signal_connect(G_OBJECT(dialog), "destroy",
			 G_CALLBACK(event_destroy), setup);
	g_signal_connect(G_OBJECT(dialog), "response",
			 G_CALLBACK(event_response), setup);

	vbox = gtk_vbox_new(FALSE, 7);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 7);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
			   vbox, TRUE, TRUE, 0);

	/* servers list */
	frame = gtk_frame_new("Servers");
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 7);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	tree = server_tree_new();
	gtk_box_pack_start(GTK_BOX(vbox2), tree, TRUE, TRUE, 0);

	/* motd */
	checkbox = gtk_check_button_new_with_label("Skip MOTD while connecting");
	gtk_widget_set_name(checkbox, "skip_motd");
	gtk_box_pack_start(GTK_BOX(vbox2), checkbox, FALSE, FALSE, 0);

	/* IPv6 */
	checkbox = gtk_check_button_new_with_label("Prefer IPv6 addresses with DNS lookups");
	gtk_widget_set_name(checkbox, "resolve_prefer_ipv6");
	gtk_box_pack_start(GTK_BOX(vbox2), checkbox, FALSE, FALSE, 0);

	/* reconnect wait */
	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	label = gtk_label_new("Seconds to wait before reconnecting (-1 = never reconnect)");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	spin = gtk_spin_button_new(NULL, 0, 0);
        gtk_spin_button_set_range(GTK_SPIN_BUTTON(spin), -1, G_MAXINT);
        gtk_spin_button_set_increments(GTK_SPIN_BUTTON(spin), 1, 60);
	gtk_widget_set_name(spin, "server_reconnect_time");
	gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 0);

	/* source host */
	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	label = gtk_label_new("Source host (\"virtual host\")");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	entry = gtk_entry_new();
	gtk_widget_set_name(entry, "hostname");
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

	/* user information */
	frame = gtk_frame_new("User Information");
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

	table = gtk_table_new(3, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_set_border_width(GTK_CONTAINER(table), 7);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 7);

	label = gtk_label_new("Nick");
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 GTK_FILL, 0, 0, 0);

	entry = gtk_entry_new();
	gtk_widget_set_name(entry, "nick");
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 0, 1);

	label = gtk_label_new("Alternate Nick");
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			 GTK_FILL, 0, 0, 0);

	entry = gtk_entry_new();
	gtk_widget_set_name(entry, "alternate_nick");
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 1, 2);

	label = gtk_label_new("User Name");
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
			 GTK_FILL, 0, 0, 0);

	entry = gtk_entry_new();
	gtk_widget_set_name(entry, "user_name");
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 2, 3);

	label = gtk_label_new("Real Name");
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
			 GTK_FILL, 0, 0, 0);

	entry = gtk_entry_new();
	gtk_widget_set_name(entry, "real_name");
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 3, 4);

	gtk_widget_show_all(dialog);

	setup_register(setup, dialog);
}

void setup_servers_init(void)
{
}

void setup_servers_deinit(void)
{
	if (server_dialog != NULL)
		gtk_widget_destroy(server_dialog);
}
