#include <Elementary.h>
#include "common.h"

static edj_mgr *g_em;

struct edj_mgr_s
{
   Eina_List *vds;
   view_data *vd;
   Evas_Object *layout;
};

edj_mgr *
edj_mgr_init(Evas_Object *parent)
{
   edj_mgr *em = calloc(1, sizeof(edj_mgr));
   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "viewer_layout");
   evas_object_show(layout);
   em->layout = layout;
   g_em = em;
   return em;
}

edj_mgr *
edj_mgr_get()
{
   return g_em;
}

void
edj_mgr_term(edj_mgr *em)
{
   view_data *vd;
   EINA_LIST_FREE(em->vds, vd)
     view_term(vd);
   evas_object_del(em->layout);
   free(em);
}

view_data *
edj_mgr_view_get(edj_mgr *em, Eina_Stringshare *group)
{
   if (!em) em = g_em;
   if (!group) return em->vd;

   view_data *vd;
   Eina_List *l;
   EINA_LIST_FOREACH(em->vds, l, vd)
     {
        if (view_group_name_get(vd) == group)
          {
             em->vd = vd;
             return vd;
          }
     }
   return NULL;
}

view_data *
edj_mgr_view_new(edj_mgr *em, const char *group, stats_data *sd,
                 config_data *cd)
{
   view_data *vd = edj_mgr_view_get(em, group);

   if (!vd) vd = view_init(em->layout, group, sd, cd);
   if (!vd) return NULL;

   edj_mgr_view_switch_to(em, vd);

   em->vds = eina_list_append(em->vds, vd);

   return vd;
}

view_data *
edj_mgr_view_switch_to(edj_mgr *em, view_data *vd)
{
   Evas_Object *prev =
      elm_object_part_content_unset(em->layout, "elm.swallow.content");
   elm_object_part_content_set(em->layout, "elm.swallow.content",
                               view_obj_get(vd));
   if (prev && (prev != view_obj_get(vd)))
     {
        elm_object_part_content_set(em->layout, "elm.swallow.prev", prev);
        elm_object_signal_emit(em->layout, "elm,view,switch", "");
     }
   else
     {
        elm_object_signal_emit(em->layout, "elm,view,switch,instant", "");
     }

   em->vd  = vd;

   return vd;
}

Evas_Object *
edj_mgr_obj_get(edj_mgr *em)
{
   return em->layout;
}
