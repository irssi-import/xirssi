/*
 gui.c : some misc GUI-related functions

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
#include "gui.h"

void gui_popup_error(const char *format, ...)
{
	GtkWidget *dialog;
	va_list va;
	char *str;

	va_start(va, format);
	str = g_strdup_vprintf(format, va);
	va_end(va);

	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK, "%s", str);
	g_signal_connect(G_OBJECT(dialog), "response",
			 G_CALLBACK(gtk_widget_destroy), NULL);
	g_free(str);

	gtk_widget_show(dialog);
}

GtkWidget *gui_table_add_entry(GtkTable *table, int x, int y,
			       const char *label_text,
			       const char *name, const char *value)
{
	GtkWidget *label, *entry;

	label = gtk_label_new(label_text);
	gtk_misc_set_alignment(GTK_MISC(label), 1, .5);
	gtk_table_attach(GTK_TABLE(table), label, x, x+1, y, y+1,
			 GTK_FILL, 0, 0, 0);

	entry = gtk_entry_new();
	gtk_table_attach_defaults(GTK_TABLE(table), entry, x+1, x+2, y, y+1);

	if (value != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), value);

	g_object_set_data(G_OBJECT(table), name, entry);
	return entry;
}

void gui_entry_update(GObject *object, const char *name, char **value)
{
	GtkEntry *entry;
	const char *entry_text;

	entry = g_object_get_data(object, name);
        entry_text = gtk_entry_get_text(entry);

	g_free(*value);
	*value = *entry_text == '\0' ? NULL : g_strdup(entry_text);
}

void gui_entry_set_from(GObject *object, const char *key, const char *value)
{
	GtkEntry *entry;

	entry = g_object_get_data(object, key);
	if (value != NULL)
		gtk_entry_set_text(entry, value);
}

void gui_toggle_set_from(GObject *object, const char *key, gboolean value)
{
	GtkToggleButton *toggle;

	toggle = g_object_get_data(object, key);
	gtk_toggle_button_set_active(toggle, value);
}

void gui_spin_set_from(GObject *object, const char *key, int value)
{
	GtkSpinButton *spin;

	spin = g_object_get_data(object, key);
	gtk_spin_button_set_value(spin, value);
}

static void selection_get_paths(GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, gpointer data)
{
	GSList **paths = data;

	*paths = g_slist_prepend(*paths, gtk_tree_path_copy(path));
}

GSList *gui_tree_selection_get_paths(GtkTreeView *view)
{
	GtkTreeSelection *sel;
	GSList *paths;

	/* get paths of selected rows */
	paths = NULL;
	sel = gtk_tree_view_get_selection(view);
	gtk_tree_selection_selected_foreach(sel, selection_get_paths, &paths);

	return paths;
}

void gui_tree_selection_delete(GtkTreeModel *model, GtkTreeView *view,
			       TreeSelectionDeleteFunc func, void *user_data)
{
	GtkTreeIter iter;
	GSList *paths;
	int liststore;

	g_return_if_fail(model != NULL);
	g_return_if_fail(view != NULL);

	if (GTK_IS_LIST_STORE(model))
		liststore = TRUE;
	else if (GTK_IS_TREE_STORE(model))
		liststore = FALSE;
	else {
		g_warning("gui_tree_selection_delete(): unknown tree model");
		return;
	}

	paths =  gui_tree_selection_get_paths(view);
	while (paths != NULL) {
		GtkTreePath *path = paths->data;

		/* remove from tree */
		gtk_tree_model_get_iter(model, &iter, path);
		if (func == NULL || func(model, &iter, path, user_data)) {
			if (liststore) {
				gtk_list_store_remove(GTK_LIST_STORE(model),
						      &iter);
			} else {
				gtk_tree_store_remove(GTK_TREE_STORE(model),
						      &iter);
			}
		}

		paths = g_slist_remove(paths, path);
		gtk_tree_path_free(path);
	}
}

gint gui_tree_strcase_sort_func(GtkTreeModel *model,
				GtkTreeIter *a, GtkTreeIter *b,
				gpointer user_data)
{
	int column = GPOINTER_TO_INT(user_data);
	GValue a_value = { 0, };
	GValue b_value = { 0, };
	int ret;

	gtk_tree_model_get_value(model, a, column, &a_value);
	gtk_tree_model_get_value(model, b, column, &b_value);

	ret = strcasecmp(g_value_get_string(&a_value),
			 g_value_get_string(&b_value));

	g_value_unset(&a_value);
	g_value_unset(&b_value);

	return ret;
}

typedef struct {
	gboolean found;
	int column;
	GtkTreeIter *iter;
	void *data;
} StoreFind;

static gboolean store_find_func(GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, gpointer data)
{
	StoreFind *rec = data;
	void *iter_data;

	gtk_tree_model_get(model, iter, rec->column, &iter_data, -1);
	if (iter_data == rec->data) {
		rec->found = TRUE;
		memcpy(rec->iter, iter, sizeof(GtkTreeIter));
	}

	return rec->found;
}

gboolean gui_tree_model_find(GtkTreeModel *model, int column,
			     GtkTreeIter *iter, void *data)
{
	StoreFind rec;

	rec.found = FALSE;
	rec.column = column;
	rec.iter = iter;
	rec.data = data;
	gtk_tree_model_foreach(model, store_find_func, &rec);

	return rec.found;
}

char *gui_tree_model_get_string(GtkTreeModel *model, int column,
				const char *separator)
{
	GtkTreeIter iter;
	GString *str;
	char *iter_str, *ret;

	if (!gtk_tree_model_get_iter_first(model, &iter))
		return NULL;

	str = g_string_new(NULL);
	do {
		gtk_tree_model_get(model, &iter, column, &iter_str, -1);
		if (str->len != 0)
			g_string_append(str, separator);
		g_string_append(str, iter_str);
		g_free(iter_str);
	} while (gtk_tree_model_iter_next(model, &iter));

	ret = str->str;
	g_string_free(str, FALSE);

	return ret;
}
