/*
 gui-keyboard.c : irssi

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

char *gui_keyboard_get_event_string(GdkEventKey *event)
{
	GString *cmd;
	char *ret;
	int chr, len;

	chr = event->keyval;

	/* get alt/ctrl/shift masks */
	cmd = g_string_new(NULL);
	if (event->state & GDK_SHIFT_MASK) {
		if (chr > 255)
			g_string_prepend(cmd, "shift-");
		else
			chr = i_toupper(chr);
	}

	if (event->state & GDK_MOD1_MASK)
		g_string_prepend(cmd, "meta-");
	if (event->state & GDK_MOD2_MASK)
		g_string_prepend(cmd, "mod2-");
	if (event->state & GDK_MOD3_MASK)
		g_string_prepend(cmd, "mod3-");
	if (event->state & GDK_MOD4_MASK)
		g_string_prepend(cmd, "mod4-");
	if (event->state & GDK_MOD5_MASK)
		g_string_prepend(cmd, "mod5-");

	if (event->state & GDK_CONTROL_MASK) {
		if (chr > 255)
			g_string_prepend(cmd, "ctrl-");
		else {
			g_string_prepend(cmd, "^");
			chr = i_toupper(chr);
		}
	}

	/* get key name */
	if (event->keyval > 255) {
		len = cmd->len;
		g_string_append(cmd, gdk_keyval_name(event->keyval));
		g_strdown(cmd->str+len);
	} else if (event->keyval == ' ')
		g_string_append(cmd, "space");
	else if (event->keyval == '\n')
		g_string_append(cmd, "return");
	else
		g_string_sprintfa(cmd, "%c", chr);

	ret = cmd->str;
	g_string_free(cmd, FALSE);
	return ret;
}

static int gtk_entry_move_forward_to_space(GtkEntry *entry)
{
	PangoLayout *layout = gtk_entry_get_layout(entry);
	PangoLogAttr *log_attrs;
	int pos, n_attrs;

	pos = gtk_editable_get_position(GTK_EDITABLE(entry));
	if (pos == entry->text_length)
		return pos;

	pango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);

	while (pos < n_attrs && log_attrs[pos].is_white)
		pos++;
	while (pos < n_attrs && !log_attrs[pos].is_white)
		pos++;

	g_free(log_attrs);
	return pos;
}

static int gtk_entry_move_backward_to_space(GtkEntry *entry)
{
	PangoLayout *layout = gtk_entry_get_layout(entry);
	PangoLogAttr *log_attrs;
	int pos, n_attrs;

	pos = gtk_editable_get_position(GTK_EDITABLE(entry));
	if (pos == 0)
		return pos;

	pango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);

	while (pos > 0 && log_attrs[pos-1].is_white)
		pos--;
	while (pos > 0 && !log_attrs[pos-1].is_white)
		pos--;

	g_free(log_attrs);
	return pos;
}

static void key_backward_character(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "move_cursor",
			      GTK_MOVEMENT_VISUAL_POSITIONS, -1, FALSE);
}

static void key_forward_character(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "move_cursor",
			      GTK_MOVEMENT_VISUAL_POSITIONS, 1, FALSE);
}

static void key_backward_word(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "move_cursor",
			      GTK_MOVEMENT_WORDS, -1, FALSE);
}

static void key_forward_word(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "move_cursor",
			      GTK_MOVEMENT_WORDS, 1, FALSE);
}

static void key_backward_to_space(const char *data, Entry *entry)
{
	int pos;

	pos = gtk_entry_move_backward_to_space(entry->entry);
	gtk_editable_set_position(GTK_EDITABLE(entry->entry), pos);
}

static void key_forward_to_space(const char *data, Entry *entry)
{
	int pos;

	pos = gtk_entry_move_forward_to_space(entry->entry);
	gtk_editable_set_position(GTK_EDITABLE(entry->entry), pos);
}

static void key_beginning_of_line(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "move_cursor",
			      GTK_MOVEMENT_DISPLAY_LINE_ENDS, -1, FALSE);
}

static void key_end_of_line(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "move_cursor",
			      GTK_MOVEMENT_DISPLAY_LINE_ENDS, 1, FALSE);
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

static void key_backspace(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "delete_from_cursor",
			      GTK_DELETE_CHARS, -1);
}

static void key_delete_character(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "delete_from_cursor",
			      GTK_DELETE_CHARS, 1);
}

