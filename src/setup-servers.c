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

static GtkWidget *server_dialog = NULL;
static GtkTreeStore *server_store = NULL;
static Setup *server_setup;

static gboolean event_destroy(GtkWidget *widget)
{
        setup_server_signals_deinit();
	setup_destroy(server_setup);

	server_dialog = NULL;
	server_setup = NULL;
	return FALSE;
}

static void event_response(GtkWidget *widget, int response_id)
{
	/* ok / cancel pressed */
	if (response_id == GTK_RESPONSE_OK)
                setup_apply_changes(server_setup);

	gtk_widget_destroy(widget);
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

	gtk_tree_model_get(model, iter, 0, &tree_data, -1);
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
	gtk_tree_model_get(model, iter, 0, &data, -1);

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

static void selection_get_paths(GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, gpointer data)
{
	GSList **paths = data;

	*paths = g_slist_prepend(*paths, gtk_tree_path_copy(path));
}

static gboolean event_remove(GtkWidget *widget, GtkTreeView *view)
{
	GtkTreeModel *model;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GSList *paths;
	void *data;

	/* get paths of selected rows */
	paths = NULL;
	sel = gtk_tree_view_get_selection(view);
        gtk_tree_selection_selected_foreach(sel, selection_get_paths, &paths);

	/* remove the rows */
	model = GTK_TREE_MODEL(server_store);
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

	setup_set_changed(server_setup);
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

	gtk_tree_model_get(model, iter, 0, &iter_data, -1);
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

static void server_store_fill_server(GtkTreeStore *store, GtkTreeIter *parent,
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
		server_store_fill_server(store, &iter, network);
	}
}

static GList *get_chat_protocol_names(void)
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

void setup_server_add_protocol_widget(GtkTable *table, int y,
				      ChatProtocol *proto,
				      int default_proto)
{
	GtkWidget *label, *combo;
	GList *list;

	label = gtk_label_new("Protocol");
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(table, label, 0, 1, y, y+1, GTK_FILL, 0, 0, 0);

	if (proto == NULL) {
		/* allow changing only if it's new */
		combo = gtk_combo_new();
		gtk_table_attach_defaults(table, combo, 1, 2, y, y+1);

		list = get_chat_protocol_names();
		gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
		g_list_free(list);

		gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(combo)->entry),
				       FALSE);
		g_object_set_data(G_OBJECT(table), "protocol",
				  GTK_COMBO(combo)->entry);
	} else {
		label = gtk_label_new(proto->name);
		gtk_table_attach_defaults(table, label, 1, 2, y, y+1);
	}
}

static GtkWidget *server_tree_new(void)
{
	GtkWidget *frame, *hbox, *sw, *view, *buttonbox, *button;
	GtkTreeView *tree_view;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	hbox = gtk_hbox_new(FALSE, 7);

	/* scrolled window */
	frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), sw);

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

	/* tree view */
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(server_store));
	g_signal_connect(G_OBJECT(view), "button_press_event",
			 G_CALLBACK(event_button_press), NULL);
	gtk_container_add(GTK_CONTAINER(sw), view);

	tree_view = GTK_TREE_VIEW(view);
	/*gtk_tree_view_set_reorderable(tree_view, TRUE);*/
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tree_view),
				    GTK_SELECTION_MULTIPLE);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes("Name", renderer,
							  "text", COL_NAME,
							  NULL);
	gtk_tree_view_append_column(tree_view, column);
	gtk_tree_view_column_set_min_width(column, 200);

	column = gtk_tree_view_column_new_with_attributes("Port", renderer,
							  "text", COL_PORT,
							  NULL);
	gtk_tree_view_append_column(tree_view, column);

	column = gtk_tree_view_column_new_with_attributes("Protocol", renderer,
							  "text", COL_PROTOCOL,
							  NULL);
	gtk_tree_view_append_column(tree_view, column);

	/* add/edit/remove */
	buttonbox = gtk_vbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox),
				  GTK_BUTTONBOX_START);
	gtk_box_set_spacing(GTK_BOX(buttonbox), 5);
	gtk_box_pack_start(GTK_BOX(hbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new_with_label("Add Network...");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_add_network), tree_view);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);

	button = gtk_button_new_with_label("Add Server...");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_add_server), tree_view);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);

	button = gtk_button_new_with_label("Edit...");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_edit), tree_view);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);

	button = gtk_button_new_with_label("Remove");
	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(event_remove), tree_view);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);

	return hbox;
}

