#ifndef __GUI_KEYBOARD_H
#define __GUI_KEYBOARD_H

char *gui_keyboard_get_event_string(GdkEventKey *event);

int gui_is_modifier(int keyval);
int gui_is_dead_key(int keyval);

void gui_keyboards_init(void);
void gui_keyboards_deinit(void);

#endif
