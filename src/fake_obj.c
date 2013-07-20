#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT 1

#include <Elementary.h>
#include <Edje_Edit.h>
#include "common.h"

typedef struct part_obj_s
{
   Evas_Object *obj;
   Eina_Stringshare *name;
} part_obj;

struct fake_obj_s
{
   Evas_Object *layout;
   Eina_List *swallows;
};

const char *FAKEOBJ = "fakeobj";

void
fake_objs_update(fake_obj *fo)
{
   Evas_Object *edje = elm_layout_edje_get(fo->layout);
   Eina_List *parts = edje_edit_parts_list_get(edje);
   Eina_List *l, *l_next, *l2;
   char *part_name;
   Edje_Part_Type type;
   part_obj *po;

   Eina_Bool removed = EINA_TRUE;

   //Remove the fake swallow objects that parts are removed.
   EINA_LIST_FOREACH_SAFE(fo->swallows, l, l_next, po)
     {
        EINA_LIST_FOREACH(parts, l2, part_name)
          {
             if (strlen(po->name) > strlen(part_name))
               {
                  if (strcmp(po->name, part_name)) continue;
               }
             else
               {
                  if (strcmp(part_name, po->name)) continue;
               }

             if (edje_edit_part_type_get(edje, part_name) !=
                 EDJE_PART_TYPE_SWALLOW) continue;

             removed = EINA_FALSE;
          }
        if (removed)
          {
             evas_object_del(po->obj);
             eina_stringshare_del(po->name);
             fo->swallows = eina_list_remove(fo->swallows, l);
             free(po);
          }
     }

   //Add new part object or Update changed part.
   EINA_LIST_FOREACH(parts, l, part_name)
     {
        type = edje_edit_part_type_get(edje, part_name);

        if (type == EDJE_PART_TYPE_SWALLOW)
          {
             //Check this part is exist 
             if (edje_object_part_swallow_get(edje, part_name)) continue;

             //New part. Add fake object.
             Evas_Object *obj = elm_layout_add(fo->layout);
             elm_layout_file_set(obj, EDJE_PATH, "swallow");
             evas_object_show(obj);
             elm_object_part_content_set(fo->layout, part_name, obj);

             part_obj *po = malloc(sizeof(part_obj));
             if (!po) continue;

             po->obj = obj;
             po->name = eina_stringshare_add(part_name);
             fo->swallows = eina_list_append(fo->swallows, po);
          }
     }
}

static Eina_Bool
animator_cb(void *data)
{
   fake_obj *fo = data;
   fake_objs_update(fo);
   return ECORE_CALLBACK_CANCEL;
}

static void
edje_change_file_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    const char *emission EINA_UNUSED,
                    const char *source EINA_UNUSED)
{
   fake_obj *fo = data;
   fake_objs_update(fo);
}

static void
layout_del_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   fake_obj_del(obj);
}

void fake_obj_new(Evas_Object *layout)
{
   fake_obj *fo = evas_object_data_get(layout, FAKEOBJ);
   if (fo) return;

   fo = calloc(1, sizeof(fake_obj));

   edje_object_signal_callback_add(elm_layout_edje_get(layout),
                                   "edje,change,file", "edje",
                                   edje_change_file_cb, fo);

   ecore_animator_add(animator_cb, fo);
   evas_object_data_set(layout, FAKEOBJ, fo);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, layout_del_cb, fo);

   fo->layout = layout;
}

void fake_obj_del(Evas_Object *layout)
{
   fake_obj *fo = evas_object_data_get(layout, FAKEOBJ);
   if (fo) return;

   Eina_List *l;
   part_obj *po;
   EINA_LIST_FOREACH(fo->swallows, l, po)
     {
        evas_object_del(po->obj);
        eina_stringshare_del(po->name);
        free(po);
     }
   eina_list_free(fo->swallows);

   evas_object_data_set(layout, FAKEOBJ, NULL);
   evas_object_event_callback_del(layout, EVAS_CALLBACK_DEL, layout_del_cb);
}
