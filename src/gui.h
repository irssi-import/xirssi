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
void gui_entry_update(GObject *table, const char *name, char **value);

#endif
