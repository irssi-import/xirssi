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
#include "glade/support.h"

void setup_servers_init(GtkWidget *dialog);
void setup_channels_init(GtkWidget *dialog);
void setup_aliases_init(GtkWidget *dialog);
void setup_completions_init(GtkWidget *dialog);

typedef struct {
	const char *name;
	const char *page;
	GdkPixbuf **pixbuf;
	void (*init_func)(GtkWidget *dialog);
} SetupPrefs;

static int images_loaded = FALSE;
static GdkPixbuf *servers_pixbuf, *channels_pixbuf, *connect_sets_pixbuf;
static GdkPixbuf *autolog_pixbuf, *adv_logging_pixbuf;
static GdkPixbuf *windows_pixbuf, *queries_pixbuf, *completion_pixbuf;
static GdkPixbuf *aliases_pixbuf, *keyboard_pixbuf;
static GdkPixbuf *ignores_pixbuf, *highlighting_pixbuf, *input_sets_pixbuf;
static GdkPixbuf *window_output_pixbuf, *colors_pixbuf, *themes_pixbuf;

static SetupPrefs prefs[] = {
	{ "Connections" },
	{ "Servers",		"page_servers", &servers_pixbuf, setup_servers_init },
	{ "Channels",		"page_channels", &channels_pixbuf, setup_channels_init },
	{ "Settings",		"page_connect_settings", &connect_sets_pixbuf },

	{ "Logging" },
	{ "Automatic",		"page_autolog", &autolog_pixbuf },
	{ "Advanced",		"page_advanced_logging", &adv_logging_pixbuf },

	{ "User Interface" },
	{ "Windows",		"page_windows", &windows_pixbuf },
	{ "Queries",		"page_queries", &queries_pixbuf },
	{ "Completion",		"page_completion", &completion_pixbuf, setup_completions_init },
	{ "Aliases",		"page_aliases", &aliases_pixbuf, setup_aliases_init },
	{ "Keyboard",		"page_keyboard", &keyboard_pixbuf },

	{ "Window Input" },
	{ "Ignores",		"page_ignores", &ignores_pixbuf },
	{ "Highlighting",	"page_highlighting", &highlighting_pixbuf },
	{ "Settings",		"page_input_settings", &input_sets_pixbuf },

	{ "Appearance" },
	{ "Window Output",	"page_window_output", &window_output_pixbuf },
	{ "Colors",		"page_colors", &colors_pixbuf },
	{ "Themes",		"page_themes", &themes_pixbuf }
};

GtkWidget *setup_dialog;

static gboolean event_destroy(GtkWidget *widget, Setup *setup)
{
	setup_destroy(setup);
	setup_dialog = NULL;
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
	widget = g_object_get_data(G_OBJECT(setup_dialog),
				   "active_sidebar_list");
	if (widget != NULL)
		gtk_widget_hide(widget);

	/* show our new list */
	if (widget == list)
		list = NULL;
	else {
		gtk_widget_show(list);

		/* show the active page of this list */
		widget = g_object_get_data(G_OBJECT(list), "active_button");
		if (widget != NULL) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
						     TRUE);
		} else {
			notebook = g_object_get_data(G_OBJECT(setup_dialog),
						     "prefs_notebook");
			gtk_notebook_set_current_page(notebook, 0);
		}
	}

	g_object_set_data(G_OBJECT(setup_dialog), "active_sidebar_list", list);
	return FALSE;
}

static gboolean event_sub_clicked(GtkWidget *button, const char *page)
{
	GtkWidget *widget;
	GtkNotebook *notebook;
	int page_num;

	widget = g_object_get_data(G_OBJECT(setup_dialog), page);
	notebook = g_object_get_data(G_OBJECT(setup_dialog), "prefs_notebook");

	page_num = gtk_notebook_page_num(notebook, widget);
	gtk_notebook_set_current_page(notebook, page_num);

	g_object_set_data(G_OBJECT(button->parent), "active_button", button);
	return FALSE;
}

static void setup_init_sidebar(GtkWidget *dialog)
{
	GtkWidget *vbox, *vbox2, *vbox3, *button, *label, *img;
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

		vbox3 = gtk_vbox_new(FALSE, 0);
		gtk_widget_show(vbox3);
		gtk_container_add(GTK_CONTAINER(button), vbox3);

		if (prefs[i].pixbuf != NULL) {
			img = gtk_image_new_from_pixbuf(*prefs[i].pixbuf);
			gtk_widget_show(img);
			gtk_box_pack_start(GTK_BOX(vbox3), img,
					   FALSE, FALSE, 0);
		}

		label = gtk_label_new(prefs[i].name);
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		gtk_box_pack_start(GTK_BOX(vbox3), label, FALSE, FALSE, 0);
		gtk_widget_show(label);

		g_signal_connect(G_OBJECT(button), "clicked",
				 G_CALLBACK(event_sub_clicked),
				 (void *) prefs[i].page);
		if (prefs[i].init_func != NULL)
                        prefs[i].init_func(dialog);
	}
}

static void images_init(void)
{
	servers_pixbuf = create_pixbuf("servers.png");
	channels_pixbuf = create_pixbuf("channels.png");
	connect_sets_pixbuf = create_pixbuf("settings.png");

	autolog_pixbuf = create_pixbuf("autolog.png");
	adv_logging_pixbuf = create_pixbuf("advanced-logging.png");

	windows_pixbuf = create_pixbuf("windows.png");
	queries_pixbuf = create_pixbuf("queries.png");
	completion_pixbuf = create_pixbuf("completion.png");
	aliases_pixbuf = create_pixbuf("aliases.png");
	keyboard_pixbuf = create_pixbuf("keyboard.png");

	ignores_pixbuf = create_pixbuf("ignores.png");
	highlighting_pixbuf = create_pixbuf("highlighting.png");
	input_sets_pixbuf = create_pixbuf("input-settings.png");

	window_output_pixbuf = create_pixbuf("window-output.png");
	colors_pixbuf = create_pixbuf("colors.png");
	themes_pixbuf = create_pixbuf("themes.png");
}

void setup_preferences_show(void)
{
	Setup *setup;

	if (setup_dialog != NULL) {
		gdk_window_raise(setup_dialog->window);
		return;
	}

	if (!images_loaded) {
		images_init();
		images_loaded = TRUE;
	}

	setup = setup_create(NULL, NULL);

	setup_dialog = create_preferences();
	g_signal_connect(G_OBJECT(setup_dialog), "destroy",
			 G_CALLBACK(event_destroy), setup);
	g_signal_connect(G_OBJECT(setup_dialog), "response",
			 G_CALLBACK(event_response), setup);

	setup_init_sidebar(setup_dialog);

	setup_register(setup, setup_dialog);
	gtk_widget_show(setup_dialog);
}

void setup_preferences_destroy(void)
{
	if (setup_dialog != NULL)
		gtk_widget_destroy(setup_dialog);
}
