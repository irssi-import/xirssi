/*
 entry.c : irssi

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
#include "special-vars.h"
#include "servers.h"

#include "keyboard.h"
#include "completion.h"
#include "command-history.h"
#include "translation.h"

#include "gui-entry.h"
#include "gui-frame.h"
#include "gui-window.h"
#include "gui-window-view.h"

static gboolean event_destroy(GtkWidget *widget, Entry *entry)
{
	signal_emit("gui entry destroyed", 1, entry);

	entry->widget = NULL;
	entry->entry = NULL;

	gui_entry_unref(entry);
	return FALSE;
}

static gboolean event_activate(GtkWidget *widget, Entry *entry)
{
	key_pressed(entry->keyboard, "entry");
	return FALSE;
}

Entry *gui_entry_new(Frame *frame)
{
	Entry *entry;

	entry = g_new0(Entry, 1);
	entry->refcount = 1;

	entry->frame = frame;
	gui_frame_ref(entry->frame);

	entry->keyboard = keyboard_create(entry);

	entry->widget = gtk_entry_new();
	entry->entry = GTK_ENTRY(entry->widget);

	g_signal_connect(G_OBJECT(entry->widget), "destroy",
			 G_CALLBACK(event_destroy), entry);
	g_signal_connect(G_OBJECT(entry->widget), "activate",
			 G_CALLBACK(event_activate), entry);

	signal_emit("gui entry created", 1, entry);
	return entry;
}

void gui_entry_ref(Entry *entry)
{
	entry->refcount++;
}

int gui_entry_unref(Entry *entry)
{
	if (--entry->refcount > 0)
		return TRUE;

	keyboard_destroy(entry->keyboard);
	gui_frame_unref(entry->frame);

	g_free(entry);
	return FALSE;
}

void gui_entry_set_window(Entry *entry, Window *window)
{
	entry->active_win = window;
}

static void key_backward_history(const char *data, Entry *entry)
{
	const char *line, *text;
	int len;

	line = gtk_entry_get_text(entry->entry);
	text = command_history_prev(entry->active_win, line);
	gtk_entry_set_text(entry->entry, text);

        len = gtk_entry_get_width_chars(entry->entry);
	gtk_editable_set_position(GTK_EDITABLE(entry->entry), len);
}

static void key_forward_history(const char *data, Entry *entry)
{
	const char *line, *text;
	int len;

	line = gtk_entry_get_text(entry->entry);
	text = command_history_next(entry->active_win, line);
	gtk_entry_set_text(entry->entry, text);

        len = gtk_entry_get_width_chars(entry->entry);
	gtk_editable_set_position(GTK_EDITABLE(entry->entry), len);
}

static void key_send_line(const char *data, Entry *entry)
{
	HISTORY_REC *history;
	const char *line;
	char *str, *add_history;

	line = gtk_entry_get_text(entry->entry);
	if (line == NULL || *line == '\0')
		return;

	/* we can't use gui_entry_get_text() later, since the entry might
	   have been destroyed after we get back */
	add_history = g_strdup(line);
	history = command_history_current(entry->active_win);

	str = g_locale_from_utf8(line, -1, NULL, NULL, NULL);
	if (str == NULL) {
		/* error - fallback to utf8 */
		str = g_strdup(line);
	}
	translate_output(str);

        gui_entry_ref(entry);
	signal_emit("send command", 3, str,
		    entry->active_win->active_server,
		    entry->active_win->active);

	if (add_history != NULL) {
		history = command_history_find(history);
		if (history != NULL)
			command_history_add(history, add_history);
                g_free(add_history);
	}

	if (gui_entry_unref(entry))
		gtk_entry_set_text(entry->entry, "");
	command_history_clear_pos(entry->active_win);

        g_free(str);
}

static void key_completion(Entry *entry, int erase)
{
	const char *text;
	char *line;
	int pos;

	pos = gtk_editable_get_position(GTK_EDITABLE(entry->entry));

        text = gtk_entry_get_text(entry->entry);
	line = word_complete(entry->active_win, text, &pos, erase);

	if (line != NULL) {
		gtk_entry_set_text(entry->entry, line);
		gtk_editable_set_position(GTK_EDITABLE(entry->entry), pos);
		g_free(line);
	}
}

static void key_word_completion(const char *data, Entry *entry)
{
        key_completion(entry, FALSE);
}

static void key_erase_completion(const char *data, Entry *entry)
{
        key_completion(entry, TRUE);
}

