/*
 gui-context-url.c : irssi

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

#include "gui-window.h"
#include "gui-window-context.h"

static void sig_window_word(GtkTextTag **tag, WindowGui *window,
			    Channel *channel, const char *word)
{
	/* common urls */
	if (strncmp(word, "http://", 7) == 0 || strncmp(word, "www.", 4) == 0 ||
	    strncmp(word, "ftp://", 6) == 0 || strncmp(word, "ftp.", 4) == 0 ||
	    strncmp(word, "irc://", 6) == 0 || strncmp(word, "mailto:", 7) == 0) {
		*tag = gtk_text_tag_table_lookup(window->tagtable, "url");
		if (*tag == NULL)
			*tag = gui_window_context_create_tag(window, "url");
		signal_stop();
	}
}

static void sig_window_press(Window *window, const char *word, GtkTextTag *tag,
			     GdkEventButton *event)
{
	if (event->button != 3)
		return;

}

void gui_context_url_init(void)
{
        signal_add("gui window context word", (SIGNAL_FUNC) sig_window_word);
	signal_add("gui window context press", (SIGNAL_FUNC) sig_window_press);
}

void gui_context_url_deinit(void)
{
        signal_remove("gui window context word", (SIGNAL_FUNC) sig_window_word);
        signal_remove("gui window context press", (SIGNAL_FUNC) sig_window_press);
}
