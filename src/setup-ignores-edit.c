/*
 setup-ignores-edit.c : irssi

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
#include "levels.h"
#include "ignore.h"
#include "misc.h"

#include "setup.h"
#include "gui.h"
#include "glade/interface.h"

static char *list_get_channels(GtkTreeModel *model)
{
	GtkTreeIter iter;
	GString *str;
	char *channel, *ret;

	if (!gtk_tree_model_get_iter_first(model, &iter))
		return NULL;

	str = g_string_new(NULL);
	do {
		gtk_tree_model_get(model, &iter, 0, &channel, -1);
		if (str->len != 0)
			g_string_append_c(str, ' ');
		g_string_append(str, channel);
		g_free(channel);
	} while (gtk_tree_model_iter_next(model, &iter));

	ret = str->str;
	g_string_free(str, FALSE);

	return ret;
}

static void ignore_save(GObject *obj, Ignore *ignore)
{
	GtkEntry *entry;
	GtkToggleButton *toggle;
	GtkSpinButton *spin;
	GtkTreeModel *model;
	const char *mask;
	char *channels;
	int secs, create_new;

	create_new = ignore == NULL;
	if (create_new) {
		ignore = g_new0(IGNORE_REC, 1);
	} else {
		g_free(ignore->mask);
		g_strfreev(ignore->channels);
	}

	/* mask */
	entry = g_object_get_data(obj, "mask");
	mask = gtk_entry_get_text(entry);
	ignore->mask = mask == NULL || *mask == '\0' ||
		strcmp(mask, "*") == 0 ? NULL : g_strdup(mask);

	/* level */
	entry = g_object_get_data(obj, "level");
	ignore->level = level2bits(gtk_entry_get_text(entry));
	if (ignore->level == 0)
		ignore->level = MSGLEVEL_ALL;

	/* exception, replies */
	toggle = g_object_get_data(obj, "exception");
	ignore->exception = gtk_toggle_button_get_active(toggle);

	toggle = g_object_get_data(obj, "replies");
	ignore->replies = gtk_toggle_button_get_active(toggle);

	/* pattern + flags */
	gui_entry_update(obj, "pattern", &ignore->pattern);

	toggle = g_object_get_data(obj, "fullword");
	ignore->fullword = gtk_toggle_button_get_active(toggle);

	toggle = g_object_get_data(obj, "regexp");
	ignore->regexp = gtk_toggle_button_get_active(toggle);

	/* get channels */
	model = g_object_get_data(g_object_get_data(obj, "channel_tree"),
				  "store");
	channels = list_get_channels(model);
	ignore->channels = channels == NULL ? NULL :
		g_strsplit(channels, " ", -1);
	g_free(channels);

	/* expire */
	spin = g_object_get_data(obj, "expire_days");
	secs = gtk_spin_button_get_value(spin) * (24*3600);
	spin = g_object_get_data(obj, "expire_hours");
	secs += gtk_spin_button_get_value(spin) * (3600);
	spin = g_object_get_data(obj, "expire_mins");
	secs += gtk_spin_button_get_value(spin) * (60);

	if (secs > 0)
		ignore->unignore_time = time(NULL) + secs;

	if (create_new)
		ignore_add_rec(ignore);
	else
		ignore_update_rec(ignore);
}

static gboolean event_response(GtkWidget *widget, int response_id,
			       Ignore *ignore)
{
	/* ok / cancel pressed */
	if (response_id == GTK_RESPONSE_OK)
		ignore_save(G_OBJECT(widget), ignore);

	gtk_widget_destroy(widget);
	return FALSE;
}

static void ignore_channels_store_fill(GtkListStore *store, Ignore *ignore)
{
	GtkTreeIter iter;
	char **tmp;

	if (ignore->channels == NULL)
		return;

	for (tmp = ignore->channels; *tmp != NULL; tmp++) {
		gtk_list_store_prepend(store, &iter);
		gtk_list_store_set(store, &iter, 0, *tmp, -1);
	}
}

