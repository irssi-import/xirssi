/*
 setup-prefs.c : irssi

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
#include "glade/interface.h"

void setup_servers_init(GtkWidget *dialog);

typedef struct {
	const char *name;
	const char *page;
	void (*init_func)(GtkWidget *dialog);
} SetupPrefs;

static SetupPrefs prefs[] = {
	{ "Servers" },
	{ "Servers",		"page_servers", setup_servers_init },
	{ "Channels",		"page_channels" },
	{ "Settings",		"page_connect_settings" },

	{ "Logging" },
	{ "Automatic",		"page_autolog" },
	{ "Advanced",		"page_advanced_logging" },

	{ "Window Input" },
	{ "Ignores",		"page_ignores" },
	{ "Highlighting",	"page_highlighting" },
	{ "Settings",		"page_input_settings" },

	{ "User Interface" },
	{ "Windows",		"page_windows" },
	{ "Queries",		"page_queries" },
	{ "Completion",		"page_completion" },
	{ "Aliases",		"page_aliases" },
	{ "Keyboard",		"page_keyboard" },

	{ "Appearance" },
	{ "Window Output",	"page_window_output" },
	{ "Colors",		"page_colors" },
	{ "Themes",		"page_themes" }
};

static GtkWidget *dialog;

static gboolean event_destroy(GtkWidget *widget, Setup *setup)
{
	setup_destroy(setup);
	dialog = NULL;
	return FALSE;
}

static void event_response(GtkWidget *widget, int response_id, Setup *setup)
{
	/* ok / cancel pressed */
	if (response_id == GTK_RESPONSE_OK)
                setup_apply_changes(setup);

	gtk_widget_destroy(widget);
}

static gboolean event_main_clicked(GtkWidget *button, GtkWidget *list)
{
	GtkWidget *widget;
	GtkNotebook *notebook;

	/* hide the old list */
	widget = g_object_get_data(G_OBJECT(dialog), "active_sidebar_list");
	if (widget != NULL)
		gtk_widget_hide(widget);

	/* show our new list */
	if (widget == list)
		list = NULL;
	else {
		gtk_widget_show(list);

		/* show the active page of this list */
		widget = g_object_get_data(G_OBJECT(list), "active_button");
		if (widget != NULL)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
		else {
			notebook = g_object_get_data(G_OBJECT(dialog), "prefs_notebook");
			gtk_notebook_set_current_page(notebook, 0);
		}
	}

	g_object_set_data(G_OBJECT(dialog), "active_sidebar_list", list);
	return FALSE;
}

static gboolean event_sub_clicked(GtkWidget *button, const char *page)
{
	GtkWidget *widget;
	GtkNotebook *notebook;
	int page_num;

	widget = g_object_get_data(G_OBJECT(dialog), page);
	notebook = g_object_get_data(G_OBJECT(dialog), "prefs_notebook");

	page_num = gtk_notebook_page_num(notebook, widget);
	gtk_notebook_set_current_page(notebook, page_num);

	g_object_set_data(G_OBJECT(button->parent), "active_button", button);
	return FALSE;
}

static void setup_init_sidebar(GtkWidget *dialog)
{
	GtkWidget *vbox, *vbox2, *button, *label;
	GSList *group;
	int i;

	vbox = g_object_get_data(G_OBJECT(dialog), "sidebar");
	vbox2 = NULL;

	/* dummy radio button which will be selected by default */
	button = gtk_radio_button_new(NULL);
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));

	for (i = 0; i < G_N_ELEMENTS(prefs); i++) {
		if (prefs[i].page == NULL) {
			/* main section */
			button = gtk_button_new_with_label(prefs[i].name);
			gtk_widget_show(button);
			gtk_box_pack_start(GTK_BOX(vbox), button,
					   FALSE, FALSE, 0);

			vbox2 = gtk_vbox_new(FALSE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), vbox2, TRUE, TRUE, 0);

			g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(event_main_clicked), vbox2);
			continue;
		}

		/* subsection */
		button = gtk_radio_button_new(group);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));

		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);

		gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
		gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
		gtk_widget_show(button);

		label = gtk_label_new(prefs[i].name);
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		gtk_container_add(GTK_CONTAINER(button), label);
		gtk_widget_show(label);

		g_signal_connect(G_OBJECT(button), "clicked",
				 G_CALLBACK(event_sub_clicked),
				 (void *) prefs[i].page);
		if (prefs[i].init_func != NULL)
                        prefs[i].init_func(dialog);
	}
}

void setup_preferences_show(void)
{
	Setup *setup;

	if (dialog != NULL) {
		gdk_window_raise(dialog->window);
		return;
	}

	setup = setup_create(NULL, NULL);

	dialog = create_preferences();
	g_signal_connect(G_OBJECT(dialog), "destroy",
			 G_CALLBACK(event_destroy), setup);
	g_signal_connect(G_OBJECT(dialog), "response",
			 G_CALLBACK(event_response), setup);

	setup_init_sidebar(dialog);

	setup_register(setup, dialog);
	gtk_widget_show(dialog);
}
