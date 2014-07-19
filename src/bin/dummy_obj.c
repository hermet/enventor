#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT 1

#include <Elementary.h>
#include <Edje_Edit.h>
#include "common.h"

typedef struct part_obj_s
{
   Evas_Object *obj;
   Eina_Stringshare *name;
} part_obj;

typedef struct dummy_obj_s
{
   Evas_Object *layout;
   Eina_List *swallows;
   Ecore_Animator *animator;
} dummy_obj;

const char *DUMMYOBJ = "dummy_obj";

void
dummy_objs_update(dummy_obj *dummy)
{
   Eina_List *parts = edje_edit_parts_list_get(dummy->layout);
   Eina_List *l, *l_next, *l2;
   char *part_name;
   Edje_Part_Type type;
   part_obj *po;

   Eina_Bool removed;

   //Remove the fake swallow objects that parts are removed.
   EINA_LIST_FOREACH_SAFE(dummy->swallows, l, l_next, po)
     {
        removed = EINA_TRUE;

        EINA_LIST_FOREACH(parts, l2, part_name)
          {
             if (po->name[0] != part_name[0]) continue;
             if (strlen(po->name) != strlen(part_name)) continue;
             if (!strcmp(po->name, part_name))
               {
                  if (edje_edit_part_type_get(dummy->layout, part_name) ==
                      EDJE_PART_TYPE_SWALLOW)
                    removed = EINA_FALSE;
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
        type = edje_edit_part_type_get(dummy->layout, part_name);

        if (type == EDJE_PART_TYPE_SWALLOW)
          {
             //Check this part is exist 
             if (edje_object_part_swallow_get(dummy->layout, part_name))
               continue;

             po = malloc(sizeof(part_obj));
             if (!po) continue;

             //New part. Add fake object.
             Evas_Object *obj = edje_object_add(dummy->layout);
             edje_object_file_set(obj, EDJE_PATH, "swallow");
             edje_object_part_swallow(dummy->layout, part_name, obj);

             po->obj = obj;
             po->name = eina_stringshare_add(part_name);
             dummy->swallows = eina_list_append(dummy->swallows, po);
          }
     }

   edje_edit_string_list_free(parts);
}

static Eina_Bool
animator_cb(void *data)
{
   dummy_obj *dummy = data;
   dummy_objs_update(dummy);
   dummy->animator = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
layout_del_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   dummy_obj_del(obj);
}

void
dummy_obj_update(Evas_Object *layout)
{
   dummy_obj *dummy = evas_object_data_get(layout, DUMMYOBJ);
   if (!dummy) return;
   dummy_objs_update(dummy);
}

void
dummy_obj_new(Evas_Object *layout)
{
   dummy_obj *dummy = evas_object_data_get(layout, DUMMYOBJ);
   if (dummy) return;

   dummy = calloc(1, sizeof(dummy_obj));

   Ecore_Animator *animator = ecore_animator_add(animator_cb, dummy);
   evas_object_data_set(layout, DUMMYOBJ, dummy);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, layout_del_cb,
                                  dummy);

   dummy->layout = layout;
   dummy->animator = animator;
}

void dummy_obj_del(Evas_Object *layout)
{
   dummy_obj *dummy = evas_object_data_get(layout, DUMMYOBJ);
   if (!dummy) return;

   part_obj *po;
   EINA_LIST_FREE(dummy->swallows, po)
     {
        evas_object_del(po->obj);
        eina_stringshare_del(po->name);
        free(po);
     }

   ecore_animator_del(dummy->animator);
   free(dummy);

   evas_object_data_set(layout, DUMMYOBJ, NULL);
   evas_object_event_callback_del(layout, EVAS_CALLBACK_DEL, layout_del_cb);
}