static void key_delete_next_word(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "delete_from_cursor",
			      GTK_DELETE_WORD_ENDS, 1);
}

static void key_delete_previous_word(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "delete_from_cursor",
			      GTK_DELETE_WORD_ENDS, -1);
}

static void key_delete_to_previous_space(const char *data, Entry *entry)
{
	int start, end;

	end = gtk_editable_get_position(GTK_EDITABLE(entry->entry));
	start = gtk_entry_move_backward_to_space(entry->entry);
	if (start != end) {
		gtk_editable_delete_text(GTK_EDITABLE(entry->entry),
					 start, end);
	}
}

static void key_delete_to_next_space(const char *data, Entry *entry)
{
	int start, end;

	start = gtk_editable_get_position(GTK_EDITABLE(entry->entry));
	end = gtk_entry_move_forward_to_space(entry->entry);
	if (start != end) {
		gtk_editable_delete_text(GTK_EDITABLE(entry->entry),
					 start, end);
	}
}

static void key_erase_line(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "delete_from_cursor",
			      GTK_DELETE_DISPLAY_LINES, -1);
}

static void key_erase_to_beg_of_line(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "delete_from_cursor",
			      GTK_DELETE_DISPLAY_LINE_ENDS, -1);
}

static void key_erase_to_end_of_line(const char *data, Entry *entry)
{
	g_signal_emit_by_name(G_OBJECT(entry->entry), "delete_from_cursor",
			      GTK_DELETE_DISPLAY_LINE_ENDS, 1);
}

static void key_yank_from_cutbuffer(const char *data, Entry *entry)
{
	/* FIXME: */
}

static void key_transpose_characters(const char *data, Entry *entry)
{
	char *chars;
	int pos;

	pos = gtk_editable_get_position(GTK_EDITABLE(entry->entry));
	if (pos > 0 && entry->entry->text_length > 1) {
		if (pos == entry->entry->text_length)
			pos--;

		chars = gtk_editable_get_chars(GTK_EDITABLE(entry->entry),
					       pos, pos+1);
		gtk_editable_delete_text(GTK_EDITABLE(entry->entry),
					 pos, pos+1);
		pos--;
		gtk_editable_insert_text(GTK_EDITABLE(entry->entry),
					 chars, -1, &pos);
		g_free(chars);
	}
}

static void key_send_line(const char *data, Entry *entry)
{
	HISTORY_REC *history;
	const char *line;
	char *str, *next, *add_history;

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

	gtk_widget_ref(entry->widget);

	/* str may contain \r or \n chars, split the line from there */
	do {
		for (next = str; ; next++) {
			if (*next == '\0' || *next == '\r' || *next == '\n')
				break;
		}

		if (*next == '\0')
			next = NULL;
		else
			*next++ = '\0';

		signal_emit("send command", 3, str,
			    entry->active_win->active_server,
			    entry->active_win->active);

		str = next;
	} while (str != NULL);

	if (add_history != NULL) {
		history = command_history_find(history);
		if (history != NULL)
			command_history_add(history, add_history);
                g_free(add_history);
	}

	gtk_entry_set_text(entry->entry, "");
	gtk_widget_unref(entry->widget);

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
	Frame *frame;

	frame = gui_widget_find_data(entry->widget, "Frame");
	gtk_notebook_set_current_page(frame->notebook, atoi(data)-1);
}

