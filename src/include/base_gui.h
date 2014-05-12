#include "panes.h"

Eina_Bool base_gui_init();
void base_gui_show();
Evas_Object *base_win_get();
Evas_Object *base_layout_get();
void base_win_resize_object_add(Evas_Object *resize_obj);
void base_title_set(const char *path);
void base_statusbar_toggle(Eina_Bool config);
void base_tools_toggle(Eina_Bool config);
void base_tools_set(Evas_Object *tools);
void base_full_view_left();
void base_full_view_right();
void base_left_view_set(Evas_Object *left);
void base_right_view_set(Evas_Object *right);
void base_gui_term();
