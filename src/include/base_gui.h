#include "panes.h"

Eina_Bool base_gui_init(void);
void base_gui_show(void);
Evas_Object *base_win_get(void);
Evas_Object *base_layout_get(void);
void base_win_resize_object_add(Evas_Object *resize_obj);
void base_title_set(const char *path);
void base_statusbar_toggle(Eina_Bool config);
void base_tools_toggle(Eina_Bool config);
void base_tools_set(Evas_Object *tools);
void base_text_editor_full_view(void);
void base_live_view_full_view(void);
void base_editors_full_view(void);
void base_console_full_view(void);
void base_console_toggle(Eina_Bool config);
void base_live_view_set(Evas_Object *live_view);
void base_text_editor_set(Evas_Object *text_editor);
void base_gui_term(void);
