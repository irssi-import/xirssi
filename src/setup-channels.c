/*
 setup-channels.c : irssi

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
#include "chat-protocols.h"
#include "chatnets.h"
#include "channels-setup.h"

#include "setup.h"
#include "gui.h"

void channel_dialog_show(ChannelConfig *channel, const char *network);

static void setup_channel_signals_init(void);
static void setup_channel_signals_deinit(void);

enum {
	COL_PTR,
	COL_AUTOJOIN,
	COL_NAME,

	N_COLUMNS
};

static GtkTreeStore *channel_store = NULL;

static gboolean event_destroy(GtkWidget *widget)
{
	setup_channel_signals_deinit();
	channel_store = NULL;
	return FALSE;
}

static gboolean event_button_press(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreePath *path;
	GtkTreeModel *model;
        GtkTreeIter iter;

	if (!gtk_tree_view_get_path_at_pos(tree, event->x, event->y,
					   &path, NULL, NULL, NULL))
		return FALSE;

	model = gtk_tree_view_get_model(tree);
	if (!gtk_tree_model_get_iter(model, &iter, path))
		return FALSE;

	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		/* doubleclicked column - expand/collapse child nodes */
		if (!gtk_tree_view_row_expanded(tree, path))
			gtk_tree_view_expand_row(tree, path, FALSE);
		else
			gtk_tree_view_collapse_row(tree, path);
	}

	return FALSE;
}

static void selection_get_first_network(GtkTreeModel *model, GtkTreePath *path,
					GtkTreeIter *iter, gpointer data)
{
	const char **network = data;
	void *config;

	if (*network != NULL)
		return;

	gtk_tree_model_get(model, iter, COL_PTR, &config, -1);
	if (config != NULL)
		*network = g_strdup(CHANNEL_SETUP(config)->chatnet);
	else
		gtk_tree_model_get(model, iter, COL_NAME, network, -1);
}

static gboolean event_add(GtkWidget *widget, GtkTreeView *view)
{
	GtkTreeSelection *sel;
	char *network;

	network = NULL;
	sel = gtk_tree_view_get_selection(view);
	gtk_tree_selection_selected_foreach(sel, selection_get_first_network,
					    &network);

	channel_dialog_show(NULL, network);
	g_free(network);
	return FALSE;
}

static void selection_edit(GtkTreeModel *model, GtkTreePath *path,
			   GtkTreeIter *iter, gpointer data)
{
	gtk_tree_model_get(model, iter, COL_PTR, &data, -1);

	if (data != NULL)
		channel_dialog_show(data, NULL);
}

static gboolean event_edit(GtkWidget *widget, GtkTreeView *view)
{
	GtkTreeSelection *sel;

	sel = gtk_tree_view_get_selection(view);
        gtk_tree_selection_selected_foreach(sel, selection_edit, NULL);
	return FALSE;
}

static void network_remove_with_channels(const char *network)
{
	NetworkConfig *netconfig;
	GSList *tmp, *next;

	for (tmp = setupchannels; tmp != NULL; tmp = next) {
		ChannelConfig *channel = tmp->data;

		next = tmp->next;
		if (channel->chatnet == NULL ||
		    g_strcasecmp(channel->chatnet, network) != 0)
			continue;

                channel_setup_remove(channel);
	}

	netconfig = chatnet_find(network);
	if (netconfig != NULL)
		chatnet_remove(netconfig);
}

static gboolean event_remove(GtkWidget *widget, GtkTreeView *view)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GSList *paths;
	char *name;
	void *data;

	model = GTK_TREE_MODEL(channel_store);

	paths =  gui_tree_selection_get_paths(view);
	while (paths != NULL) {
		GtkTreePath *path = paths->data;

		/* remove from tree */
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, COL_PTR, &data, -1);
		gtk_tree_store_remove(channel_store, &iter);

		/* remove from config */
		if (data != NULL)
			channel_setup_remove(data);
		else {
			gtk_tree_model_get(model, &iter, COL_NAME, &name, -1);
			network_remove_with_channels(name);
			g_free(name);
		}

		paths = g_slist_remove(paths, path);
		gtk_tree_path_free(path);
	}

	return FALSE;
}

typedef struct {
	gboolean found;
	GtkTreeIter *iter;
	ChannelConfig *channel;
	const char *network;
} StoreFind;

static gboolean store_find_network_func(GtkTreeModel *model, GtkTreePath *path,
					GtkTreeIter *iter, gpointer data)
{
	StoreFind *rec = data;
	void *iter_data;
	char *iter_name;

	gtk_tree_model_get(model, iter, COL_PTR, &iter_data, -1);
	if (iter_data != NULL)
		return FALSE;

	gtk_tree_model_get(model, iter, COL_NAME, &iter_name, -1);
	if (g_strcasecmp(iter_name, rec->network) == 0) {
		rec->found = TRUE;
		memcpy(rec->iter, iter, sizeof(GtkTreeIter));
	}
	g_free(iter_name);

	return rec->found;
}

