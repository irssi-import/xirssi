/*
 setup-servers.c : irssi

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
#include "chat-protocols.h"
#include "chatnets.h"
#include "servers-setup.h"

#include "setup.h"
#include "gui.h"

void network_dialog_show(NetworkConfig *network);
void server_dialog_show(ServerConfig *server, const char *network);

static void setup_server_signals_init(void);
static void setup_server_signals_deinit(void);

enum {
	COL_PTR,
	COL_AUTOCONNECT,
	COL_NAME,
	COL_PORT,
	COL_PROTOCOL,

	N_COLUMNS
};

static GtkTreeStore *server_store = NULL;

static gboolean event_destroy(GtkWidget *widget)
{
	setup_server_signals_deinit();
	server_store = NULL;
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

#if 0 // FIXME
typedef struct {
	GtkTreePath *path;
	GtkTreeIter iter;
	void *data;

	unsigned int found:1;
} TreeSearch;

static gboolean mode_find(GtkTreeModel *model, GtkTreePath *path,
			  GtkTreeIter *iter, gpointer data)
{
	TreeSearch *rec = data;
	void *tree_data;

	gtk_tree_model_get(model, iter, COL_PTR, &tree_data, -1);
	if (tree_data != rec->data)
		return FALSE;

	rec->found = TRUE;
	rec->path = path;
	memcpy(&rec->iter, iter, sizeof(GtkTreeIter));
	return TRUE;
}

static int tree_find_data(GtkTreeModel *model, void *data, GtkTreeIter *iter)
{
	TreeSearch rec;

	rec.data = data;
	rec.found = FALSE;
	gtk_tree_model_foreach(model, mode_find, &rec);

	if (rec.found)
		memcpy(iter, &rec.iter, sizeof(GtkTreeIter));

	return rec.found;
}

static int fix_tree_networks(NetworkConfig *network)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter, new_iter, sib_iter;

	model = GTK_TREE_MODEL(server_store);
	if (!tree_find_data(model, network, &iter))
		return 0;

	path = gtk_tree_model_get_path(model, &iter);
	gtk_tree_path_up(path);

	gtk_tree_store_remove(server_store, &iter);

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_path_free(path);

	gtk_tree_store_insert_before(server_store, &new_iter, NULL, &iter);
	gtk_tree_store_set(server_store, &new_iter,
			   COL_PTR, network,
			   COL_NAME, network->name,
			   -1);
	return 0;
}

static gboolean event_row_changed(GtkTreeModel *model,
				  GtkTreePath  *path,
				  GtkTreeIter  *iter)
{
	GtkTreeStore *store;
	GtkTreeIter new_iter, parent;
	void *data;

	store = GTK_TREE_STORE(model);
	gtk_tree_model_get(model, iter, COL_PTR, &data, -1);

	if (IS_CHATNET(data) && gtk_tree_path_get_depth(path) > 1) {
		/* network node placed under another network - not allowed */
		g_timeout_add(0, (GSourceFunc) fix_tree_networks, data);
	}

	return FALSE;
}
#endif

static gboolean event_add_network(void)
{
	network_dialog_show(NULL);
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
	if (IS_CHATNET(config))
		*network = CHATNET(config)->name;
	else
		*network = SERVER_SETUP(config)->chatnet;
}

static gboolean event_add_server(GtkWidget *widget, GtkTreeView *view)
{
	GtkTreeSelection *sel;
	const char *network;

	network = NULL;
	sel = gtk_tree_view_get_selection(view);
	gtk_tree_selection_selected_foreach(sel, selection_get_first_network,
					    &network);

	server_dialog_show(NULL, network);
	return FALSE;
}

static void selection_edit(GtkTreeModel *model, GtkTreePath *path,
			   GtkTreeIter *iter, gpointer data)
{
	gtk_tree_model_get(model, iter, COL_PTR, &data, -1);

	if (IS_CHATNET(data))
		network_dialog_show(data);
	else
		server_dialog_show(data, NULL);
}

static gboolean event_edit(GtkWidget *widget, GtkTreeView *view)
{
	GtkTreeSelection *sel;

	sel = gtk_tree_view_get_selection(view);
        gtk_tree_selection_selected_foreach(sel, selection_edit, NULL);
	return FALSE;
}

static void network_remove_with_servers(NetworkConfig *network)
{
	GSList *tmp, *next;

	for (tmp = setupservers; tmp != NULL; tmp = next) {
		ServerConfig *server = tmp->data;

		next = tmp->next;
		if (server->chatnet == NULL ||
		    g_strcasecmp(server->chatnet, network->name) != 0)
			continue;

                server_setup_remove(server);
	}

	chatnet_remove(network);
}

static gboolean event_remove(GtkWidget *widget, GtkTreeView *view)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GSList *paths;
	void *data;

	model = GTK_TREE_MODEL(server_store);

	paths =  gui_tree_selection_get_paths(view);
	while (paths != NULL) {
		GtkTreePath *path = paths->data;

		/* remove from tree */
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, COL_PTR, &data, -1);
		gtk_tree_store_remove(server_store, &iter);

		/* remove from config */
		if (IS_SERVER_SETUP(data))
			server_setup_remove(data);
		else
			network_remove_with_servers(data);

		paths = g_slist_remove(paths, path);
		gtk_tree_path_free(path);
	}

	return FALSE;
}

