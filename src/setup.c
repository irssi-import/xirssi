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

	if (!setup->changed && setup->changed_func != NULL) {
		/* notify once about the change */
		setup->changed_func(setup, setup->user_data);
		setup->changed = TRUE;
	}
	return TRUE;
}

void setup_register(Setup *setup, GtkWidget *widget)
{
	GList *tmp, *children;
	SETTINGS_REC *set;
	const char *name;

	/* recursively register all known widgets with settings */
	if (GTK_IS_CONTAINER(widget)) {
		children = gtk_container_get_children(GTK_CONTAINER(widget));
		for (tmp = children; tmp != NULL; tmp = tmp->next)
			setup_register(setup, tmp->data);
		g_list_free(children);
	}

	/* widget name contains the /SET name */
	name = gtk_widget_get_name(widget);
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

void setup_apply_changes(Setup *setup)
{
	if (!setup->changed)
		return;

	g_hash_table_foreach_remove(setup->changed_settings,
				    (GHRFunc) setup_apply_widget,
				    setup);

	signal_emit("setup changed", 1);
}
