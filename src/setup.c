/*
 setup.c : irssi

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
#include "signals.h"
#include "misc.h"
#include "settings.h"

#include "setup.h"
#include "glade/support.h"

struct _Setup {
	GHashTable *changed_settings;

	SetupChangedCallback changed_func;
	void *user_data;

	unsigned int changed:1;
};

Setup *setup_create(SetupChangedCallback changed_func, void *user_data)
{
	Setup *setup;

	setup = g_new0(Setup, 1);
	setup->changed_settings = g_hash_table_new(NULL, NULL);
	setup->changed_func = changed_func;
	setup->user_data = user_data;
	return setup;
}

void setup_destroy(Setup *setup)
{
	g_hash_table_destroy(setup->changed_settings);
	g_free(setup);
}

static gboolean event_changed(GtkWidget *widget, Setup *setup)
{
	SETTINGS_REC *set;

	set = g_object_get_data(G_OBJECT(widget), "set");
	g_hash_table_insert(setup->changed_settings, widget, set);

	if (!setup->changed) {
		setup->changed = TRUE;
		if (setup->changed_func != NULL) {
			/* notify once about the change */
			setup->changed_func(setup, setup->user_data);
		}
	}
	return TRUE;
}

void setup_register(Setup *setup, GtkWidget *widget)
{
	GList *tmp, *children;
	SETTINGS_REC *set;
	const char *name;
	char number[MAX_INT_STRLEN];

	/* recursively register all known widgets with settings */
	if (GTK_IS_CONTAINER(widget)) {
		children = gtk_container_get_children(GTK_CONTAINER(widget));
		for (tmp = children; tmp != NULL; tmp = tmp->next)
			setup_register(setup, tmp->data);
		g_list_free(children);
	}

	/* widget name contains the /SET name */
	name = gtk_widget_get_name(widget);

	if (GTK_IS_BUTTON(widget)) {
		/* ..unless it's a button, it contains <buttontype>_setname */
		GtkWidget *w2;

		if (strncmp(name, "level_", 6) == 0) {
			w2 = lookup_widget(widget, name+6);
			setup_register_level_button(widget, w2);
		} else if (strncmp(name, "browse_", 7) == 0) {
			w2 = lookup_widget(widget, name+7);
                        setup_register_dir_button(widget, w2);
		} else if (strncmp(name, "irssicolor_", 11) == 0) {
			w2 = lookup_widget(widget, name+11);
                        setup_register_irssicolor_button(widget, w2);
		} else {
			/* maybe a button which has color eventbox inside? */
			children = gtk_container_get_children(GTK_CONTAINER(widget));
			w2 = children == NULL ? NULL : children->data;
			g_list_free(children);

			if (GTK_IS_EVENT_BOX(w2)) {
				name = gtk_widget_get_name(w2);
				if (strncmp(name, "color_", 6) == 0)
					setup_register_color_button(widget, w2);
			}
		}
	}

	set = name == NULL ? NULL : settings_get_record(name);
	if (set == NULL)
		return;

	if (GTK_IS_TOGGLE_BUTTON(widget)) {
		/* GtkToggleButton == boolean */
		g_return_if_fail(set->type == SETTING_TYPE_BOOLEAN);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
					     settings_get_bool(set->key));
		g_signal_connect(G_OBJECT(widget), "toggled",
				 G_CALLBACK(event_changed), setup);
	} else if (GTK_IS_ENTRY(widget) || GTK_IS_SPIN_BUTTON(widget)) {
		g_return_if_fail(set->type != SETTING_TYPE_BOOLEAN);

		if (GTK_IS_SPIN_BUTTON(widget)) {
			g_return_if_fail(set->type == SETTING_TYPE_INT);

			gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),
						  settings_get_int(set->key));
		} else if (set->type == SETTING_TYPE_INT) {
			ltoa(number, settings_get_int(set->key));
			gtk_entry_set_text(GTK_ENTRY(widget), number);
		} else {
			g_return_if_fail(set->type == SETTING_TYPE_STRING);

			gtk_entry_set_text(GTK_ENTRY(widget),
					   settings_get_str(set->key));
		}

		g_signal_connect(G_OBJECT(widget), "changed",
				 G_CALLBACK(event_changed), setup);
	} else {
		/* unknown widget type */
		g_warning("Unknown widget type for setting %s", set->key);
		return;
	}

	g_object_set_data(G_OBJECT(widget), "set", set);
}

void setup_register_level_button(GtkWidget *button, GtkWidget *entry)
{
	/* FIXME: */
}

void setup_register_irssicolor_button(GtkWidget *button, GtkWidget *entry)
{
	/* FIXME: */
}

