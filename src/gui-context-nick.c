/*
 gui-context-nick.c : irssi

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
#include "servers.h"
#include "channels.h"
#include "nicklist.h"

#include "gui-frame.h"
#include "gui-tab.h"
#include "gui-window.h"
#include "gui-window-view.h"
#include "gui-window-context.h"
#include "gui-nicklist-view.h"
#include "gui-menu.h"

#define STATUSBAR_CONTEXT "context nick"

static void statusbar_push_nick(GtkStatusbar *statusbar, Nick *nick)
{
	unsigned int id;
	GString *str;
	char *text;

	id = gtk_statusbar_get_context_id(statusbar, STATUSBAR_CONTEXT);

	str = g_string_new(NULL);
	g_string_sprintfa(str, "Nick: %s", nick->nick);

	if (nick->host != NULL)
		g_string_sprintfa(str, "  -  Host: %s", nick->host);
	if (nick->realname != NULL)
		g_string_sprintfa(str, "  -  Name: %s", nick->realname);

	/* this really should be done separately to all the nick->* values,
	   at least when we add i18n support to strings above.. */
	text = g_locale_to_utf8(str->str, -1, NULL, NULL, NULL);
	if (text == NULL) {
		text = g_convert(str->str, str->len, "UTF-8", "CP1252",
				 NULL, NULL, NULL);
	}

	gtk_statusbar_pop(statusbar, id);
	gtk_statusbar_push(statusbar, id, text != NULL ? text : str->str);

	g_free(text);
	g_string_free(str, TRUE);
}

static void statusbar_pop_nick(GtkStatusbar *statusbar)
{
	unsigned int id;

	/* don't bother to check if we haven't pushed it */
	id = gtk_statusbar_get_context_id(statusbar, STATUSBAR_CONTEXT);
	gtk_statusbar_pop(statusbar, id);
}

static Server *tag_get_server(GtkTextTag *tag)
{
	/* "nick <server tag>" */
	if (strncmp(tag->name, "nick ", 5) != 0)
		return NULL;

	return server_find_tag(tag->name+5);
}

static Nick *nick_find_first(Server *server, const char *nick)
{
	GSList *tmp;
	Nick *nickrec;

	for (tmp = server->channels; tmp != NULL; tmp = tmp->next) {
		Channel *channel = tmp->data;

		nickrec = nicklist_find(channel, nick);
		if (nickrec != NULL && nickrec->host != NULL)
			return nickrec;
	}

	return NULL;
}

static void sig_window_word(GtkTextTag **tag, WindowGui *window,
			    Channel *channel, const char *word)
{
	char *name;

	if (channel == NULL) {
		channel = CHANNEL(window->window->active);
		if (channel == NULL)
			return;
	}

	if (nicklist_find(channel, word) == NULL)
		return;

	/* yeah, it's a nick */
	name = g_strconcat("nick ", channel->server->tag, NULL);
	*tag = gtk_text_tag_table_lookup(window->tagtable, name);
	if (*tag == NULL)
		*tag = gui_window_context_create_tag(window, name);
	g_free(name);

	signal_stop();
}

static void sig_window_enter(Window *window, const char *word, GtkTextTag *tag)
{
	Server *server;
	Nick *nick;

	server = tag_get_server(tag);
	if (server != NULL) {
		nick = nick_find_first(server, word);
		if (nick != NULL)
			statusbar_push_nick(active_frame->statusbar, nick);
	}
}

static void sig_window_leave(Window *window, const char *word, GtkTextTag *tag)
{
	statusbar_pop_nick(active_frame->statusbar);
}

typedef struct {
	Server *server;
	Channel *channel;
	char *nick;
} NickMenuData;

static const char *get_nicks(Server **server, Channel **channel,
			     void *user_data)
{
	NickMenuData *data = user_data;

	if (server == NULL) {
		/* destroy */
                server_unref(data->server);
		g_free(data->nick);
		g_free(data);
		return NULL;
	}

	if (data->server->disconnected)
		return NULL;

	*server = data->server;
	*channel = g_slist_find(data->server->channels, data->channel) != NULL ?
		data->channel : NULL;
	return data->nick;
}

static void sig_window_press(Window *window, const char *word,
			     GtkTextTag *tag, GdkEventButton *event)
{
	NickMenuData *data;
	Server *server;

	if (event->button != 3)
		return;

	server = tag_get_server(tag);
	if (server == NULL)
		return;

	data = g_new(NickMenuData, 1);
	data->server = server;
	data->channel = CHANNEL(active_win->active);
	data->nick = g_strdup(word);
	server_ref(server);

	gui_menu_nick_popup(get_nicks, data, event->button);
}

static void sig_nicklist_enter(NicklistView *view, Nick *nick)
{
	statusbar_push_nick(view->tab->frame->statusbar, nick);
}

static void sig_nicklist_leave(NicklistView *view, Nick *nick)
{
	statusbar_pop_nick(view->tab->frame->statusbar);
}

void gui_context_nick_init(void)
{
        signal_add("gui window context word", (SIGNAL_FUNC) sig_window_word);
        signal_add("gui window context enter", (SIGNAL_FUNC) sig_window_enter);
        signal_add("gui window context leave", (SIGNAL_FUNC) sig_window_leave);
	signal_add("gui window context press", (SIGNAL_FUNC) sig_window_press);
	signal_add("gui nicklist enter", (SIGNAL_FUNC) sig_nicklist_enter);
	signal_add("gui nicklist leave", (SIGNAL_FUNC) sig_nicklist_leave);
}

void gui_context_nick_deinit(void)
{
        signal_remove("gui window context word", (SIGNAL_FUNC) sig_window_word);
        signal_remove("gui window context enter", (SIGNAL_FUNC) sig_window_enter);
        signal_remove("gui window context leave", (SIGNAL_FUNC) sig_window_leave);
	signal_remove("gui window context press", (SIGNAL_FUNC) sig_window_press);
	signal_remove("gui nicklist enter", (SIGNAL_FUNC) sig_nicklist_enter);
	signal_remove("gui nicklist leave", (SIGNAL_FUNC) sig_nicklist_leave);
}
