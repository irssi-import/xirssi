#ifndef __GUI_H
#define __GUI_H

/* popup modal error message dialog */
void gui_popup_error(const char *format, ...);

/* for settings: add a label/entry to specified position with default value */
GtkWidget *gui_table_add_entry(GtkTable *table, int x, int y,
			       const char *label_text,
			       const char *name, const char *value);
/* data and name parameters should be the same as given to
   gui_table_add_entry(). value is a pointer to g_strdup()'ed string
   which is replaced with the new value, or set to NULL if value was "". */
void gui_entry_update(GObject *object, const char *name, char **value);

void gui_entry_set_from(GObject *object, const char *key, const char *value);
void gui_toggle_set_from(GObject *object, const char *key, gboolean value);
void gui_spin_set_from(GObject *object, const char *key, int value);

/* GtkTreeView helper functions */

/* Returns TRUE if row should be deleted */
typedef gboolean (*TreeSelectionDeleteFunc) (GtkTreeModel *, GtkTreeIter *,
					     GtkTreePath *, void *user_data);

/* Return GtkTreePaths for all selected rows */
GSList *gui_tree_selection_get_paths(GtkTreeView *view);

/* Delete selected rows. If func is NULL, all selected rows are deleted. */
void gui_tree_selection_delete(GtkTreeModel *model, GtkTreeView *view,
			       TreeSelectionDeleteFunc func, void *user_data);

/* Sort function for case-insensitive sorting */
gint gui_tree_strcase_sort_func(GtkTreeModel *model,
				GtkTreeIter *a, GtkTreeIter *b,
				gpointer user_data);

/* Find GtkTreeIter for specified data */
gboolean gui_tree_model_find(GtkTreeModel *model, int column,
			     GtkTreeIter *iter, void *data);

/* Returns all the rows in specified column as a string. */
char *gui_tree_model_get_string(GtkTreeModel *model, int column,
				const char *separator);

#endif
