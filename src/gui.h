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

gint gui_tree_strcase_sort_func(GtkTreeModel *model,
				GtkTreeIter *a, GtkTreeIter *b,
				gpointer user_data);

GSList *gui_tree_selection_get_paths(GtkTreeView *view);

#endif