typedef struct {
	gboolean found;
	GtkTreeIter *iter;
	void *data;
} StoreFind;

static gboolean store_find_func(GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, gpointer data)
{
	StoreFind *rec = data;
	void *iter_data;

	gtk_tree_model_get(model, iter, COL_PTR, &iter_data, -1);
	if (iter_data == rec->data) {
		rec->found = TRUE;
		memcpy(rec->iter, iter, sizeof(GtkTreeIter));
	}

	return rec->found;
}

static gboolean store_find(GtkTreeStore *store, GtkTreeIter *iter, void *data)
{
	StoreFind rec;

	rec.found = FALSE;
	rec.iter = iter;
	rec.data = data;
	gtk_tree_model_foreach(GTK_TREE_MODEL(store), store_find_func, &rec);

	return rec.found;
}

static void store_setup_network(GtkTreeStore *store, GtkTreeIter *iter,
				NetworkConfig *network)
{
	gtk_tree_store_set(store, iter,
			   COL_PTR, network,
			   COL_NAME, network->name,
			   COL_PROTOCOL, CHAT_PROTOCOL(network)->name,
			   -1);
}

static void store_setup_server(GtkTreeStore *store, GtkTreeIter *iter,
			       ServerConfig *server)
{
	char port[MAX_INT_STRLEN];

	if (server->port <= 0)
		port[0] = '\0';
	else
		ltoa(port, server->port);

	gtk_tree_store_set(store, iter,
			   COL_PTR, server,
			   COL_NAME, server->address,
			   COL_PORT, port,
			   COL_PROTOCOL, CHAT_PROTOCOL(server)->name,
			   -1);
}

static void server_store_fill_network(GtkTreeStore *store, GtkTreeIter *parent,
				      NetworkConfig *network)
{
	GtkTreeIter iter;
	GSList *tmp;

	for (tmp = setupservers; tmp != NULL; tmp = tmp->next) {
		ServerConfig *server = tmp->data;

		if (network == NULL && server->chatnet != NULL)
			continue;

		if (server->chatnet == NULL ||
		    g_strcasecmp(server->chatnet, network->name) != 0)
			continue;

		gtk_tree_store_append(store, &iter, parent);
		store_setup_server(store, &iter, server);
	}
}

static void server_store_fill(GtkTreeStore *store)
{
	GtkTreeIter iter;
	GSList *tmp;

	for (tmp = chatnets; tmp != NULL; tmp = tmp->next) {
		NetworkConfig *network = tmp->data;

		gtk_tree_store_append(store, &iter, NULL);
		store_setup_network(store, &iter, network);
		server_store_fill_network(store, &iter, network);
	}
}

GList *get_chat_protocol_names(void)
{
	GList *list;
	GSList *tmp;

	list = NULL;
	for (tmp = chat_protocols; tmp != NULL; tmp = tmp->next) {
		ChatProtocol *proto = tmp->data;

		list = g_list_append(list, proto->name);
	}
	return list;
}

GList *get_network_names(int chat_type, const char *network_none)
{
	GList *list;
	GSList *tmp;

	list = NULL;
	for (tmp = chatnets; tmp != NULL; tmp = tmp->next) {
		NetworkConfig *network = tmp->data;

		if (chat_type == -1 || network->chat_type == chat_type)
			list = g_list_append(list, network->name);
	}
	list = g_list_append(list, (char *) network_none);
	return list;
}

static void autoconnect_toggled(GtkCellRendererToggle *cell, char *path_string)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	ServerConfig *server;
	gboolean active;

	model = GTK_TREE_MODEL(server_store);
	path = gtk_tree_path_new_from_string(path_string);

	active = !gtk_cell_renderer_toggle_get_active(cell);

	/* update store */
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_store_set(server_store, &iter,
			   COL_AUTOCONNECT, active, -1);

	/* update config */
	gtk_tree_model_get(model, &iter, COL_PTR, &server, -1);
	server->autoconnect = active;
}

static void autoconnect_set_func(GtkTreeViewColumn *column,
				 GtkCellRenderer   *cell,
				 GtkTreeModel      *model,
				 GtkTreeIter       *iter,
				 gpointer           data)
{
	ServerConfig *server;

	gtk_tree_model_get(model, iter, COL_PTR, &server, -1);
	server = SERVER_SETUP(server);

	g_object_set(G_OBJECT(cell),
		     "active", server != NULL && server->autoconnect,
		     "visible", server != NULL,
		     NULL);
}

static void server_set_func(GtkTreeViewColumn *column,
			    GtkCellRenderer   *cell,
			    GtkTreeModel      *model,
			    GtkTreeIter       *iter,
			    gpointer           data)
{
	void *iter_data;

	gtk_tree_model_get(model, iter, COL_PTR, &iter_data, -1);

	g_object_set(G_OBJECT(cell),
		     "weight", IS_CHATNET(iter_data) ? PANGO_WEIGHT_BOLD : 0,
		     NULL);
}

