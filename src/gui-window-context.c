/*
 gui-window-context.c : irssi

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

#include "formats.h"

#include "gui-window.h"
#include "gui-window-view.h"
#include "gui-window-context.h"

typedef struct {
	Window *window;

	int tag_event;
	GtkTextTag *tag;
	int mouse_offset;
	int start_offset;
	char *word;
} ContextEvent;

static void get_tag_start(GtkTextIter *start_iter, const GtkTextIter *iter,
			  GtkTextTag *tag)
{
	memcpy(start_iter, iter, sizeof(GtkTextIter));

	/* go to beginning of the tag */
	while (gtk_text_iter_backward_char(start_iter)) {
		if (!gtk_text_iter_has_tag(start_iter, tag)) {
			gtk_text_iter_forward_char(start_iter);
			break;
		}
	}
}

static void get_tag_end(GtkTextIter *end_iter, const GtkTextIter *iter,
			GtkTextTag *tag)
{
	memcpy(end_iter, iter, sizeof(GtkTextIter));

	/* go to end of tag */
	while (gtk_text_iter_forward_char(end_iter)) {
		if (!gtk_text_iter_has_tag(end_iter, tag))
			break;
	}
}

static char *get_tag_word(GtkTextIter *start_iter, const GtkTextIter *iter,
			  GtkTextTag *tag)
{
	GtkTextIter end_iter;

	/* get the text in tag */
	get_tag_end(&end_iter, iter, tag);
	return gtk_text_buffer_get_text(gtk_text_iter_get_buffer(iter),
					start_iter, &end_iter, TRUE);
}

static void mark_tag_select(const GtkTextIter *iter, GtkTextTag *tag)
{
	GtkTextBuffer *buffer;
	GtkTextMark *select, *insert;
	GtkTextIter start_iter, end_iter;

	buffer = gtk_text_iter_get_buffer(iter);
	select = gtk_text_buffer_get_selection_bound(buffer);
	insert = gtk_text_buffer_get_insert(buffer);

	get_tag_start(&start_iter, iter, tag);
	get_tag_end(&end_iter, iter, tag);

	gtk_text_buffer_move_mark(buffer, select, &start_iter);
	gtk_text_buffer_move_mark(buffer, insert, &end_iter);
}

static gboolean event_tag(GtkTextTag *tag, GtkWidget *widget,
			  GdkEventButton *event, const GtkTextIter *iter)
{
	GtkTextIter start_iter;
        ContextEvent *context;
	int offset, moved;

	context = g_object_get_data(G_OBJECT(widget), "context");
	context->tag_event = TRUE;

	if (event->type != GDK_MOTION_NOTIFY &&
	    event->type != GDK_BUTTON_PRESS &&
            event->type != GDK_BUTTON_RELEASE &&
	    event->type != GDK_2BUTTON_PRESS)
		return FALSE;

	/* a) motion, b) mouse button press */
	moved = FALSE;
	offset = gtk_text_iter_get_offset(iter);
	if (context->mouse_offset != offset) {
		/* moved - get the start offset */
		get_tag_start(&start_iter, iter, tag);
		context->mouse_offset = offset;

		offset = gtk_text_iter_get_offset(&start_iter);
		if (context->start_offset != offset) {
			/* nope, we've moved, get the word at the tag */
			context->start_offset = offset;
			moved = TRUE;

			if (context->tag != NULL) {
				/* moved directly from tag to another,
				   send the leave signal for the old one */
				signal_emit("gui window context leave", 3,
					    context->window, context->word,
					    context->tag);
			}

			g_free(context->word);
			context->word = get_tag_word(&start_iter, iter, tag);
		}

	}

	if (moved || context->tag != tag) {
		context->tag = tag;
		signal_emit("gui window context enter", 4,
			    context->window, context->word, tag, widget);
	}

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		signal_emit("gui window context press", 5, context->window,
			    context->word, tag, event, widget);
		break;
	case GDK_BUTTON_RELEASE:
		signal_emit("gui window context release", 5, context->window,
			    context->word, tag, event, widget);
		break;
	case GDK_2BUTTON_PRESS:
		/* doubleclicked tag, set the selection to cover the tag
		   entirely - FIXME: the end selection won't stay there.. */
		mark_tag_select(iter, tag);
		return TRUE;
	default:
	}

	return FALSE;
}

