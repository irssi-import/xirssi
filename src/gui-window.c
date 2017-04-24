/*
 gui-window.c : irssi

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
#include "settings.h"

#include "printtext.h"

#include "gui-colors.h"
#include "gui-frame.h"
#include "gui-tab.h"
#include "gui-window.h"
#include "gui-window-view.h"
#include "gui-window-context.h"

void gui_window_activities_init(void);
void gui_window_activities_deinit(void);

static int window_create_override;

void gui_window_add_view(WindowGui *window, Tab *tab)
{
	WindowView *view;

	view = gui_window_view_new(gui_tab_pane_new(tab),
				   window, window->buffer);

	window->views = g_slist_prepend(window->views, view);
	window->active_view = view;

	gui_tab_set_active_window(tab, window->window);
}

void gui_window_remove_view(WindowView *view)
{
	WindowGui *window = view->window;

	window->views = g_slist_remove(window->views, view);

	window->active_view = window->views != NULL ?
		window->views->data : NULL;
	if (window->views == NULL)
		window_destroy(window->window);
}

void gui_window_update_width(WindowGui *window)
{
	GSList *tmp;
	int width, height;

	width = height = -1;
	for (tmp = window->views; tmp != NULL; tmp = tmp->next) {
		WindowView *view = tmp->data;

		if (width == -1 || width > view->approx_width)
			width = view->approx_width;
		if (height == -1 || height > view->approx_height)
			height = view->approx_height;
	}

	window->window->width = width;
	window->window->height = height;
}

static int tab_has_window(Tab *tab, Window *window)
{
	GList *tmp;

	for (tmp = tab->panes; tmp != NULL; tmp = tmp->next) {
		TabPane *pane = tmp->data;

		if (pane->view != NULL &&
		    pane->view->window->window == window)
			return TRUE;
	}

	return FALSE;
}

int gui_window_is_visible(Window *window)
{
	GSList *tmp;

	for (tmp = frames; tmp != NULL; tmp = tmp->next) {
		Frame *frame = tmp->data;

		if (tab_has_window(frame->active_tab, window))
			return TRUE;
	}

	return FALSE;
}

static void gui_window_print(WindowGui *window, TextDest *dest,
			     const char *text, int fg, int bg, int flags)
{
	GtkTextTag *tag;
	GtkTextIter iter, start_iter;
	GdkColor *colortab;
	char *utf8_text, fg_tag_name[20], bg_tag_name[20];
	int start_offset;

	memset(&iter, 0, sizeof(iter));
	memset(&start_iter, 0, sizeof(start_iter));

	utf8_text = g_locale_to_utf8(text, -1, NULL, NULL, NULL);
	if (utf8_text == NULL) {
		/* error - fallback to hardcoded charset (ms windows one) */
		utf8_text = g_convert(text, strlen(text),
				      "UTF-8", "CP1252",
				      NULL, NULL, NULL);
		if (utf8_text == NULL) {
			/* shouldn't happen I think.. */
			utf8_text = g_strdup(text);
		}
	}

	gtk_text_buffer_get_end_iter(window->buffer, &iter);

	if (window->newline) {
		gtk_text_buffer_insert(window->buffer, &iter, "\n", 1);
		window->newline = FALSE;
	}

	if (flags & GUI_PRINT_FLAG_NEWLINE)
		gtk_text_buffer_insert(window->buffer, &iter, "\n", 1);

	if (flags & GUI_PRINT_FLAG_INDENT) {
		/* get the current cursor position from
		   left margin in pixels */
		GdkRectangle location;
		GtkTextView *view;

		view = window->active_view->view;
		gtk_text_view_get_iter_location(view, &iter, &location);
		window->indent = -location.x +
			gtk_text_view_get_left_margin(view);
	}

	/* add text */
	start_offset = gtk_text_iter_get_offset(&iter);
	gtk_text_buffer_insert(window->buffer, &iter, utf8_text, -1);
	gtk_text_buffer_place_cursor(window->buffer, &iter);
	gtk_text_buffer_get_iter_at_offset(window->buffer, &start_iter,
					   start_offset);

	/* add tags */
	if (flags & GUI_PRINT_FLAG_UNDERLINE) {
		gtk_text_buffer_apply_tag(window->buffer, window->tag_underline,
					  &start_iter, &iter);
	}
	if (flags & GUI_PRINT_FLAG_MONOSPACE) {
		gtk_text_buffer_apply_tag(window->buffer, window->tag_monospace,
					  &start_iter, &iter);
	}

	if ((flags & GUI_PRINT_FLAG_MIRC_COLOR) == 0) {
		/* normal color */
		if (fg < 0 || fg > DEFAULT_COLORS)
			fg = -1;
		if (bg < 0 || bg > DEFAULT_COLORS)
			bg = -1;
		colortab = colors;

		if (flags & GUI_PRINT_FLAG_BOLD) {
			if (fg == -1)
				fg = COLOR_BOLD;
			else
				fg |= 8;
		}

		if ((flags & GUI_PRINT_FLAG_BLINK) && bg != -1)
			bg |= 8;

		g_snprintf(fg_tag_name, sizeof(fg_tag_name), "c_%d", fg);
		g_snprintf(bg_tag_name, sizeof(bg_tag_name), "bc_%d", bg);
	} else {
		/* mirc color */
		fg %= MIRC_COLORS;
		bg %= MIRC_COLORS;
		colortab = mirc_colors;

		g_snprintf(fg_tag_name, sizeof(fg_tag_name), "m_%d", fg);
		g_snprintf(bg_tag_name, sizeof(bg_tag_name), "bm_%d", bg);
	}

	if ((flags & GUI_PRINT_FLAG_BOLD) &&
	    (flags & GUI_PRINT_FLAG_MIRC_COLOR)) {
		/* bold mirc color */
		tag = gtk_text_tag_table_lookup(window->tagtable, "bold");
		if (tag == NULL) {
			tag = gtk_text_buffer_create_tag(window->buffer,
							 "bold", NULL);
			g_object_set(G_OBJECT(tag), "weight",
				     PANGO_WEIGHT_BOLD, NULL);
		}

		gtk_text_buffer_apply_tag(window->buffer, tag,
					  &start_iter, &iter);
	}

	if (fg >= 0) {
		tag = gtk_text_tag_table_lookup(window->tagtable, fg_tag_name);
		if (tag == NULL) {
			tag = gtk_text_buffer_create_tag(window->buffer,
							 fg_tag_name, NULL);
			g_object_set(G_OBJECT(tag), "foreground-gdk",
				     &colortab[fg], NULL);
		}

		gtk_text_buffer_apply_tag(window->buffer, tag,
					  &start_iter, &iter);
	}

	if (bg >= 0) {
		tag = gtk_text_tag_table_lookup(window->tagtable, bg_tag_name);
		if (tag == NULL) {
			tag = gtk_text_buffer_create_tag(window->buffer,
							 bg_tag_name, NULL);
			g_object_set(G_OBJECT(tag), "background-gdk",
				     &colortab[fg], NULL);
		}

		gtk_text_buffer_apply_tag(window->buffer, tag,
					  &start_iter, &iter);
	}

	/* add context tags */
	gui_window_print_mark_context(window, dest, &start_iter, utf8_text);

	g_free(utf8_text);
}

