/*
 gui-nicklist.c : irssi

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
#include "modules.h"
#include "signals.h"
#include "nicklist.h"

#include "gui-channel.h"
#include "gui-nicklist.h"
#include "gui-nicklist-view.h"

#include "ball-green.h"
#include "ball-orange.h"
#include "ball-yellow.h"

static GdkPixbuf *op_pixbuf, *voice_pixbuf, *halfop_pixbuf;

static gint nicklist_sort_func(GtkTreeModel *model,
			       GtkTreeIter *a, GtkTreeIter *b,
			       gpointer user_data)
{
	Nick *nick1, *nick2;
	GValue value;

	memset(&value, 0, sizeof(value));

	gtk_tree_model_get_value(model, a, NICKLIST_COL_PTR, &value);
	nick1 = value.data[0].v_pointer;
	g_value_unset(&value);

	gtk_tree_model_get_value(model, b, NICKLIST_COL_PTR, &value);
	nick2 = value.data[0].v_pointer;
	g_value_unset(&value);

	return nicklist_compare(nick1, nick2);
}

Nicklist *gui_nicklist_new(Channel *channel)
{
	Nicklist *nicklist;
	GtkListStore *store;

	nicklist = g_new0(Nicklist, 1);
	nicklist->refcount = 1;
	nicklist->channel = channel;

	nicklist->store = store =
		gtk_list_store_new(3, G_TYPE_POINTER, G_TYPE_STRING,
				   GDK_TYPE_PIXBUF);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store),
					NICKLIST_COL_NAME,
					nicklist_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
					     NICKLIST_COL_NAME,
					     GTK_SORT_ASCENDING);

	signal_emit("gui nicklist created", 1, nicklist);
	return nicklist;
}

void gui_nicklist_ref(Nicklist *nicklist)
{
	nicklist->refcount++;
}

void gui_nicklist_unref(Nicklist *nicklist)
{
	if (--nicklist->refcount > 0)
		return;

	signal_emit("gui nicklist destroyed", 1, nicklist);

	g_object_unref(G_OBJECT(nicklist->store));
	g_free(nicklist);
}

static void gui_nicklist_update_label(Nicklist *nicklist)
{
	g_snprintf(nicklist->label, sizeof(nicklist->label),
		   "%d ops, %d total",
		   nicklist->ops, nicklist->nicks);

	g_slist_foreach(nicklist->views,
			(GFunc) gui_nicklist_view_label_updated, NULL);
}

static void gui_nicklist_add(Channel *channel, Nick *nick)
{
	ChannelGui *gui;
	GdkPixbuf *pixbuf;
	GtkTreeIter iter;

	gui = CHANNEL_GUI(channel);

	if (nick->op) {
		gui->nicklist->ops++;
		pixbuf = op_pixbuf;
	} else if (nick->halfop) {
		gui->nicklist->halfops++;
		pixbuf = halfop_pixbuf;
	} else if (nick->voice) {
		gui->nicklist->voices++;
		pixbuf = voice_pixbuf;
	} else {
		gui->nicklist->normal++;
		pixbuf = NULL;
	}
	gui->nicklist->nicks++;

	gtk_list_store_append(gui->nicklist->store, &iter);
	gtk_list_store_set(gui->nicklist->store, &iter,
			   NICKLIST_COL_PTR, nick,
			   NICKLIST_COL_NAME, nick->nick,
			   NICKLIST_COL_PIXMAP, pixbuf,
			   -1);

	gui_nicklist_update_label(gui->nicklist);
}

static void gui_nicklist_remove(Channel *channel, Nick *nick)
{
	ChannelGui *gui;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GValue value;
	NICK_REC *val_nick;

	gui = CHANNEL_GUI(channel);
	if (gui == NULL)
		return;

	if (nick->op)
		gui->nicklist->ops--;
	else if (nick->halfop)
		gui->nicklist->halfops--;
	else if (nick->voice)
		gui->nicklist->voices--;
	else
		gui->nicklist->normal--;
	gui->nicklist->nicks--;

	memset(&value, 0, sizeof(value));

	model = GTK_TREE_MODEL(gui->nicklist->store);
        gtk_tree_model_get_iter_first(model, &iter);
	do {
		gtk_tree_model_get_value(model, &iter, NICKLIST_COL_PTR,
					 &value);
		val_nick = value.data[0].v_pointer;
		g_value_unset(&value);

		if (val_nick == nick) {
			gtk_list_store_remove(gui->nicklist->store, &iter);
			break;
		}
	} while (gtk_tree_model_iter_next(model, &iter));

	gui_nicklist_update_label(gui->nicklist);
}

static void gui_nicklist_changed(Channel *channel, Nick *nick)
{
	gui_nicklist_remove(channel, nick);
	gui_nicklist_add(channel, nick);
}

void gui_nicklists_init(void)
{
	op_pixbuf = gdk_pixbuf_new_from_inline(sizeof(ball_green),
					       ball_green, FALSE, NULL);
	halfop_pixbuf = gdk_pixbuf_new_from_inline(sizeof(ball_orange),
						   ball_orange, FALSE, NULL);
	voice_pixbuf = gdk_pixbuf_new_from_inline(sizeof(ball_yellow),
						  ball_yellow, FALSE, NULL);

	signal_add("nicklist new", (SIGNAL_FUNC) gui_nicklist_add);
	signal_add("nicklist remove", (SIGNAL_FUNC) gui_nicklist_remove);
	signal_add("nicklist changed", (SIGNAL_FUNC) gui_nicklist_changed);
	signal_add("nick mode changed", (SIGNAL_FUNC) gui_nicklist_changed);
}

void gui_nicklists_deinit(void)
{
	signal_remove("nicklist new", (SIGNAL_FUNC) gui_nicklist_add);
	signal_remove("nicklist remove", (SIGNAL_FUNC) gui_nicklist_remove);
	signal_remove("nicklist changed", (SIGNAL_FUNC) gui_nicklist_changed);
	signal_remove("nick mode changed", (SIGNAL_FUNC) gui_nicklist_changed);
}
