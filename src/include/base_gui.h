#include "panes.h"

Eina_Bool base_gui_init();
Evas_Object *base_win_get();
Evas_Object *base_layout_get();
void base_win_resize_object_add(Evas_Object *resize_obj);
void base_title_set(const char *path);
void base_statusbar_toggle();
void base_hotkey_toggle();
void base_hotkeys_set(Evas_Object *hotkeys);
void base_full_view_left();
void base_full_view_right();
void base_left_view_set();
void base_gui_term();
void base_right_view_set();
