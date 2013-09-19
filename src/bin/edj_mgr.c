#include <Elementary.h>
#include "common.h"

static edj_mgr *g_em;

struct edj_mgr_s
{
   Eina_List *vds;
   view_data *vd;
};

edj_mgr *
edj_mgr_init()
{
   edj_mgr *em = calloc(1, sizeof(edj_mgr));
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
edj_mgr_view_new(edj_mgr *em, Evas_Object *parent, const char *group,
                 stats_data *sd, config_data *cd)
{
   view_data *vd = edj_mgr_view_get(em, group);

   if (!vd) vd = view_init(parent, group, sd, cd);
   if (!vd) return NULL;

   if (em->vd) evas_object_hide(view_obj_get(em->vd));

   em->vds = eina_list_append(em->vds, vd);
   em->vd = vd;

   return vd;
}

view_data *
edj_mgr_view_switch_to(edj_mgr *em, const char *group)
{
   view_data *vd = edj_mgr_view_get(em, group);
   if (!vd) return NULL;

   em->vd  = vd;

   return vd;
}