void setup_servers_init(GtkWidget *dialog)
{
	GtkWidget *button;
	GtkTreeView *tree_view;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	g_signal_connect(G_OBJECT(dialog), "destroy",
			 G_CALLBACK(event_destroy), NULL);

	/* tree store */
	server_store = gtk_tree_store_new(N_COLUMNS,
					  G_TYPE_POINTER,
					  G_TYPE_BOOLEAN,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING);
	/*g_signal_connect_after(G_OBJECT(server_store), "row_changed",
			       G_CALLBACK(event_row_changed), NULL);*/
        server_store_fill(server_store);

	/* view */
	tree_view = g_object_get_data(G_OBJECT(dialog), "servers_tree");
	g_signal_connect(G_OBJECT(tree_view), "button_press_event",
			 G_CALLBACK(event_button_press), NULL);

	gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(server_store));
	/*gtk_tree_view_set_reorderable(tree_view, TRUE);*/
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tree_view),
				    GTK_SELECTION_MULTIPLE);

	/* -- */
	renderer = gtk_cell_renderer_toggle_new();
	g_object_set(G_OBJECT(renderer), "activatable", TRUE, NULL);
	g_signal_connect(G_OBJECT(renderer), "toggled",
			 G_CALLBACK(autoconnect_toggled), NULL);
	column = gtk_tree_view_column_new_with_attributes("Autoconn", renderer,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						autoconnect_set_func,
						NULL, NULL);
	gtk_tree_view_column_set_alignment(column, 1.0);
	gtk_tree_view_append_column(tree_view, column);

	/* -- */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Name", renderer,
							  "text", COL_NAME,
							  NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						server_set_func, NULL, NULL);
	gtk_tree_view_append_column(tree_view, column);
	gtk_tree_view_column_set_min_width(column, 150);

	/* -- */
	column = gtk_tree_view_column_new_with_attributes("Port", renderer,
							  "text", COL_PORT,
							  NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* -- */
	column = gtk_tree_view_column_new_with_attributes("Protocol", renderer,
							  "text", COL_PROTOCOL,
							  NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* buttons */
	button = g_object_get_data(G_OBJECT(dialog), "server_add_network");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_add_network), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "server_add_server");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_add_server), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "server_edit");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_edit), tree_view);

	button = g_object_get_data(G_OBJECT(dialog), "server_remove");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_remove), tree_view);

	setup_server_signals_init();
}

static void sig_server_added(ServerConfig *server)
{
	NetworkConfig *network;
	GtkTreeModel *model;
	GtkTreeIter iter, parent;
	int append;

	if (!store_find(server_store, &iter, server))
		append = TRUE;
	else {
		/* make sure the network is still the same */
		append = FALSE;
		model = GTK_TREE_MODEL(server_store);
		if (gtk_tree_model_iter_parent(model, &parent, &iter)) {
			/* we were below a network */
			gtk_tree_model_get(model, &parent, COL_PTR,
					   &network, -1);
			if (server->chatnet == NULL ||
			    g_strcasecmp(server->chatnet, network->name) != 0)
				append = TRUE;
		} else {
			/* we didn't have network */
			if (server->chatnet != NULL)
				append = TRUE;
		}

		if (append)
			gtk_tree_store_remove(server_store, &iter);
	}

	if (append) {
		network = server->chatnet == NULL ? NULL :
			chatnet_find(server->chatnet);

		if (network != NULL &&
		    store_find(server_store, &parent, network))
			gtk_tree_store_append(server_store, &iter, &parent);
		else
			gtk_tree_store_append(server_store, &iter, NULL);
	}

	store_setup_server(server_store, &iter, server);
}

static void sig_server_removed(ServerConfig *server)
{
	GtkTreeIter iter;

	if (store_find(server_store, &iter, server))
		gtk_tree_store_remove(server_store, &iter);
}

static void sig_network_added(NetworkConfig *network)
{
	GtkTreeIter iter;

	if (!store_find(server_store, &iter, network))
		gtk_tree_store_append(server_store, &iter, NULL);
        store_setup_network(server_store, &iter, network);
}

static void sig_network_removed(NetworkConfig *network)
{
	GtkTreeIter iter;

	if (store_find(server_store, &iter, network))
		gtk_tree_store_remove(server_store, &iter);
}

static void setup_server_signals_init(void)
{
	signal_add("server setup updated", (SIGNAL_FUNC) sig_server_added);
	signal_add("server setup destroyed", (SIGNAL_FUNC) sig_server_removed);
	signal_add("chatnet created", (SIGNAL_FUNC) sig_network_added);
	signal_add("chatnet removed", (SIGNAL_FUNC) sig_network_removed);
}

static void setup_server_signals_deinit(void)
{
	signal_remove("server setup updated", (SIGNAL_FUNC) sig_server_added);
	signal_remove("server setup destroyed", (SIGNAL_FUNC) sig_server_removed);
	signal_remove("chatnet created", (SIGNAL_FUNC) sig_network_added);
	signal_remove("chatnet removed", (SIGNAL_FUNC) sig_network_removed);
}