static gboolean store_find_network(GtkTreeStore *store, GtkTreeIter *iter,
				   const char *network)
{
	StoreFind rec;

	rec.found = FALSE;
	rec.iter = iter;
	rec.network = network;
	gtk_tree_model_foreach(GTK_TREE_MODEL(store),
			       store_find_network_func, &rec);

	return rec.found;
}

static gboolean store_find_channel_func(GtkTreeModel *model, GtkTreePath *path,
					GtkTreeIter *iter, gpointer data)
{
	StoreFind *rec = data;
	void *iter_data;

	gtk_tree_model_get(model, iter, COL_PTR, &iter_data, -1);
	if (iter_data == rec->channel) {
		rec->found = TRUE;
		memcpy(rec->iter, iter, sizeof(GtkTreeIter));
	}

	return rec->found;
}

static gboolean store_find_channel(GtkTreeStore *store, GtkTreeIter *iter,
				   ChannelConfig *channel)
{
	StoreFind rec;

	rec.found = FALSE;
	rec.iter = iter;
	rec.channel = channel;
	gtk_tree_model_foreach(GTK_TREE_MODEL(store),
			       store_find_channel_func, &rec);

	return rec.found;
}

static void store_setup_channel(GtkTreeStore *store, GtkTreeIter *iter,
				ChannelConfig *channel)
{
	gtk_tree_store_set(store, iter,
			   COL_PTR, channel,
			   COL_NAME, channel->name,
			   -1);
}

static void channel_store_fill(GtkTreeStore *store)
{
	GtkTreeIter iter, net_iter;
	GSList *tmp;

	/* add channels - add the networks on the way, they may not even
	   exist in the network list so use just their names */
	for (tmp = setupchannels; tmp != NULL; tmp = tmp->next) {
		ChannelConfig *channel = tmp->data;

		if (channel->chatnet == NULL)
			gtk_tree_store_append(store, &iter, NULL);
		else {
			if (!store_find_network(store, &net_iter,
						channel->chatnet)) {
				/* unknown network, create it */
				gtk_tree_store_append(store, &net_iter, NULL);
				gtk_tree_store_set(store, &net_iter,
						   COL_NAME, channel->chatnet,
						   -1);
			}

			gtk_tree_store_append(store, &iter, &net_iter);
		}

		store_setup_channel(store, &iter, channel);
	}
}

static void autojoin_toggled(GtkCellRendererToggle *cell, char *path_string)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	ChannelConfig *channel;

	model = GTK_TREE_MODEL(channel_store);
	path = gtk_tree_path_new_from_string(path_string);

	/* update config */
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, COL_PTR, &channel, -1);
	channel->autojoin = !gtk_cell_renderer_toggle_get_active(cell);

	/* update tree view */
        gtk_tree_model_row_changed(model, path, &iter);
}

static void autojoin_set_func(GtkTreeViewColumn *column,
			      GtkCellRenderer   *cell,
			      GtkTreeModel      *model,
			      GtkTreeIter       *iter,
			      gpointer           data)
{
	ChannelConfig *channel;

	gtk_tree_model_get(model, iter, COL_PTR, &channel, -1);

	g_object_set(G_OBJECT(cell),
		     "active", channel != NULL && channel->autojoin,
		     "visible", channel != NULL,
		     NULL);
}

static void channel_set_func(GtkTreeViewColumn *column,
			     GtkCellRenderer   *cell,
			     GtkTreeModel      *model,
			     GtkTreeIter       *iter,
			     gpointer           data)
{
	ChannelConfig *channel;
	char *name;

	gtk_tree_model_get(model, iter, COL_PTR, &channel, -1);

	if (channel == NULL) {
		/* network */
		gtk_tree_model_get(model, iter, COL_NAME, &name, -1);
		g_object_set(G_OBJECT(cell),
			     "text", name,
			     "weight", PANGO_WEIGHT_BOLD,
			     NULL);
		g_free(name);
	} else {
		/* channel */
		g_object_set(G_OBJECT(cell),
			     "text", channel->name,
			     "weight", 0,
			     NULL);
	}
}

