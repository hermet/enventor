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

typedef struct wireframes_obj_s
{
   Evas_Object *layout;
   Eina_List *part_list;
   Ecore_Animator *animator;
} wireframes_obj;

const char *OUTLINEOBJ = "wireframes_obj";
const char *OUTLINE_EDIT_LAYOUT_KEY = "edit_layout";

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
wireframes_objs_update(wireframes_obj *wireframes)
{
   Eina_List *parts = edje_edit_parts_list_get(wireframes->layout);
   Eina_List *l, *l_next, *l2;
   char *part_name;
   part_obj *po;
   Eina_Bool removed;

   //Remove the wireframes objects that parts are removed.
   EINA_LIST_FOREACH_SAFE(wireframes->part_list, l, l_next, po)
     {
        removed = EINA_TRUE;

        EINA_LIST_FOREACH(parts, l2, part_name)
          {
             if (!part_name || !po->name[0]) continue;
             if (po->name[0] != part_name[0]) continue;
             if ((strlen(po->name) != strlen(part_name))) continue;
             if (!strcmp(po->name, part_name))
               {
                  removed = EINA_FALSE;
                  break;
               }
          }
        if (removed)
          {
             evas_object_del(po->obj);
             eina_stringshare_del(po->name);
             wireframes->part_list = eina_list_remove_list(wireframes->part_list, l);
             free(po);
          }
     }

   //Trick!. set smart members of actual live view object.
   Evas_Object *scroller = view_obj_get(VIEW_DATA);
   if (!scroller) goto end;
   Evas_Object *o = elm_object_content_get(scroller);
   if (!o) goto end;
   Evas_Object *o2 =
      elm_object_part_content_get(o, "elm.swallow.content");
   if (!o2) goto end;
   Evas *evas = evas_object_evas_get(scroller);

   //Add new part object or Update changed part.
   EINA_LIST_FOREACH(parts, l, part_name)
     {
        Eina_List *part_l;
        Evas_Object *pobj = NULL;
        int part_x = 0, part_y = 0, part_w = 0, part_h = 0, part_lx = 0, part_ly = 0;

        EINA_LIST_FOREACH(wireframes->part_list, part_l, po)
        {
           if (po->name == part_name)
             {
                pobj = po->obj;
                break;
             }
        }
        if (!pobj)
          {
             pobj = edje_object_add(scroller);
             edje_object_file_set(pobj, EDJE_PATH, "wireframes");
             evas_object_smart_member_add(pobj, o2);
             po = malloc(sizeof(part_obj));
             po->obj = pobj;
             po->name = eina_stringshare_add(part_name);
             wireframes->part_list =
                eina_list_append(wireframes->part_list, po);
             evas_object_show(pobj);
             evas_object_data_set(pobj, OUTLINE_EDIT_LAYOUT_KEY,
                                  wireframes->layout);
          }
         evas_object_geometry_get(wireframes->layout, &part_lx, &part_ly,
                                  NULL, NULL);
         edje_object_part_geometry_get(wireframes->layout, part_name,
                                       &part_x, &part_y, &part_w, &part_h);
         evas_object_resize(pobj, part_w, part_h);
         evas_object_move(pobj, part_lx + part_x, part_ly + part_y);
     }
end:
   edje_edit_string_list_free(parts);
}

static void
layout_geom_changed_cb(void *data, Evas *evas EINA_UNUSED,
                       Evas_Object *obj, void *ei EINA_UNUSED)
{
   wireframes_obj *wireframes = (wireframes_obj *)data;

   Eina_List *spacer_l;
   part_obj *po;
   int x = 0, y = 0, w = 0, h = 0, lx = 0, ly = 0;

   evas_object_geometry_get(obj, &lx, &ly, NULL, NULL);

   EINA_LIST_FOREACH(wireframes->part_list, spacer_l, po)
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
   wireframes_obj *wireframes = data;
   wireframes_objs_update(wireframes);
   wireframes->animator = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
layout_del_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   wireframes_obj_del(obj);
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
wireframes_obj_update(Evas_Object *layout)
{
   wireframes_obj *wireframes = evas_object_data_get(layout, OUTLINEOBJ);
   if (!wireframes) return;
   wireframes_objs_update(wireframes);
}

void
wireframes_obj_new(Evas_Object *layout)
{
   if (!layout) return;

   wireframes_obj *wireframes = evas_object_data_get(layout, OUTLINEOBJ);
   if (wireframes) return;

   wireframes = calloc(1, sizeof(wireframes_obj));
   if (!wireframes)
     {
        mem_fail_msg();
        return;
     }

   Ecore_Animator *animator = ecore_animator_add(animator_cb, wireframes);
   evas_object_data_set(layout, OUTLINEOBJ, wireframes);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, layout_del_cb,
                                  wireframes);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_RESIZE,
                                  layout_geom_changed_cb, wireframes);
   evas_object_event_callback_add(layout, EVAS_CALLBACK_MOVE,
                                  layout_geom_changed_cb, wireframes);
   wireframes->layout = layout;
   wireframes->animator = animator;
}

void
wireframes_obj_del(Evas_Object *layout)
{
   wireframes_obj *wireframes = evas_object_data_get(layout, OUTLINEOBJ);
   if (!wireframes) return;

   evas_object_event_callback_del_full(layout, EVAS_CALLBACK_RESIZE,
                                       layout_geom_changed_cb, wireframes);
   evas_object_event_callback_del_full(layout, EVAS_CALLBACK_MOVE,
                                       layout_geom_changed_cb, wireframes);

   part_obj *po;
   EINA_LIST_FREE(wireframes->part_list, po)
     {
        evas_object_del(po->obj);
        eina_stringshare_del(po->name);
        free(po);
     }

   ecore_animator_del(wireframes->animator);
   free(wireframes);

   evas_object_data_set(layout, OUTLINEOBJ, NULL);
   evas_object_event_callback_del(layout, EVAS_CALLBACK_DEL, layout_del_cb);
}
