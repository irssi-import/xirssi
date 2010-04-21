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
#include "channels.h"
#include "servers.h"
#include "nicklist.h"

#include "gui-channel.h"
#include "gui-nicklist.h"
#include "gui-nicklist-view.h"

static gint nicklist_sort_func(GtkTreeModel *model,
			       GtkTreeIter *a, GtkTreeIter *b,
			       gpointer user_data)
{
	Nick *nick1, *nick2;

	gtk_tree_model_get(model, a, 0, &nick1, -1);
	gtk_tree_model_get(model, b, 0, &nick2, -1);

	return nicklist_compare(nick1, nick2, user_data);
}

Nicklist *gui_nicklist_new(Channel *channel)
{
	Nicklist *nicklist;
	GtkListStore *store;
	const gchar *nick_flags = NULL;

	nicklist = g_new0(Nicklist, 1);
	nicklist->channel = channel;
	nick_flags = channel->server->get_nick_flags(channel->server);

	nicklist->store = store = gtk_list_store_new(1, G_TYPE_POINTER);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), 0,
					nicklist_sort_func, (void *)nick_flags, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), 0,
					     GTK_SORT_ASCENDING);

	signal_emit("gui nicklist created", 1, nicklist);
	return nicklist;
}

void gui_nicklist_destroy(Nicklist *nicklist)
{
	signal_emit("gui nicklist destroyed", 1, nicklist);

	while (nicklist->views != NULL) {
		NicklistView *view = nicklist->views->data;
		gui_nicklist_view_set(view, NULL);
		nicklist->views = g_slist_remove(nicklist->views, view);
	}

	g_object_unref(G_OBJECT(nicklist->store));
	g_free(nicklist);
}

static void gui_nicklist_update_label(Nicklist *nicklist)
{
	GSList *tmp;
	char label[128];

	g_snprintf(label, sizeof(label), "%d ops, %d total",
		   nicklist->ops, nicklist->nicks);

	for (tmp = nicklist->views; tmp != NULL; tmp = tmp->next) {
		NicklistView *view = tmp->data;

		gui_nicklist_view_update_label(view, label);
	}
}

static void gui_nicklist_add(Channel *channel, Nick *nick, int update_label)
{
	ChannelGui *gui;
	GtkTreeIter iter;

	gui = CHANNEL_GUI(channel);

	if (nick->op)
		gui->nicklist->ops++;
	else if (nick->halfop)
		gui->nicklist->halfops++;
	else if (nick->voice)
		gui->nicklist->voices++;
	else
		gui->nicklist->normal++;
	gui->nicklist->nicks++;

	gtk_list_store_append(gui->nicklist->store, &iter);
	gtk_list_store_set(gui->nicklist->store, &iter,
			   0, nick, -1);

	gui_nicklist_update_label(gui->nicklist);
}

static void gui_nicklist_remove(Channel *channel, Nick *nick, int update_label)
{
	ChannelGui *gui;
	GtkTreeModel *model;
	GtkTreeIter iter;
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

	model = GTK_TREE_MODEL(gui->nicklist->store);
        gtk_tree_model_get_iter_first(model, &iter);
	do {
		gtk_tree_model_get(model, &iter, 0, &val_nick, -1);
		if (val_nick == nick) {
			gtk_list_store_remove(gui->nicklist->store, &iter);
			break;
		}
	} while (gtk_tree_model_iter_next(model, &iter));

	gui_nicklist_update_label(gui->nicklist);
}

static void gui_nicklist_changed(Channel *channel, Nick *nick)
{
	gui_nicklist_remove(channel, nick, FALSE);
	gui_nicklist_add(channel, nick, TRUE);
}

static void gui_nicklist_mode_changed(Channel *channel, Nick *nick)
{
	GSList *nicks, *tmp;
	ChannelGui *gui;

	gui_nicklist_remove(channel, nick, FALSE);
	gui_nicklist_add(channel, nick, FALSE);

	/* nick counts are messed up now, we have to recalculate them */
	gui = CHANNEL_GUI(channel);
	gui->nicklist->ops = gui->nicklist->halfops =
		gui->nicklist->voices = gui->nicklist->normal = 0;

	nicks = nicklist_getnicks(channel);
	for (tmp = nicks; tmp != NULL; tmp = tmp->next) {
		Nick *nick = tmp->data;

		if (nick->op)
			gui->nicklist->ops++;
		else if (nick->halfop)
			gui->nicklist->halfops++;
		else if (nick->voice)
			gui->nicklist->voices++;
		else
			gui->nicklist->normal++;
	}
	g_slist_free(nicks);

	gui_nicklist_update_label(gui->nicklist);
}

void gui_nicklists_init(void)
{
	signal_add("nicklist new", (SIGNAL_FUNC) gui_nicklist_add);
	signal_add("nicklist remove", (SIGNAL_FUNC) gui_nicklist_remove);
	signal_add("nicklist changed", (SIGNAL_FUNC) gui_nicklist_changed);
	signal_add("nick mode changed", (SIGNAL_FUNC) gui_nicklist_mode_changed);
}

void gui_nicklists_deinit(void)
{
	signal_remove("nicklist new", (SIGNAL_FUNC) gui_nicklist_add);
	signal_remove("nicklist remove", (SIGNAL_FUNC) gui_nicklist_remove);
	signal_remove("nicklist changed", (SIGNAL_FUNC) gui_nicklist_changed);
	signal_remove("nick mode changed", (SIGNAL_FUNC) gui_nicklist_mode_changed);
}
