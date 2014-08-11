#include <Elementary.h>
#include "common.h"

void
console_text_set(Evas_Object *console, const char *text)
{
   elm_entry_entry_set(console, text);
}

Evas_Object *
console_create(Evas_Object *parent)
{
   Evas_Object *obj = elm_entry_add(parent);
   elm_entry_scrollable_set(obj, EINA_TRUE);
   elm_entry_editable_set(obj, EINA_FALSE);
   elm_entry_line_wrap_set(obj, ELM_WRAP_WORD);
   elm_object_focus_allow_set(obj, EINA_FALSE);
   evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);

   return obj;
}