static void key_check_replaces(const char *data, Entry *entry)
{
	const char *text;
	char *line;
	int pos;

	pos = gtk_editable_get_position(GTK_EDITABLE(entry->entry));

        text = gtk_entry_get_text(entry->entry);
	line = auto_word_complete(text, &pos);

	if (line != NULL) {
		gtk_entry_set_text(entry->entry, line);
		gtk_editable_set_position(GTK_EDITABLE(entry->entry), pos);
		g_free(line);
	}
}

static void key_command(Entry *entry, const char *command, const char *data)
{
	signal_emit(command, 3, data, entry->active_win->active_server,
		    entry->active_win->active);
}

static void key_previous_window(const char *data, Entry *entry)
{
        key_command(entry, "command window previous", "");
}

static void key_next_window(const char *data, Entry *entry)
{
        key_command(entry, "command window next", "");
}

static void key_active_window(const char *data, Entry *entry)
{
        key_command(entry, "command window goto", "active");
}

static void key_previous_window_item(void)
{
	SERVER_REC *server;
	GSList *pos;

	if (active_win->items != NULL)
		signal_emit("command window item prev", 3, "", active_win->active_server, active_win->active);
	else if (servers != NULL) {
		/* change server */
		if (active_win->active_server == NULL)
			server = servers->data;
		else {
			pos = g_slist_find(servers, active_win->active_server);
			server = pos->next != NULL ? pos->next->data : servers->data;
		}
		signal_emit("command window server", 3, server->tag, active_win->active_server, active_win->active);
	}
}

static void key_next_window_item(void)
{
	SERVER_REC *server;
	int index;

	if (active_win->items != NULL) {
		signal_emit("command window item next", 3, "",
			    active_win->active_server, active_win->active);
	} else if (servers != NULL) {
		/* change server */
		if (active_win->active_server == NULL)
			server = servers->data;
		else {
			index = g_slist_index(servers, active_win->active_server);
			server = index > 0 ? g_slist_nth(servers, index-1)->data :
				g_slist_last(servers)->data;
		}
		signal_emit("command window server", 3, server->tag,
			    active_win->active_server, active_win->active);
	}
}

enum {
	KEY_SCROLL_START,
	KEY_SCROLL_END,
	KEY_SCROLL_BACKWARD,
	KEY_SCROLL_FORWARD
};

static void key_scroll(Entry *entry, int pos)
{
	WindowGui *gui;
	GtkTextView *view;
	GtkTextIter iter;

	gui = WINDOW_GUI(entry->active_win);
	view = gui->active_view->view;

	switch (pos) {
	case KEY_SCROLL_START:
		gtk_text_buffer_get_iter_at_offset(gui->buffer, &iter, 0);
		break;
	case KEY_SCROLL_END:
		gtk_text_buffer_get_iter_at_mark(gui->buffer, &iter,
			gtk_text_buffer_get_insert(gui->buffer));
		break;
	case KEY_SCROLL_BACKWARD:
		/* FIXME */
		break;
	case KEY_SCROLL_FORWARD:
		/* FIXME */
		break;
	}
        gtk_text_view_scroll_to_iter(view, &iter, 0, 0, 0, 0);
}

static void key_scroll_start(const char *data, Entry *entry)
{
	key_scroll(entry, KEY_SCROLL_START);
}

static void key_scroll_end(const char *data, Entry *entry)
{
	key_scroll(entry, KEY_SCROLL_END);
}

static void key_scroll_backward(const char *data, Entry *entry)
{
	key_scroll(entry, KEY_SCROLL_BACKWARD);
}

static void key_scroll_forward(const char *data, Entry *entry)
{
	key_scroll(entry, KEY_SCROLL_FORWARD);
}

static void key_insert_text(const char *data, Entry *entry)
{
	char *str;
	int pos;

	str = parse_special_string(data, entry->active_win->active_server,
				   entry->active_win->active, "", NULL, 0);
	pos = gtk_editable_get_position(GTK_EDITABLE(entry->entry));
	gtk_editable_insert_text(GTK_EDITABLE(entry->entry), str, -1, &pos);
	gtk_editable_set_position(GTK_EDITABLE(entry->entry), pos);
        g_free(str);
}

static void key_change_window(const char *data, Entry *entry)
{
	key_command(entry, "command window refnum", data);
}

static void key_change_tab(const char *data, Entry *entry)
{
	gtk_notebook_set_current_page(entry->frame->notebook, atoi(data)-1);
}

