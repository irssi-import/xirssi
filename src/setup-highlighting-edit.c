/*
 setup-highlighting-edit.c : irssi

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

#include "hilight-text.h"

#include "setup.h"
#include "gui.h"
#include "glade/interface.h"

static void highlight_save(GObject *obj, Highlight *highlight)
{
	GtkEntry *entry;
	GtkToggleButton *toggle;
	GtkSpinButton *spin;
	GtkTreeModel *model;
	char *channels;

	if (highlight == NULL) {
		highlight = g_new0(Highlight, 1);
	} else {
		g_strfreev(highlight->channels);
	}

	/* text */
	gui_entry_update(obj, "text", &highlight->text);

	/* level */
	entry = g_object_get_data(obj, "level");
	highlight->level = level2bits(gtk_entry_get_text(entry), NULL);
	if (highlight->level == 0)
		highlight->level = MSGLEVEL_ALL;

	/* priority */
	spin = g_object_get_data(obj, "priority");
	highlight->priority = gtk_spin_button_get_value(spin);

	/* match flags */
	toggle = g_object_get_data(obj, "fullword");
	highlight->fullword = gtk_toggle_button_get_active(toggle);

	toggle = g_object_get_data(obj, "regexp");
	highlight->regexp = gtk_toggle_button_get_active(toggle);

	toggle = g_object_get_data(obj, "nickmask");
	highlight->nickmask = gtk_toggle_button_get_active(toggle);

	/* hilight flags */
	toggle = g_object_get_data(obj, "word");
	highlight->word = gtk_toggle_button_get_active(toggle);

	toggle = g_object_get_data(obj, "nick");
	highlight->nick = gtk_toggle_button_get_active(toggle);

	/* get channels */
	model = g_object_get_data(g_object_get_data(obj, "channel_tree"),
				  "store");
	channels = gui_tree_model_get_string(model, 0, " ");
	highlight->channels = channels == NULL ? NULL :
		g_strsplit(channels, " ", -1);
	g_free(channels);

	/* colors */
	gui_entry_update(obj, "color", &highlight->color);
	gui_entry_update(obj, "act_color", &highlight->act_color);

	hilight_create(highlight);
}

static gboolean event_response(GtkWidget *widget, int response_id,
			       Highlight *highlight)
{
	/* ok / cancel pressed */
	if (response_id == GTK_RESPONSE_OK)
		highlight_save(G_OBJECT(widget), highlight);

	gtk_widget_destroy(widget);
	return FALSE;
}

static void highlight_channels_store_fill(GtkListStore *store, Highlight *highlight)
{
	GtkTreeIter iter;
	char **tmp;

	if (highlight->channels == NULL)
		return;

	for (tmp = highlight->channels; *tmp != NULL; tmp++) {
		gtk_list_store_prepend(store, &iter);
		gtk_list_store_set(store, &iter, 0, *tmp, -1);
	}
}

static void regexp_update_invalid(GtkToggleButton *toggle, GtkEntry *entry,
				  GtkWidget *label)
{
#ifdef USE_GREGEX
        GRegex *preg;
	GError *re_error;
#else
        regex_t preg;
	int regexp_compiled;
#endif
	const char *text;

	if (!gtk_toggle_button_get_active(toggle)) {
		/* not a regexp */
		gtk_widget_hide(label);
		return;
	}

	text = gtk_entry_get_text(entry);
#ifdef USE_GREGEX
	re_error = NULL;
        preg = g_regex_new(text, G_REGEX_OPTIMIZE | G_REGEX_RAW | G_REGEX_CASELESS, 0, &re_error);
	g_error_free(re_error);

	if (preg != NULL) {
		/* regexp ok */
                g_regex_unref(preg);

		gtk_widget_hide(label);
	} else {
		/* invalid regexp - show the error label */
		gtk_widget_show(label);
	}

#else
	if (regcomp(&preg, text, REG_EXTENDED|REG_ICASE|REG_NOSUB) == 0) {
		/* regexp ok */
                regfree(&preg);

		gtk_widget_hide(label);
	} else {
		/* invalid regexp - show the error label */
		gtk_widget_show(label);
	}
#endif
}

static gboolean event_text_changed(GtkEntry *entry, GObject *obj)
{
	regexp_update_invalid(g_object_get_data(obj, "regexp"), entry,
			      g_object_get_data(obj, "regexp_invalid"));
	return FALSE;
}

static gboolean event_regexp_toggled(GtkToggleButton *toggle, GObject *obj)
{
	regexp_update_invalid(toggle, g_object_get_data(obj, "text"),
			      g_object_get_data(obj, "regexp_invalid"));
	return FALSE;
}

void highlight_dialog_show(Highlight *highlight)
{
	GtkWidget *dialog, *label;
	GtkListStore *store;
	GdkColor color;
	GObject *obj;
	char *level;

	dialog = create_dialog_highlight_settings();
	obj = G_OBJECT(dialog);

	g_signal_connect(obj, "response",
			 G_CALLBACK(event_response), highlight);

	/* text */
	g_signal_connect(g_object_get_data(obj, "text"), "changed",
			 G_CALLBACK(event_text_changed), obj);

	g_signal_connect(g_object_get_data(obj, "regexp"), "toggled",
			 G_CALLBACK(event_regexp_toggled), obj);

	label = g_object_get_data(obj, "regexp_invalid");
	gdk_color_parse("red", &color);
	gdk_color_alloc(gtk_widget_get_colormap(dialog), &color);
	gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &color);

	/* level */
	setup_register_level_button(g_object_get_data(obj, "level_button"),
				    g_object_get_data(obj, "level"));

	/* channels */
	store = setup_channels_list_init(g_object_get_data(obj, "channel_tree"),
					 g_object_get_data(obj, "channel_add"),
					 g_object_get_data(obj, "channel_remove"));

	/* color change buttons */
	setup_register_irssicolor_button(g_object_get_data(obj, "color_button"),
                                         g_object_get_data(obj, "color"));
	setup_register_irssicolor_button(g_object_get_data(obj, "act_color_button"),
                                         g_object_get_data(obj, "act_color"));

	if (highlight != NULL) {
		/* set old values */
		level = bits2level(highlight->level);
		gui_entry_set_from(obj, "level", level);
		g_free(level);

		gui_spin_set_from(obj, "priority", highlight->priority);

		gui_toggle_set_from(obj, "fullword", highlight->fullword);
		gui_toggle_set_from(obj, "regexp", highlight->regexp);
		gui_toggle_set_from(obj, "nickmask", highlight->nickmask);

		/* set text after regexp, so the invalid state is
		   checked properly */
		gui_entry_set_from(obj, "text", highlight->text);

		gui_toggle_set_from(obj, "word", highlight->word);
		gui_toggle_set_from(obj, "nick", highlight->nick);

		highlight_channels_store_fill(store, highlight);

		gui_entry_set_from(obj, "color", highlight->color);
		gui_entry_set_from(obj, "act_color", highlight->act_color);
	}

	gtk_widget_show(dialog);
}
