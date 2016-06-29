typedef struct _Enventor_Item_Data Enventor_Item;

EAPI Evas_Object *enventor_object_add(Evas_Object *parent);

//FIXME: Should be eofied.
EAPI Enventor_Item *enventor_object_main_file_set(Evas_Object *obj, const char *file);
EAPI Evas_Object *enventor_item_editor_get(const Enventor_Item *it);
EAPI const char *enventor_item_file_get(const Enventor_Item *it);

#include "enventor_object.eo.legacy.h"