void gui_entries_init(void)
{
	static char changekeys[] = "1234567890qwertyuio";
	char key[20], data[MAX_INT_STRLEN];
	int i;

	key_configure_freeze();

        /* history */
	key_bind("backward_history", "", "up", NULL, (SIGNAL_FUNC) key_backward_history);
	key_bind("forward_history", "", "down", NULL, (SIGNAL_FUNC) key_forward_history);

        /* line transmitting */
	key_bind("send_line", "Execute the input line", "return", NULL, (SIGNAL_FUNC) key_send_line);
	key_bind("word_completion", "", "tab", NULL, (SIGNAL_FUNC) key_word_completion);
	key_bind("erase_completion", "", "meta-k", NULL, (SIGNAL_FUNC) key_erase_completion);
	key_bind("check_replaces", "Check word replaces", NULL, NULL, (SIGNAL_FUNC) key_check_replaces);

        /* window managing */
	key_bind("previous_window", "Previous window", "^P", NULL, (SIGNAL_FUNC) key_previous_window);
	key_bind("next_window", "Next window", "^N", NULL, (SIGNAL_FUNC) key_next_window);
	key_bind("active_window", "Go to next window with the highest activity", "meta-a", NULL, (SIGNAL_FUNC) key_active_window);
	key_bind("next_window_item", "Next channel/query", "^X", NULL, (SIGNAL_FUNC) key_next_window_item);
	key_bind("previous_window_item", "Previous channel/query", NULL, NULL, (SIGNAL_FUNC) key_previous_window_item);

	key_bind("scroll_start", "Beginning of the window", "", NULL, (SIGNAL_FUNC) key_scroll_start);
	key_bind("scroll_end", "End of the window", "", NULL, (SIGNAL_FUNC) key_scroll_end);
	key_bind("scroll_backward", "Previous page", "prior", NULL, (SIGNAL_FUNC) key_scroll_backward);
	key_bind("scroll_backward", NULL, "meta-p", NULL, (SIGNAL_FUNC) key_scroll_backward);
	key_bind("scroll_forward", "Next page", "next", NULL, (SIGNAL_FUNC) key_scroll_forward);
	key_bind("scroll_forward", NULL, "meta-n", NULL, (SIGNAL_FUNC) key_scroll_forward);

	/* .. */
	key_bind("insert_text", "Append text to line", NULL, NULL, (SIGNAL_FUNC) key_insert_text);

        /* autoreplaces */
	key_bind("multi", NULL, "return", "check_replaces;send_line", NULL);
	key_bind("multi", NULL, "space", "check_replaces;insert_text  ", NULL);

        /* moving between windows */
	key_bind("change_window", "Change window", "", NULL, (SIGNAL_FUNC) key_change_window);
	for (i = 0; changekeys[i] != '\0'; i++) {
		g_snprintf(key, sizeof(key), "meta-%c", changekeys[i]);
		ltoa(data, i+1);
		key_bind("change_tab", "Change tab", key, data, (SIGNAL_FUNC) key_change_tab);
	}
        key_configure_thaw();
}

void gui_entries_deinit(void)
{
	key_unbind("backward_history", (SIGNAL_FUNC) key_backward_history);
	key_unbind("forward_history", (SIGNAL_FUNC) key_forward_history);

	key_unbind("send_line", (SIGNAL_FUNC) key_send_line);
	key_unbind("word_completion", (SIGNAL_FUNC) key_word_completion);
	key_unbind("erase_completion", (SIGNAL_FUNC) key_erase_completion);
	key_unbind("check_replaces", (SIGNAL_FUNC) key_check_replaces);

	key_unbind("previous_window", (SIGNAL_FUNC) key_previous_window);
	key_unbind("next_window", (SIGNAL_FUNC) key_next_window);
	key_unbind("active_window", (SIGNAL_FUNC) key_active_window);
	key_unbind("next_window_item", (SIGNAL_FUNC) key_next_window_item);
	key_unbind("previous_window_item", (SIGNAL_FUNC) key_previous_window_item);

	key_unbind("scroll_start", (SIGNAL_FUNC) key_scroll_start);
	key_unbind("scroll_end", (SIGNAL_FUNC) key_scroll_end);
	key_unbind("scroll_backward", (SIGNAL_FUNC) key_scroll_backward);
	key_unbind("scroll_backward", (SIGNAL_FUNC) key_scroll_backward);
	key_unbind("scroll_forward", (SIGNAL_FUNC) key_scroll_forward);
	key_unbind("scroll_forward", (SIGNAL_FUNC) key_scroll_forward);

	key_unbind("insert_text", (SIGNAL_FUNC) key_insert_text);

	key_unbind("change_window", (SIGNAL_FUNC) key_change_window);
	key_unbind("change_tab", (SIGNAL_FUNC) key_change_tab);
}