void gui_keyboards_init(void)
{
	static char changekeys[] = "1234567890qwertyuio";
	char key[20], data[MAX_INT_STRLEN];
	int i;

	key_configure_freeze();

	/* cursor movement */
	key_bind("backward_character", "", "left", NULL, (SIGNAL_FUNC) key_backward_character);
	key_bind("forward_character", "", "right", NULL, (SIGNAL_FUNC) key_forward_character);
 	key_bind("backward_word", "", "cleft", NULL, (SIGNAL_FUNC) key_backward_word);
 	key_bind("backward_word", NULL, "meta-b", NULL, (SIGNAL_FUNC) key_backward_word);
	key_bind("forward_word", "", "cright", NULL, (SIGNAL_FUNC) key_forward_word);
	key_bind("forward_word", NULL, "meta-f", NULL, (SIGNAL_FUNC) key_forward_word);
 	key_bind("backward_to_space", "", NULL, NULL, (SIGNAL_FUNC) key_backward_to_space);
	key_bind("forward_to_space", "", NULL, NULL, (SIGNAL_FUNC) key_forward_to_space);
	key_bind("beginning_of_line", "", "home", NULL, (SIGNAL_FUNC) key_beginning_of_line);
	key_bind("beginning_of_line", NULL, "^A", NULL, (SIGNAL_FUNC) key_beginning_of_line);
	key_bind("end_of_line", "", "end", NULL, (SIGNAL_FUNC) key_end_of_line);
	key_bind("end_of_line", NULL, "^E", NULL, (SIGNAL_FUNC) key_end_of_line);

        /* history */
	key_bind("backward_history", "", "up", NULL, (SIGNAL_FUNC) key_backward_history);
	key_bind("forward_history", "", "down", NULL, (SIGNAL_FUNC) key_forward_history);

        /* line editing */
	key_bind("backspace", "", "backspace", NULL, (SIGNAL_FUNC) key_backspace);
	key_bind("delete_character", "", "delete", NULL, (SIGNAL_FUNC) key_delete_character);
	key_bind("delete_character", NULL, "^D", NULL, (SIGNAL_FUNC) key_delete_character);
	key_bind("delete_next_word", "", NULL, NULL, (SIGNAL_FUNC) key_delete_next_word);
	key_bind("delete_previous_word", "meta-backspace", NULL, NULL, (SIGNAL_FUNC) key_delete_previous_word);
	key_bind("delete_to_previous_space", "", "^W", NULL, (SIGNAL_FUNC) key_delete_to_previous_space);
	key_bind("delete_to_next_space", "", "", NULL, (SIGNAL_FUNC) key_delete_to_next_space);
	key_bind("erase_line", "", "^U", NULL, (SIGNAL_FUNC) key_erase_line);
	key_bind("erase_to_beg_of_line", "", NULL, NULL, (SIGNAL_FUNC) key_erase_to_beg_of_line);
	key_bind("erase_to_end_of_line", "", "^K", NULL, (SIGNAL_FUNC) key_erase_to_end_of_line);
	key_bind("yank_from_cutbuffer", "", "^Y", NULL, (SIGNAL_FUNC) key_yank_from_cutbuffer);
	key_bind("transpose_characters", "Swap current and previous character", "^T", NULL, (SIGNAL_FUNC) key_transpose_characters);

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

void gui_keyboards_deinit(void)
{
	key_unbind("backward_character", (SIGNAL_FUNC) key_backward_character);
	key_unbind("forward_character", (SIGNAL_FUNC) key_forward_character);
 	key_unbind("backward_word", (SIGNAL_FUNC) key_backward_word);
	key_unbind("forward_word", (SIGNAL_FUNC) key_forward_word);
 	key_unbind("backward_to_space", (SIGNAL_FUNC) key_backward_to_space);
	key_unbind("forward_to_space", (SIGNAL_FUNC) key_forward_to_space);
	key_unbind("beginning_of_line", (SIGNAL_FUNC) key_beginning_of_line);
	key_unbind("end_of_line", (SIGNAL_FUNC) key_end_of_line);

	key_unbind("backward_history", (SIGNAL_FUNC) key_backward_history);
	key_unbind("forward_history", (SIGNAL_FUNC) key_forward_history);

	key_unbind("backspace", (SIGNAL_FUNC) key_backspace);
	key_unbind("delete_character", (SIGNAL_FUNC) key_delete_character);
	key_unbind("delete_next_word", (SIGNAL_FUNC) key_delete_next_word);
	key_unbind("delete_previous_word", (SIGNAL_FUNC) key_delete_previous_word);
	key_unbind("delete_to_next_space", (SIGNAL_FUNC) key_delete_to_next_space);
	key_unbind("delete_to_previous_space", (SIGNAL_FUNC) key_delete_to_previous_space);
	key_unbind("erase_line", (SIGNAL_FUNC) key_erase_line);
	key_unbind("erase_to_beg_of_line", (SIGNAL_FUNC) key_erase_to_beg_of_line);
	key_unbind("erase_to_end_of_line", (SIGNAL_FUNC) key_erase_to_end_of_line);
	key_unbind("yank_from_cutbuffer", (SIGNAL_FUNC) key_yank_from_cutbuffer);
	key_unbind("transpose_characters", (SIGNAL_FUNC) key_transpose_characters);

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