void setup_channels_init(GtkWidget *dialog)
{
	GtkWidget *button;
	GtkTreeView *tree_view;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	g_signal_connect(G_OBJECT(dialog), "destroy",
			 G_CALLBACK(event_destroy), NULL);

	/* tree store */
	channel_store = gtk_tree_store_new(N_COLUMNS,
					   G_TYPE_POINTER,
					   G_TYPE_BOOLEAN,
					   G_TYPE_STRING);
        channel_store_fill(channel_store);

	/* view */
	tree_view = g_object_get_data(G_OBJECT(dialog), "channel_tree");
	g_signal_connect(G_OBJECT(tree_view), "button_press_event",
			 G_CALLBACK(event_button_press), NULL);

	gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(channel_store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tree_view),
				    GTK_SELECTION_MULTIPLE);

	/* -- */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "Autojoin / Name");

	renderer = gtk_cell_renderer_toggle_new();
	g_object_set(G_OBJECT(renderer), "activatable", TRUE, NULL);
	g_signal_connect(G_OBJECT(renderer), "toggled",
			 G_CALLBACK(autojoin_toggled), NULL);

	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						autojoin_set_func,
						NULL, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						channel_set_func, NULL, NULL);

	gtk_tree_view_append_column(tree_view, column);

	/* buttons */
	button = g_object_get_data(G_OBJECT(dialog), "channel_add");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_add), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "channel_edit");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_edit), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "channel_remove");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_remove), tree_view);

	setup_channel_signals_init();

	gtk_tree_view_expand_all(tree_view);
}

static void sig_channel_added(ChannelConfig *channel)
{
	GtkTreeModel *model;
	GtkTreeIter iter, parent;
	char *name;
	int append;

	if (!store_find_channel(channel_store, &iter, channel))
		append = TRUE;
	else {
		/* make sure the network is still the same */
		append = FALSE;
		model = GTK_TREE_MODEL(channel_store);
		if (gtk_tree_model_iter_parent(model, &parent, &iter)) {
			/* we were below a network */
			gtk_tree_model_get(model, &parent, COL_NAME, &name, -1);
			if (channel->chatnet == NULL ||
			    g_strcasecmp(channel->chatnet, name) != 0)
				append = TRUE;
			g_free(name);
		} else {
			/* we didn't have network */
			if (channel->chatnet != NULL)
				append = TRUE;
		}

		if (append)
			gtk_tree_store_remove(channel_store, &iter);
	}

	if (append) {
		if (channel->chatnet == NULL)
			gtk_tree_store_append(channel_store, &iter, NULL);
		else if (store_find_network(channel_store, &parent,
					    channel->chatnet))
			gtk_tree_store_append(channel_store, &iter, &parent);
		else {
			/* create the network */
			gtk_tree_store_append(channel_store, &parent, NULL);
			gtk_tree_store_set(channel_store, &parent,
					   COL_NAME, channel->chatnet,
					   -1);
			gtk_tree_store_append(channel_store, &iter, &parent);
		}
	}

	store_setup_channel(channel_store, &iter, channel);
}

static void sig_channel_removed(ChannelConfig *channel)
{
	GtkTreeIter iter;

	if (store_find_channel(channel_store, &iter, channel))
		gtk_tree_store_remove(channel_store, &iter);
}

static void sig_network_added(NetworkConfig *network)
{
	GtkTreeIter iter;

	if (!store_find_network(channel_store, &iter, network->name))
		gtk_tree_store_append(channel_store, &iter, NULL);
	gtk_tree_store_set(channel_store, &iter,
			   COL_NAME, network->name,
			   -1);
}

static int network_has_channels(const char *network)
{
	GSList *tmp;

	for (tmp = setupchannels; tmp != NULL; tmp = tmp->next) {
		CHANNEL_SETUP_REC *rec = tmp->data;

		if (rec->chatnet != NULL &&
		    g_strcasecmp(rec->chatnet, network) == 0)
			return TRUE;
	}

	return FALSE;
}

static void sig_network_removed(NetworkConfig *network)
{
	GtkWidget *dialog;
	GtkTreeIter iter;

	if (!store_find_network(channel_store, &iter, network->name))
		return;

	/* check if there's some channels in the removed network */
	if (!network_has_channels(network->name)) {
		gtk_tree_store_remove(channel_store, &iter);
		return;
	}

	/* yes, ask if we want to remove them */
	dialog = gtk_message_dialog_new(GTK_WINDOW(setup_dialog),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					"Do you also want to "
					"remove all channels in %s?",
					network->name);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
		if (store_find_network(channel_store, &iter, network->name))
			gtk_tree_store_remove(channel_store, &iter);
	}

	gtk_widget_destroy(dialog);
}

static void setup_channel_signals_init(void)
{
	signal_add("channel setup created", (SIGNAL_FUNC) sig_channel_added);
	signal_add("channel setup destroyed", (SIGNAL_FUNC) sig_channel_removed);
	signal_add("chatnet created", (SIGNAL_FUNC) sig_network_added);
	signal_add("chatnet removed", (SIGNAL_FUNC) sig_network_removed);
}

static void setup_channel_signals_deinit(void)
{
	signal_remove("channel setup created", (SIGNAL_FUNC) sig_channel_added);
	signal_remove("channel setup destroyed", (SIGNAL_FUNC) sig_channel_removed);
	signal_remove("chatnet created", (SIGNAL_FUNC) sig_network_added);
	signal_remove("chatnet removed", (SIGNAL_FUNC) sig_network_removed);
}
