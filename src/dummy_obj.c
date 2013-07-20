#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT 1

#include <Elementary.h>
#include <Edje_Edit.h>
#include "common.h"

typedef struct part_obj_s
{
   Evas_Object *obj;
   Eina_Stringshare *name;
} part_obj;

struct dummy_obj_s
{
   Evas_Object *layout;
   Eina_List *swallows;
};

const char *DUMMYOBJ = "dummy_obj";

void
dummy_objs_update(dummy_obj *dummy)
{
   Evas_Object *edje = elm_layout_edje_get(dummy->layout);
   Eina_List *parts = edje_edit_parts_list_get(edje);
   Eina_List *l, *l_next, *l2;
   char *part_name;
   Edje_Part_Type type;
   part_obj *po;

   Eina_Bool removed;

   //Remove the fake swallow objects that parts are removed.
   EINA_LIST_FOREACH_SAFE(dummy->swallows, l, l_next, po)
     {
        removed = EINA_FALSE;

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
                 EDJE_PART_TYPE_SWALLOW)
               {
                  removed = EINA_TRUE;
                  break;
               }
          }
        if (removed)
          {
             evas_object_del(po->obj);
             eina_stringshare_del(po->name);
             dummy->swallows = eina_list_remove_list(dummy->swallows, l);
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

             part_obj *po = malloc(sizeof(part_obj));
             if (!po) continue;

             //New part. Add fake object.
             Evas_Object *obj = elm_layout_add(dummy->layout);
             elm_layout_file_set(obj, EDJE_PATH, "swallow");
             evas_object_show(obj);
             elm_object_part_content_set(dummy->layout, part_name, obj);

             po->obj = obj;
             po->name = eina_stringshare_add(part_name);
             dummy->swallows = eina_list_append(dummy->swallows, po);
          }
     }
}

static Eina_Bool
animator_cb(void *data)
{
   dummy_obj *dummy = data;
   dummy_objs_update(dummy);
   return ECORE_CALLBACK_CANCEL;
}

static void
edje_change_file_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    const char *emission EINA_UNUSED,
                    const char *source EINA_UNUSED)
{
   dummy_obj *dummy = data;
   dummy_objs_update(dummy);
}

static void
layout_del_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   dummy_obj_del(obj);
}

void dummy_obj_new(Evas_Object *layout)
{
   DFUNC_NAME();
   dummy_obj *dummy = evas_object_data_get(layout, DUMMYOBJ);
   if (dummy) return;

   dummy = calloc(1, sizeof(dummy_obj));

   edje_object_signal_callback_add(elm_layout_edje_get(layout),
                                   "edje,change,file", "edje",
                                   edje_change_file_cb, dummy);

   ecore_animator_add(animator_cb, dummy);
   evas_object_data_set(layout, DUMMYOBJ, dummy);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, layout_del_cb,
                                  dummy);

   dummy->layout = layout;
}

void dummy_obj_del(Evas_Object *layout)
{
   dummy_obj *dummy = evas_object_data_get(layout, DUMMYOBJ);
   if (!dummy) return;

   Eina_List *l;
   part_obj *po;
   EINA_LIST_FOREACH(dummy->swallows, l, po)
     {
        evas_object_del(po->obj);
        eina_stringshare_del(po->name);
        free(po);
     }
   eina_list_free(dummy->swallows);
   free(dummy);

   evas_object_data_set(layout, DUMMYOBJ, NULL);
   evas_object_event_callback_del(layout, EVAS_CALLBACK_DEL, layout_del_cb);
   edje_object_signal_callback_del(elm_layout_edje_get(layout),
                                   "edje,change,file", "edje",
                                   edje_change_file_cb);

}