static void event_color_response(GtkColorSelectionDialog *csd, int response_id,
				 GtkWidget *eventbox)
{
	GdkColor color;

	/* ok / cancel pressed */
	if (response_id == GTK_RESPONSE_OK) {
		gtk_color_selection_get_current_color(
			GTK_COLOR_SELECTION(csd->colorsel), &color);
                gtk_widget_modify_bg(eventbox, GTK_STATE_NORMAL, &color);
                gtk_widget_modify_bg(eventbox, GTK_STATE_PRELIGHT, &color);
                gtk_widget_modify_bg(eventbox, GTK_STATE_ACTIVE, &color);
	}

	gtk_widget_destroy(GTK_WIDGET(csd));
}

static gboolean event_color_button_clicked(GtkWidget *button,
					   GtkWidget *eventbox)
{
	GtkWidget *colorsel;
	GtkColorSelectionDialog *csd;

	colorsel = gtk_color_selection_dialog_new(NULL);
	csd = GTK_COLOR_SELECTION_DIALOG(colorsel);
	gtk_color_selection_set_current_color(
		GTK_COLOR_SELECTION(csd->colorsel),
		&eventbox->style->bg[GTK_STATE_NORMAL]);

	g_signal_connect_swapped(G_OBJECT(colorsel), "destroy",
				 G_CALLBACK(gtk_widget_unref), eventbox);
	g_signal_connect(G_OBJECT(colorsel), "response",
			 G_CALLBACK(event_color_response), eventbox);
	gtk_widget_ref(eventbox);
	gtk_widget_show(colorsel);
	return FALSE;
}

void setup_register_color_button(GtkWidget *button, GtkWidget *eventbox)
{
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_color_button_clicked), eventbox);
}

static void event_dir_response(GtkFileSelection *filesel, int response_id,
			       GtkEntry *entry)
{
	const char *fname;

	/* ok / cancel pressed */
	if (response_id == GTK_RESPONSE_OK) {
                fname = gtk_file_selection_get_filename(filesel);
		gtk_entry_set_text(entry, fname);
	}

	gtk_widget_destroy(GTK_WIDGET(filesel));
}

static gboolean event_dir_button_clicked(GtkWidget *button, GtkEntry *entry)
{
	GtkWidget *filesel;

	filesel = gtk_file_selection_new(NULL);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel),
					gtk_entry_get_text(entry));

	g_signal_connect_swapped(G_OBJECT(filesel), "destroy",
				 G_CALLBACK(gtk_widget_unref), entry);
	g_signal_connect(G_OBJECT(filesel), "response",
			 G_CALLBACK(event_dir_response), entry);
	gtk_widget_ref(GTK_WIDGET(entry));
	gtk_widget_show(filesel);
	return FALSE;
}

void setup_register_dir_button(GtkWidget *button, GtkWidget *entry)
{
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_dir_button_clicked), entry);
}

static int setup_apply_widget(GtkWidget *widget, SETTINGS_REC *set,
			      Setup *setup)
{
	const char *str_value = NULL;
	char str[MAX_INT_STRLEN];
	int int_value = -1;
	gboolean bool_value;

	if (GTK_IS_SPIN_BUTTON(widget))
                int_value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	else if (GTK_IS_ENTRY(widget))
		str_value = gtk_entry_get_text(GTK_ENTRY(widget));
	else if (!GTK_IS_TOGGLE_BUTTON(widget)) {
		/* unknown widget type - we should have
		   noticed it while registering */
		g_error("unknown widget for setting %s", set->key);
	}

	switch (set->type) {
	case SETTING_TYPE_BOOLEAN:
		/* boolean is always GtkToggleButton */
		bool_value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
		settings_set_bool(set->key, bool_value);
		break;

	case SETTING_TYPE_INT:
		if (str_value != NULL)
			int_value = atoi(str_value);
		settings_set_int(set->key, int_value);
		break;
	case SETTING_TYPE_STRING:
		if (str_value == NULL) {
			ltoa(str, int_value);
			str_value = str;
		}
		settings_set_str(set->key, str_value);
		break;
	}

	return TRUE;
}

void setup_set_changed(Setup *setup)
{
	setup->changed = TRUE;
}

void setup_apply_changes(Setup *setup)
{
	if (!setup->changed)
		return;

	if (g_hash_table_size(setup->changed_settings) != 0) {
		g_hash_table_foreach_remove(setup->changed_settings,
					    (GHRFunc) setup_apply_widget,
					    setup);
		signal_emit("setup changed", 1);
	}

	settings_save(NULL, FALSE);
}
