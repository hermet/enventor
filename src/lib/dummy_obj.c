#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT 1

#include <Enventor.h>
#include <Edje_Edit.h>
#include "enventor_private.h"

typedef struct part_obj_s
{
   Evas_Object *obj;
   Eina_Stringshare *name;
} part_obj;

typedef struct dummy_obj_s
{
   Evas_Object *layout;
   Eina_List *swallows;
   Eina_List *spacers;
   Ecore_Animator *animator;
} dummy_obj;

const char *DUMMYOBJ = "dummy_obj";
const char *EDIT_LAYOUT_KEY = "edit_layout";

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/
static void
edje_part_clicked(void *data, Evas *e EINA_UNUSED,
                  Evas_Object *obj, void *ei EINA_UNUSED)
{
   part_obj *po = (part_obj *)data;
   Evas_Object *layout = evas_object_data_get(obj, EDIT_LAYOUT_KEY);
   evas_object_smart_callback_call(layout, "dummy,clicked", (char *)(po->name));
}

static void
dummy_objs_update(dummy_obj *dummy)
{
   Eina_List *parts = edje_edit_parts_list_get(dummy->layout);
   Eina_List *l, *l_next, *l2;
   char *part_name;
   Edje_Part_Type type = EDJE_PART_TYPE_NONE;
   part_obj *po;
   Evas *evas = evas_object_evas_get(dummy->layout);
   Eina_Bool removed;

   //Remove the fake swallow objects that parts are removed.
   EINA_LIST_FOREACH_SAFE(dummy->swallows, l, l_next, po)
     {
        removed = EINA_TRUE;

        EINA_LIST_FOREACH(parts, l2, part_name)
          {
             if (!part_name || !po->name[0]) continue;
             if (po->name[0] != part_name[0]) continue;
             if ((strlen(po->name) != strlen(part_name))) continue;
             if (!strcmp(po->name, part_name))
               {
                  type = edje_edit_part_type_get(dummy->layout, part_name);
                  if ((type == EDJE_PART_TYPE_SWALLOW))
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

   //Remove the fake swallow objects that parts are removed.
   EINA_LIST_FOREACH_SAFE(dummy->spacers, l, l_next, po)
     {
        removed = EINA_TRUE;

        EINA_LIST_FOREACH(parts, l2, part_name)
          {
             if (po->name[0] != part_name[0]) continue;
             if ((strlen(po->name) != strlen(part_name))) continue;
             if (!strcmp(po->name, part_name))
               {
                  type = edje_edit_part_type_get(dummy->layout, part_name);
                  if ((type == EDJE_PART_TYPE_SPACER))
                    removed = EINA_FALSE;
                  break;
               }
          }
        if (removed)
          {
             evas_object_del(po->obj);
             eina_stringshare_del(po->name);
             dummy->spacers = eina_list_remove_list(dummy->spacers, l);
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
             if (!po)
               {
                  EINA_LOG_ERR("Failed to allocate Memory!");
                  continue;
               }

             //New part. Add fake object.
             Evas_Object *obj = edje_object_add(evas);
             if (!edje_object_file_set(obj, EDJE_PATH, "swallow"))
               EINA_LOG_ERR("Failed to set File to Edje Object!");
             edje_object_part_swallow(dummy->layout, part_name, obj);

             po->obj = obj;
             evas_object_data_set(po->obj, EDIT_LAYOUT_KEY, dummy->layout);
             po->name = eina_stringshare_add(part_name);
             dummy->swallows = eina_list_append(dummy->swallows, po);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN,
                                            edje_part_clicked, po);
          }
        else if (type == EDJE_PART_TYPE_SPACER)
          {
             Eina_List *spacer_l;
             Evas_Object *obj = NULL;
             int x = 0, y = 0, w = 0, h = 0, lx = 0, ly = 0;

             EINA_LIST_FOREACH(dummy->spacers, spacer_l, po)
                if (po->name == part_name)
                  {
                     obj = po->obj;
                     break;
                  }
             if (!obj)
               {
                  Evas_Object *scroller = view_obj_get(VIEW_DATA);
                  Evas_Object *scroller_edje = elm_layout_edje_get(scroller);
                  Evas_Object *clipper =
                     (Evas_Object *)edje_object_part_object_get(scroller_edje,
                                                                "clipper");
                  obj = elm_layout_add(scroller);
                  elm_layout_file_set(obj, EDJE_PATH, "spacer");
                  evas_object_smart_member_add(obj, scroller);


                  po = malloc(sizeof(part_obj));
                  po->obj = obj;
                  po->name = eina_stringshare_add(part_name);
                  dummy->spacers = eina_list_append(dummy->spacers, po);
                  evas_object_show(obj);
                  evas_object_clip_set(obj, clipper);
                  evas_object_data_set(obj, EDIT_LAYOUT_KEY, dummy->layout);

                  evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN,
                                                 edje_part_clicked, po);
               }
             evas_object_geometry_get(dummy->layout, &lx, &ly, NULL, NULL);
             edje_object_part_geometry_get(dummy->layout, part_name, &x, &y, &w, &h);
             evas_object_resize(obj, w, h);
             evas_object_move(obj, lx + x, ly + y);
          }
     }

   edje_edit_string_list_free(parts);
}

static void
layout_geom_changed_cb(void *data, Evas *evas EINA_UNUSED,
                       Evas_Object *obj, void *ei EINA_UNUSED)
{
   dummy_obj *dummy = (dummy_obj *)data;

   Eina_List *spacer_l;
   part_obj *po;
   int x = 0, y = 0, w = 0, h = 0, lx = 0, ly = 0;

   evas_object_geometry_get(obj, &lx, &ly, NULL, NULL);

   EINA_LIST_FOREACH(dummy->spacers, spacer_l, po)
     if (edje_object_part_exists(obj, po->name))
       {
          edje_object_part_geometry_get(obj, po->name, &x, &y, &w, &h);
          evas_object_resize(po->obj, w, h);
          evas_object_move(po->obj, lx + x, ly + y);
       }
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

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

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
   if (!layout) return;

   dummy_obj *dummy = evas_object_data_get(layout, DUMMYOBJ);
   if (dummy) return;

   dummy = calloc(1, sizeof(dummy_obj));
   if (!dummy)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }

   Ecore_Animator *animator = ecore_animator_add(animator_cb, dummy);
   evas_object_data_set(layout, DUMMYOBJ, dummy);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, layout_del_cb,
                                  dummy);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_RESIZE,
                                  layout_geom_changed_cb, dummy);
   evas_object_event_callback_add(layout, EVAS_CALLBACK_MOVE,
                                  layout_geom_changed_cb, dummy);
   dummy->layout = layout;
   dummy->animator = animator;
}

void
dummy_obj_del(Evas_Object *layout)
{
   dummy_obj *dummy = evas_object_data_get(layout, DUMMYOBJ);
   if (!dummy) return;

   evas_object_event_callback_del_full(layout, EVAS_CALLBACK_RESIZE,
                                  layout_geom_changed_cb, dummy);
   evas_object_event_callback_del_full(layout, EVAS_CALLBACK_MOVE,
                                  layout_geom_changed_cb, dummy);

   part_obj *po;
   EINA_LIST_FREE(dummy->swallows, po)
     {
        evas_object_del(po->obj);
        eina_stringshare_del(po->name);
        free(po);
     }

   EINA_LIST_FREE(dummy->spacers, po)
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