static void sig_window_create_override(gpointer tab)
{
	window_create_override = GPOINTER_TO_INT(tab);
}

static void sig_window_created(Window *window, void *automatic)
{
	Frame *frame;
	Tab *tab;
	WindowGui *gui;

	frame = active_frame;
	if (window_create_override == 0 || active_frame == NULL)
		frame = gui_frame_new(TRUE);

	tab = frame->active_tab;
	if (window_create_override <= 1 || frame->active_tab == NULL)
		tab = gui_tab_new(frame);

	if (!automatic || active_frame == NULL) {
		gui_frame_set_active(frame);
		gui_tab_set_active(tab);
	}

	window_create_override = -1;

	gui = g_new0(WindowGui, 1);
	gui->window = window;
	window->gui_data = gui;

	gui->buffer = gtk_text_buffer_new(NULL);
	gui->tagtable = gtk_text_buffer_get_tag_table(gui->buffer);
	gui->font_monospace = pango_font_description_from_string("Monospace 10");

	/* underline tag */
	gui->tag_underline =
		gtk_text_buffer_create_tag(gui->buffer, NULL, NULL);
	g_object_set(G_OBJECT(gui->tag_underline), "underline",
		     PANGO_UNDERLINE_SINGLE, NULL);
	/* monospace tag */
	gui->tag_monospace =
		gtk_text_buffer_create_tag(gui->buffer, NULL, NULL);
	g_object_set(G_OBJECT(gui->tag_monospace), "font-desc",
		     gui->font_monospace, NULL);

	gui_window_add_view(gui, tab);
	g_object_unref(G_OBJECT(gui->buffer));

	/* just to make sure it won't contain invalid value before
	   size_allocate event is sent to window view, which then
	   updates to correct size */
	window->width = 60;
	window->height = 20;

	signal_emit("gui window created", 1, gui);
}

static void sig_window_destroyed(Window *window)
{
	WindowGui *gui;
	Frame *frame;

	gui = WINDOW_GUI(window);

	signal_emit("gui window destroyed", 1, gui);

	while (gui->views != NULL) {
		WindowView *view = gui->views->data;
		gui->views = g_slist_remove(gui->views, view);

		frame = view->pane->tab->frame;
		gtk_widget_destroy(view->pane->widget);

		if (frame->notebook->children == NULL) {
			/* we just destroyed the last tab - kill the window.
			   I'd rather do this somewhere else, but window
			   moving code makes it more difficult.. */
			gtk_widget_destroy(GTK_WIDGET(frame->window));
		}
	}

	pango_font_description_free(gui->font_monospace);

	g_free(gui);
	window->gui_data = NULL;
}

