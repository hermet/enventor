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

typedef struct outline_obj_s
{
   Evas_Object *layout;
   Eina_List *part_list;
   Ecore_Animator *animator;
} outline_obj;

const char *OUTLINEOBJ = "outline_obj";
const char *OUTLINE_EDIT_LAYOUT_KEY = "edit_layout";

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/
static void
edje_part_clicked(void *data, Evas *e EINA_UNUSED,
                  Evas_Object *obj, void *ei EINA_UNUSED)
{
   part_obj *po = (part_obj *)data;
   Evas_Object *layout = evas_object_data_get(obj, OUTLINE_EDIT_LAYOUT_KEY);
}

static void
outline_objs_update(outline_obj *outline)
{
   Eina_List *parts = edje_edit_parts_list_get(outline->layout);
   Eina_List *l, *l_next, *l2;
   char *part_name;
   Edje_Part_Type type = EDJE_PART_TYPE_NONE;
   part_obj *po;
   Evas *evas = evas_object_evas_get(outline->layout);
   Eina_Bool removed;

   //Remove the outline objects that parts are removed.
   EINA_LIST_FOREACH_SAFE(outline->part_list, l, l_next, po)
     {
        removed = EINA_TRUE;

        EINA_LIST_FOREACH(parts, l2, part_name)
          {
             if (!part_name || !po->name[0]) continue;
             if (po->name[0] != part_name[0]) continue;
             if ((strlen(po->name) != strlen(part_name))) continue;
             if (!strcmp(po->name, part_name))
               {
                  type = edje_edit_part_type_get(outline->layout, part_name);
                  removed = EINA_FALSE;
                  break;
               }
          }
        if (removed)
          {
             evas_object_del(po->obj);
             eina_stringshare_del(po->name);
             outline->part_list = eina_list_remove_list(outline->part_list, l);
             free(po);
          }
     }

   //Add new part object or Update changed part.
   EINA_LIST_FOREACH(parts, l, part_name)
     {
        type = edje_edit_part_type_get(outline->layout, part_name);

        Eina_List *part_l;
        Evas_Object *pobj = NULL;
        int part_x = 0, part_y = 0, part_w = 0, part_h = 0, part_lx = 0, part_ly = 0;

        EINA_LIST_FOREACH(outline->part_list, part_l, po)
        {
           if (po->name == part_name)
             {
                pobj = po->obj;
                break;
             }
        }
        if (!pobj)
          {
             Evas_Object *scroller = view_obj_get(VIEW_DATA);
             Evas_Object *scroller_edje = elm_layout_edje_get(scroller);
             Evas_Object *clipper =
                (Evas_Object *)edje_object_part_object_get(scroller_edje,
                                                           "clipper");

             pobj = elm_layout_add(scroller);
             elm_layout_file_set(pobj, EDJE_PATH, "outline");
             evas_object_smart_member_add(pobj, scroller);

             po = malloc(sizeof(part_obj));
             po->obj = pobj;
             po->name = eina_stringshare_add(part_name);
             outline->part_list = eina_list_append(outline->part_list, po);
             evas_object_show(pobj);
             evas_object_clip_set(pobj, clipper);
             evas_object_data_set(pobj, OUTLINE_EDIT_LAYOUT_KEY,
                                  outline->layout);

             evas_object_event_callback_add(pobj, EVAS_CALLBACK_MOUSE_DOWN,
                                            edje_part_clicked, po);
          }
         evas_object_geometry_get(outline->layout, &part_lx, &part_ly,
                                  NULL, NULL);
         edje_object_part_geometry_get(outline->layout, part_name,
                                       &part_x, &part_y, &part_w, &part_h);
         evas_object_resize(pobj, part_w, part_h);
         evas_object_move(pobj, part_lx + part_x, part_ly + part_y);
     }

   edje_edit_string_list_free(parts);
}

static void
layout_geom_changed_cb(void *data, Evas *evas EINA_UNUSED,
                       Evas_Object *obj, void *ei EINA_UNUSED)
{
   outline_obj *outline = (outline_obj *)data;

   Eina_List *spacer_l;
   part_obj *po;
   int x = 0, y = 0, w = 0, h = 0, lx = 0, ly = 0;

   evas_object_geometry_get(obj, &lx, &ly, NULL, NULL);

   EINA_LIST_FOREACH(outline->part_list, spacer_l, po)
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
   outline_obj *outline = data;
   outline_objs_update(outline);
   outline->animator = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
layout_del_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   outline_obj_del(obj);
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
outline_obj_update(Evas_Object *layout)
{
   outline_obj *outline = evas_object_data_get(layout, OUTLINEOBJ);
   if (!outline) return;
   outline_objs_update(outline);
}

void
outline_obj_new(Evas_Object *layout)
{
   if (!layout) return;

   outline_obj *outline = evas_object_data_get(layout, OUTLINEOBJ);
   if (outline) return;

   outline = calloc(1, sizeof(outline_obj));
   if (!outline)
     {
        mem_fail_msg();
        return;
     }

   Ecore_Animator *animator = ecore_animator_add(animator_cb, outline);
   evas_object_data_set(layout, OUTLINEOBJ, outline);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, layout_del_cb,
                                  outline);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_RESIZE,
                                  layout_geom_changed_cb, outline);
   evas_object_event_callback_add(layout, EVAS_CALLBACK_MOVE,
                                  layout_geom_changed_cb, outline);
   outline->layout = layout;
   outline->animator = animator;
}

void
outline_obj_del(Evas_Object *layout)
{
   outline_obj *outline = evas_object_data_get(layout, OUTLINEOBJ);
   if (!outline) return;

   evas_object_event_callback_del_full(layout, EVAS_CALLBACK_RESIZE,
                                       layout_geom_changed_cb, outline);
   evas_object_event_callback_del_full(layout, EVAS_CALLBACK_MOVE,
                                       layout_geom_changed_cb, outline);

   part_obj *po;
   EINA_LIST_FREE(outline->part_list, po)
     {
        evas_object_del(po->obj);
        eina_stringshare_del(po->name);
        free(po);
     }

   ecore_animator_del(outline->animator);
   free(outline);

   evas_object_data_set(layout, OUTLINEOBJ, NULL);
   evas_object_event_callback_del(layout, EVAS_CALLBACK_DEL, layout_del_cb);
}