GtkTextTag *gui_window_context_create_tag(WindowGui *window, const char *name)
{
	GtkTextTag *tag;

	tag = gtk_text_buffer_create_tag(window->buffer, name, NULL);
	g_signal_connect_after(G_OBJECT(tag), "event", G_CALLBACK(event_tag), NULL);
	return tag;
}

void gui_window_print_mark_context(WindowGui *window, TextDest *dest,
				   GtkTextIter *iter, const char *text)
{
	GtkTextIter start_iter, end_iter;
	GtkTextTag *tag;
	Channel *channel;
	const char *start;
	char *word;
	int len, line, index;

	channel = dest == NULL || dest->server == NULL ||
		dest->target == NULL ? NULL :
		channel_find(dest->server, dest->target);

	/* mark context words if there's any */
	line = gtk_text_iter_get_line(iter);
	index = gtk_text_iter_get_line_index(iter);
	for (start = text; ; text++) {
		if (*text != '\0' && *text != '\t' && *text != ' ')
			continue;

		if (text == start)
			len = 0;
		else {
			/* got a word */
			word = g_strndup(start, (int) (text-start));
			len = strlen(word);

			tag = NULL;
			signal_emit("gui window context word", 4,
				    &tag, window, channel, word);
			g_free(word);

			if (tag != NULL) {
				/* apply the context tag */
				gtk_text_buffer_get_iter_at_line_index(window->buffer, &start_iter, line, index);
				gtk_text_buffer_get_iter_at_line_index(window->buffer, &end_iter, line, index+len);
				gtk_text_buffer_apply_tag(window->buffer, tag,
							  &start_iter, &end_iter);
			}
		}

		if (*text == '\0')
			break;
		start = text+1;
		index += len+1;
	}
}

static void window_context_leave(WindowView *view, ContextEvent *context)
{
	GdkWindow *window;

	view->cursor_link = FALSE;
	window = gtk_text_view_get_window(view->view,
					  GTK_TEXT_WINDOW_TEXT);
	gdk_window_set_cursor(window, NULL);

	signal_emit("gui window context leave", 3,
		    context->window, context->word, context->tag);
	context->tag = NULL;
}

gboolean gui_window_context_event_motion(GtkWidget *widget, GdkEvent *event,
					 WindowView *view)
{
        ContextEvent *context;
	GdkWindow *window;

	/* needed to call this so the motion events won't get stuck */
	gdk_window_get_pointer(widget->window, NULL, NULL, NULL);

	/* GTK has already called the tag's event handler,
	   so we just need to see if it was called */
	context = g_object_get_data(G_OBJECT(widget), "context");
	if (context->tag_event) {
		/* mouse is over a tag */
		context->tag_event = FALSE;
		if (!view->cursor_link) {
			view->cursor_link = TRUE;

			window = gtk_text_view_get_window(view->view,
							  GTK_TEXT_WINDOW_TEXT);
			gdk_window_set_cursor(window, view->hand_cursor);
		}
	} else if (view->cursor_link) {
		/* moved out of tag */
		window_context_leave(view, context);
	}

	return FALSE;
}

static void sig_gui_window_view_created(WindowView *view)
{
        ContextEvent *context;

	context = g_new0(ContextEvent, 1);
	context->window = view->window->window;
	g_object_set_data(G_OBJECT(view->view), "context", context);
}

static void sig_gui_window_view_destroyed(WindowView *view)
{
        ContextEvent *context;

	context = g_object_get_data(G_OBJECT(view->view), "context");
	g_free(context->word);
	g_free(context);
}

void gui_window_contexts_init(void)
{
	signal_add("gui window view created", (SIGNAL_FUNC) sig_gui_window_view_created);
	signal_add("gui window view destroyed", (SIGNAL_FUNC) sig_gui_window_view_destroyed);
}

void gui_window_contexts_deinit(void)
{
	signal_remove("gui window view created", (SIGNAL_FUNC) sig_gui_window_view_created);
	signal_remove("gui window view destroyed", (SIGNAL_FUNC) sig_gui_window_view_destroyed);
}
