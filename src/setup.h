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

/* mark the setup to have changes */
void setup_set_changed(Setup *setup);

/* apply setting changes */
void setup_apply_changes(Setup *setup);


/* show the main preferences dialog */
extern GtkWidget *setup_dialog;

void setup_preferences_show(void);
void setup_preferences_destroy(void);

#endif
