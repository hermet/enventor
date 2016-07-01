typedef struct _Enventor_Item_Data Enventor_Item;

EAPI Evas_Object *enventor_object_add(Evas_Object *parent);

//FIXME: Should be eofied.
EAPI Enventor_Item *enventor_object_main_item_set(Evas_Object *obj, const char *file);
EAPI Enventor_Item *enventor_object_sub_item_add(Evas_Object *obj, const char *file);
EAPI Enventor_Item *enventor_object_main_item_get(const Evas_Object *obj);
EAPI const Eina_List *enventor_object_sub_items_get(const Evas_Object *obj);
EAPI Evas_Object *enventor_item_editor_get(const Enventor_Item *it);
EAPI const char *enventor_item_file_get(const Enventor_Item *it);
EAPI Enventor_Item *enventor_object_focused_item_get(const Evas_Object *obj);
EAPI Eina_Bool enventor_item_focus_set(Enventor_Item *it);

#include "enventor_object.eo.legacy.h"