static void regexp_invalid_update(GtkToggleButton *toggle, GtkEntry *entry,
				  GtkWidget *label)
{
	regex_t preg;
	const char *text;

	if (!gtk_toggle_button_get_active(toggle)) {
		/* not a regexp */
		gtk_widget_hide(label);
		return;
	}

	text = gtk_entry_get_text(entry);
	if (regcomp(&preg, text, REG_EXTENDED|REG_ICASE|REG_NOSUB) == 0) {
		/* regexp ok */
                regfree(&preg);

		gtk_widget_hide(label);
	} else {
		/* invalid regexp - show the error label */
		gtk_widget_show(label);
	}
}

static gboolean event_pattern_changed(GtkEntry *entry, GObject *obj)
{
	regexp_invalid_update(g_object_get_data(obj, "regexp"), entry,
			      g_object_get_data(obj, "regexp_invalid"));
	return FALSE;
}

static gboolean event_regexp_toggled(GtkToggleButton *toggle, GObject *obj)
{
	regexp_invalid_update(toggle, g_object_get_data(obj, "pattern"),
			      g_object_get_data(obj, "regexp_invalid"));
	return FALSE;
}

static int timeout_edit_channel(GtkTreeView *view)
{
	GtkTreeViewColumn *column;
        GtkTreePath *path;

	path = g_object_get_data(G_OBJECT(view), "path");

	column = gtk_tree_view_get_column(view, 0);
	gtk_tree_view_set_cursor(view, path, column, TRUE);
	gtk_tree_path_free(path);

	return 0;
}

static gboolean event_channel_add(GtkWidget *widget, GtkTreeView *view)
{
        GtkListStore *store;
	GtkTreePath *path;
	GtkTreeIter iter;

        store = g_object_get_data(G_OBJECT(view), "store");
	gtk_list_store_prepend(store, &iter);
	gtk_list_store_set(store, &iter, 0, "#channel", -1);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);

	/* FIXME: this is quite a horrible kludge, but I can't see any
	   other way to do it - even 0 timeout doesn't work.
	   gtk_tree_view_set_cursor() is buggy with newly prepended items
	   for some reason - hopefully will be fixed soon. And since our
	   list is sorted, we can't append it either. */
	g_object_set_data(G_OBJECT(view), "path", path);
	g_timeout_add(100, (GSourceFunc) timeout_edit_channel, view);
	return FALSE;
}

static gboolean event_channel_remove(GtkWidget *widget, GtkTreeView *view)
{
	gui_tree_selection_delete(g_object_get_data(G_OBJECT(view), "store"),
				  view, NULL, NULL);
	return FALSE;
}

static gboolean channel_edited(GtkCellRendererText *cell, char *path_string,
			       char *new_text, GtkListStore *store)
{
	GtkTreePath *path;
	GtkTreeIter iter;

	if (*new_text == '\0') {
		gui_popup_error("Channel name can't be empty");
		return FALSE;
	}

	path = gtk_tree_path_new_from_string(path_string);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
	gtk_tree_path_free(path);

	/* update store */
	gtk_list_store_set(store, &iter, 0, new_text, -1);
	return FALSE;
}

static gboolean event_hours_changed(GtkSpinButton *spin, GObject *obj)
{
	GtkSpinButton *days_spin;
	int days, hours;

	days_spin = g_object_get_data(G_OBJECT(obj), "expire_days");
	hours = gtk_spin_button_get_value_as_int(spin);
	days = gtk_spin_button_get_value_as_int(days_spin);

	if (hours < 0) {
		/* negative hours, see if we can decrease the days */
		if (days == 0)
                        gtk_spin_button_set_value(spin, 0);
		else {
                        gtk_spin_button_set_value(days_spin, days-1);
			gtk_spin_button_set_value(spin, 23);
		}
	} else if (hours > 23) {
		/* increase days */
		gtk_spin_button_set_value(days_spin, days+1);
		gtk_spin_button_set_value(spin, 0);
	}
	return FALSE;
}

