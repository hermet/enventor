#include "panes.h"
#include "edc_navigator.h"
#include "file_browser.h"

#define EDC_NAVIGATOR_UPDATE_TIME 0.25

Eina_Bool base_gui_init(void);
void base_gui_show(void);
Evas_Object *base_win_get(void);
Evas_Object *base_layout_get(void);
Enventor_Object *base_enventor_get(void);
void base_win_resize_object_add(Evas_Object *resize_obj);
void base_title_set(const char *path);
void base_statusbar_toggle(Eina_Bool config);
void base_tools_toggle(Eina_Bool config);
void base_tools_set(Evas_Object *live_view_tools, Evas_Object *text_editor_tools);
void base_enventor_full_view(void);
void base_live_view_full_view(void);
void base_editors_full_view(void);
void base_console_auto_hide(void);
void base_console_toggle(void);
void base_live_view_set(Evas_Object *live_view);
void base_enventor_set(Enventor_Object *enventor);
void base_text_editor_set(Enventor_Item *it);
void base_gui_term(void);
void base_console_reset(void);
void base_error_msg_set(const char *msg);
void base_console_full_view(void);
void base_file_browser_toggle(Eina_Bool toggle);
void base_edc_navigator_toggle(Eina_Bool toggle);
void base_edc_navigator_group_update(void);
void base_edc_navigator_deselect(void);