void setup_servers_show(void)
{
	GtkWidget *dialog, *vbox, *vbox2, *hbox, *frame, *table;
	GtkWidget *tree, *label, *checkbox, *spin, *entry;

	if (server_dialog != NULL) {
		gdk_window_raise(server_dialog->window);
		return;
	}

	server_dialog = dialog =
		gtk_dialog_new_with_buttons("Server Settings", NULL, 0,
					    GTK_STOCK_OK, GTK_RESPONSE_OK,
					    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					    NULL);
	gtk_widget_set_size_request(dialog, -1, 550);
	server_setup = setup_create(NULL, NULL);

	g_signal_connect(G_OBJECT(dialog), "destroy",
			 G_CALLBACK(event_destroy), NULL);
	g_signal_connect(G_OBJECT(dialog), "response",
			 G_CALLBACK(event_response), NULL);

	vbox = gtk_vbox_new(FALSE, 7);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 7);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
			   vbox, TRUE, TRUE, 0);

	/* servers list */
	frame = gtk_frame_new("Servers");
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 7);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	tree = server_tree_new();
	gtk_box_pack_start(GTK_BOX(vbox2), tree, TRUE, TRUE, 0);

	/* motd */
	checkbox = gtk_check_button_new_with_label("Skip MOTD while connecting");
	gtk_widget_set_name(checkbox, "skip_motd");
	gtk_box_pack_start(GTK_BOX(vbox2), checkbox, FALSE, FALSE, 0);

	/* IPv6 */
	checkbox = gtk_check_button_new_with_label("Prefer IPv6 addresses with DNS lookups");
	gtk_widget_set_name(checkbox, "resolve_prefer_ipv6");
	gtk_box_pack_start(GTK_BOX(vbox2), checkbox, FALSE, FALSE, 0);

	/* reconnect wait */
	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	label = gtk_label_new("Seconds to wait before reconnecting (-1 = never reconnect)");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	spin = gtk_spin_button_new(NULL, 0, 0);
        gtk_spin_button_set_range(GTK_SPIN_BUTTON(spin), -1, G_MAXINT);
        gtk_spin_button_set_increments(GTK_SPIN_BUTTON(spin), 1, 60);
	gtk_widget_set_name(spin, "server_reconnect_time");
	gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 0);

	/* source host */
	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	label = gtk_label_new("Source host (\"virtual host\")");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	entry = gtk_entry_new();
	gtk_widget_set_name(entry, "hostname");
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

	/* user information */
	frame = gtk_frame_new("User Information");
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

	table = gtk_table_new(3, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_set_border_width(GTK_CONTAINER(table), 7);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 7);

	label = gtk_label_new("Nick");
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 GTK_FILL, 0, 0, 0);

	entry = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry), 10);
	gtk_widget_set_name(entry, "nick");
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1,
			 GTK_FILL, 0, 0, 0);

	label = gtk_label_new("Alt. Nick");
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			 GTK_FILL, 0, 0, 0);

	entry = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry), 10);
	gtk_widget_set_name(entry, "alternate_nick");
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 1, 2,
			 GTK_FILL, 0, 0, 0);

	label = gtk_label_new("User Name");
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
			 GTK_FILL, 0, 0, 0);

	entry = gtk_entry_new();
	gtk_widget_set_name(entry, "user_name");
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 3, 4, 0, 1);

	label = gtk_label_new("Real Name");
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 1, 2,
			 GTK_FILL, 0, 0, 0);

	entry = gtk_entry_new();
	gtk_widget_set_name(entry, "real_name");
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 3, 4, 1, 2);

	gtk_widget_show_all(dialog);

	setup_register(server_setup, dialog);

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
			gtk_tree_model_get(model, &parent, 0, &network, -1);
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