static gboolean event_mins_changed(GtkSpinButton *spin, GObject *obj)
{
	GtkSpinButton *hours_spin, *days_spin;
	int hours, mins;

	hours_spin = g_object_get_data(G_OBJECT(obj), "expire_hours");
	mins = gtk_spin_button_get_value_as_int(spin);
	hours = gtk_spin_button_get_value_as_int(hours_spin);

	if (mins < 0) {
		/* negative mins, see if we can decrease the hours */
		days_spin = g_object_get_data(G_OBJECT(obj), "expire_days");
		if (hours == 0 &&
		    gtk_spin_button_get_value_as_int(days_spin) == 0)
			gtk_spin_button_set_value(spin, 0);
		else {
			gtk_spin_button_set_value(hours_spin, hours-1);
			gtk_spin_button_set_value(spin, 59);
		}
	} else if (mins > 59) {
		/* increase hours */
		gtk_spin_button_set_value(hours_spin, hours+1);
		gtk_spin_button_set_value(spin, 0);
	}
	return FALSE;
}

void ignore_dialog_show(Ignore *ignore)
{
	GtkWidget *dialog, *label;
	GtkTreeView *view;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	GdkColor color;
	GObject *obj;
	time_t diff;
	char *level;

	dialog = create_dialog_ignore_settings();
	obj = G_OBJECT(dialog);

	g_signal_connect(obj, "response",
			 G_CALLBACK(event_response), ignore);

	/* level */
	setup_register_level_button(g_object_get_data(obj, "level_button"),
				    g_object_get_data(obj, "level"));

	/* pattern */
	g_signal_connect(g_object_get_data(obj, "pattern"), "changed",
			 G_CALLBACK(event_pattern_changed), obj);

	g_signal_connect(g_object_get_data(obj, "regexp"), "toggled",
			 G_CALLBACK(event_regexp_toggled), obj);

	label = g_object_get_data(obj, "regexp_invalid");
	gdk_color_parse("red", &color);
	gdk_color_alloc(gtk_widget_get_colormap(dialog), &color);
	gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &color);

	/* channels */
	store = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), 0,
					gui_tree_strcase_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
					     0, GTK_SORT_ASCENDING);
	ignore_channels_store_fill(store, ignore);

	view = g_object_get_data(obj, "channel_tree");
	g_object_set_data(G_OBJECT(view), "store", store);
	gtk_tree_view_set_model(view, GTK_TREE_MODEL(store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(view),
				    GTK_SELECTION_MULTIPLE);
        gtk_tree_view_set_headers_visible(view, FALSE);

	/* channel column */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(renderer), "edited",
			 G_CALLBACK(channel_edited), store);
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
							  "text", 0,
							  NULL);
	gtk_tree_view_append_column(view, column);

	/* channel buttons */
	g_signal_connect(g_object_get_data(obj, "channel_add"), "clicked",
			 G_CALLBACK(event_channel_add), view);
	g_signal_connect(g_object_get_data(obj, "channel_remove"), "clicked",
			 G_CALLBACK(event_channel_remove), view);

	/* expire spin buttons */
	g_signal_connect(g_object_get_data(obj, "expire_hours"), "changed",
			 G_CALLBACK(event_hours_changed), obj);
	g_signal_connect(g_object_get_data(obj, "expire_mins"), "changed",
			 G_CALLBACK(event_mins_changed), obj);

	if (ignore != NULL) {
		/* set old values */
		gui_entry_set_from(obj, "mask", ignore->mask);
		gui_toggle_set_from(obj, "exception", ignore->exception);
		gui_toggle_set_from(obj, "replies", ignore->replies);

		level = bits2level(ignore->level);
		gui_entry_set_from(obj, "level", level);
		g_free(level);

		gui_toggle_set_from(obj, "fullword", ignore->fullword);
		gui_toggle_set_from(obj, "regexp", ignore->regexp);

		/* set pattern after regexp, so the invalid state is
		   checked properly */
		gui_entry_set_from(obj, "pattern", ignore->pattern);

		if (ignore->unignore_time != 0) {
			diff = ignore->unignore_time - time(NULL);
			gui_spin_set_from(obj, "expire_days",
					  (int) (diff/3600/24));
			gui_spin_set_from(obj, "expire_hours",
					  (int) ((diff/3600)%24));
			gui_spin_set_from(obj, "expire_mins",
					  (int) ((diff/60)%60));
		}
	}

	gtk_widget_show(dialog);
}