static void sig_window_changed(Window *window)
{
	WindowGui *gui = WINDOW_GUI(window);

	gui_tab_set_active_window(gui->active_view->pane->tab, window);
	gui_tab_set_active(gui->active_view->pane->tab);
}

static void sig_window_item_changed(Window *window, WindowItem *witem)
{
	WindowGui *gui = WINDOW_GUI(window);
	GSList *tmp;

	for (tmp = gui->views; tmp != NULL; tmp = tmp->next) {
		WindowView *view = tmp->data;

		gui_tab_set_active_window_item(view->pane->tab, window);
		gui_window_view_set_title(view);
	}
}

static void sig_gui_print_text(Window *window, void *fgcolor,
			       void *bgcolor, void *pflags,
			       const char *str, TEXT_DEST_REC *dest)
{
	int fg, bg, flags;

	g_return_if_fail(window != NULL);

	flags = GPOINTER_TO_INT(pflags);
	fg = GPOINTER_TO_INT(fgcolor);
	bg = GPOINTER_TO_INT(bgcolor);

	gui_window_print(WINDOW_GUI(window), dest, str, fg, bg, flags);
}

static GtkTextTag *get_indent_tag(WindowGui *gui, int indent)
{
	GtkTextTag *tag;
	char tag_name[50];

	g_snprintf(tag_name, sizeof(tag_name), "i_%d", indent);
	tag = gtk_text_tag_table_lookup(gui->tagtable, tag_name);
	if (tag == NULL) {
		tag = gtk_text_buffer_create_tag(gui->buffer, tag_name, NULL);
		g_object_set(G_OBJECT(tag), "indent", indent, NULL);
	}
	return tag;
}

static void sig_gui_printtext_finished(Window *window)
{
	WindowGui *gui;
	GtkTextIter start_iter, end_iter;
	int lines, max_lines, burst;

	gui = WINDOW_GUI(window);
	if (gui->indent != 0) {
		/* set indentation for line */
		gtk_text_buffer_get_end_iter(gui->buffer, &end_iter);
		memcpy(&start_iter, &end_iter, sizeof(end_iter));
		gtk_text_iter_set_line_index(&start_iter, 0);

		gtk_text_buffer_apply_tag(gui->buffer,
					  get_indent_tag(gui, gui->indent),
					  &start_iter, &end_iter);
		gui->indent = 0;
	}

        lines = gtk_text_buffer_get_line_count(gui->buffer);
	burst = settings_get_int("scrollback_burst_remove");
	max_lines = settings_get_int("scrollback_lines");

	if (max_lines > 0 && lines >= max_lines+burst) {
		/* remove first lines */
		gtk_text_buffer_get_iter_at_line(gui->buffer, &start_iter, 0);
		gtk_text_buffer_get_iter_at_line(gui->buffer, &end_iter, burst);
		gtk_text_buffer_delete(gui->buffer, &start_iter, &end_iter);
	}

	gui->newline = TRUE;
}

void gui_windows_init(void)
{
	settings_add_int("history", "scrollback_lines", 500);
	settings_add_int("history", "scrollback_burst_remove", 10);

	window_create_override = -1;

	signal_add("gui window create override", (SIGNAL_FUNC) sig_window_create_override);
	signal_add_first("window created", (SIGNAL_FUNC) sig_window_created);
	signal_add_last("window destroyed", (SIGNAL_FUNC) sig_window_destroyed);
	signal_add_first("window changed", (SIGNAL_FUNC) sig_window_changed);
	signal_add("window item changed", (SIGNAL_FUNC) sig_window_item_changed);
	signal_add("gui print text", (SIGNAL_FUNC) sig_gui_print_text);
	signal_add("gui print text finished", (SIGNAL_FUNC) sig_gui_printtext_finished);

	gui_window_views_init();
	gui_window_contexts_init();
        gui_window_activities_init();
}

void gui_windows_deinit(void)
{
        gui_window_activities_deinit();
	gui_window_contexts_deinit();
	gui_window_views_deinit();

	signal_remove("gui window create override", (SIGNAL_FUNC) sig_window_create_override);
	signal_remove("window created", (SIGNAL_FUNC) sig_window_created);
	signal_remove("window destroyed", (SIGNAL_FUNC) sig_window_destroyed);
	signal_remove("window changed", (SIGNAL_FUNC) sig_window_changed);
	signal_remove("window item changed", (SIGNAL_FUNC) sig_window_item_changed);
	signal_remove("gui print text", (SIGNAL_FUNC) sig_gui_print_text);
	signal_remove("gui print text finished", (SIGNAL_FUNC) sig_gui_printtext_finished);
}
