#ifndef __SETUP_H
#define __SETUP_H

typedef struct _Setup Setup;
typedef void (*SetupChangedCallback)(Setup *setup, void *user_data);

/* create a new setup group */
Setup *setup_create(SetupChangedCallback changed_func, void *user_data);
/* destroy setup */
void setup_destroy(Setup *setup);

/* register settings in widget and it's children */
void setup_register(Setup *setup, GtkWidget *widget);

void setup_register_level_button(GtkWidget *button, GtkWidget *entry);
void setup_register_irssicolor_button(GtkWidget *button, GtkWidget *entry);
void setup_register_color_button(GtkWidget *button, GtkWidget *eventbox);
void setup_register_dir_button(GtkWidget *button, GtkWidget *entry);

/* mark the setup to have changes */
void setup_set_changed(Setup *setup);

/* apply setting changes */
void setup_apply_changes(Setup *setup);


/* show the main preferences dialog */
extern GtkWidget *setup_dialog;

void setup_preferences_show(void);
void setup_preferences_destroy(void);

/* setup-private: */
GtkListStore *setup_channels_list_init(GtkTreeView *view,
				       GtkWidget *add_button,
				       GtkWidget *remove_button);

#endif
