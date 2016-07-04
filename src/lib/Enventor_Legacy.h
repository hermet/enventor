typedef struct _Enventor_Item_Data Enventor_Item;

EAPI Evas_Object *enventor_object_add(Evas_Object *parent);

//FIXME: Eofy.
EAPI Enventor_Item *enventor_object_main_item_set(Evas_Object *obj, const char *file);
EAPI Enventor_Item *enventor_object_sub_item_add(Evas_Object *obj, const char *file);
EAPI Enventor_Item *enventor_object_main_item_get(const Evas_Object *obj);
EAPI const Eina_List *enventor_object_sub_items_get(const Evas_Object *obj);
EAPI Evas_Object *enventor_item_editor_get(const Enventor_Item *it);
EAPI const char *enventor_item_file_get(const Enventor_Item *it);
EAPI Enventor_Item *enventor_object_focused_item_get(const Evas_Object *obj);
EAPI Eina_Bool enventor_item_focus_set(Enventor_Item *it);
EAPI int enventor_item_max_line_get(const Enventor_Item *it);
EAPI Eina_Bool enventor_item_line_goto(Enventor_Item *it, int line);
EAPI Eina_Bool enventor_item_syntax_color_full_apply(Enventor_Item *it, Eina_Bool force);
EAPI Eina_Bool enventor_item_syntax_color_partial_apply(Enventor_Item *it, double interval);
EAPI Eina_Bool enventor_item_select_region_set(Enventor_Item *it, int start, int end);
EAPI Eina_Bool enventor_item_select_none(Enventor_Item *it);
EAPI Eina_Bool enventor_item_cursor_pos_set(Enventor_Item *it, int position);
EAPI int enventor_item_cursor_pos_get(const Enventor_Item *it);
EAPI const char *enventor_item_selection_get(const Enventor_Item *it);
EAPI Eina_Bool enventor_item_text_insert(Enventor_Item *it, const char *text);
EAPI const char * enventor_item_text_get(const Enventor_Item *it);
EAPI Eina_Bool enventor_item_line_delete(Enventor_Item *it);
EAPI Eina_Bool enventor_item_file_save(Enventor_Item *it, const char *file);
EAPI Eina_Bool enventor_item_modified_get(const Enventor_Item *it);
EAPI void enventor_item_modified_set(Enventor_Item *it, Eina_Bool modified);
EAPI Eina_Bool enventor_item_del(Enventor_Item *it);

#include "enventor_object.eo.legacy.h"
