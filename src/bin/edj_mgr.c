#include <Elementary.h>
#include "common.h"

const double VIEW_CACHING_TIME = 60 * 30;

static edj_mgr *g_em;

typedef struct edj_data_s
{
   view_data *vd;
   Ecore_Timer *timer;
} edj_data;

struct edj_mgr_s
{
   Eina_List *edjs;
   edj_data *edj;
   Evas_Object *layout;

   Eina_Bool reload_need : 1;
};

void
edj_mgr_clear(edj_mgr *em)
{
   edj_data *edj;

   EINA_LIST_FREE(em->edjs, edj)
     {
        if (edj->timer) ecore_timer_del(edj->timer);
        view_term(edj->vd);
        free(edj);
     }
   em->edjs = NULL;
   em->edj = NULL;
   em->reload_need = EINA_FALSE;
}

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
   edj_mgr_clear(em);
   evas_object_del(em->layout);
   free(em);
}

view_data *
edj_mgr_view_get(edj_mgr *em, Eina_Stringshare *group)
{
   if (!em) em = g_em;
   if (!group && em->edj) return em->edj->vd;

   edj_data *edj;
   Eina_List *l;
   EINA_LIST_FOREACH(em->edjs, l, edj)
     {
        if (view_group_name_get(edj->vd) == group)
          return edj->vd;
     }
   return NULL;
}

void
edj_mgr_view_del(edj_mgr *em, view_data *vd)
{
   edj_data *edj = view_data_get(vd);
   em->edjs = eina_list_remove(em->edjs, edj);
   if (edj->timer) ecore_timer_del(edj->timer);
   view_term(vd);
   free(edj);
}

static void
view_del_cb(void *data)
{
   edj_mgr *em = g_em;
   edj_data *edj = data;
   em->edjs = eina_list_remove(em->edjs, edj);
   if (edj->timer) ecore_timer_del(edj->timer);
   if (em->edj == edj) em->edj = NULL;
   free(edj);
}

view_data *
edj_mgr_view_new(edj_mgr *em, const char *group, stats_data *sd,
                 config_data *cd)
{
   edj_data *edj = calloc(1, sizeof(edj_data));
   if (!edj) return NULL;

   view_data *vd = view_init(em->layout, group, sd, cd, view_del_cb, edj);
   if (!vd)
     {
        free(edj);
        return NULL;
     }

   edj->vd = vd;
   edj_mgr_view_switch_to(em, vd);

   em->edjs = eina_list_append(em->edjs, edj);

   return vd;
}

static Eina_Bool
view_del_timer_cb(void *data)
{
   view_data *vd = data;
   edj_data *edj = view_data_get(vd);
   edj->timer = NULL;
   edj_mgr_view_del(g_em, vd);
   return ECORE_CALLBACK_CANCEL;
}

view_data *
edj_mgr_view_switch_to(edj_mgr *em, view_data *vd)
{
   if (em->edj && (em->edj->vd == vd)) return vd;

   //Switch views
   Evas_Object *prev =
      elm_object_part_content_unset(em->layout, "elm.swallow.content");
   elm_object_part_content_set(em->layout, "elm.swallow.content",
                               view_obj_get(vd));

   //Switching effect
   if (prev && (prev != view_obj_get(vd)))
     {
        Evas_Object *tmp =
           elm_object_part_content_unset(em->layout, "elm.swallow.prev");
        if (tmp) evas_object_hide(tmp);
        elm_object_part_content_set(em->layout, "elm.swallow.prev", prev);
        elm_object_signal_emit(em->layout, "elm,view,switch", "");
     }
   else
     {
        elm_object_signal_emit(em->layout, "elm,view,switch,instant", "");
     }

   //Reset caching timers
   edj_data *cur_edj = view_data_get(vd);
   if (cur_edj->timer)
     {
        ecore_timer_del(cur_edj->timer);
        cur_edj->timer = NULL;
     }

   edj_data *prev_edj = em->edj;
   if (prev_edj)
     {
        if (prev_edj->timer) ecore_timer_del(prev_edj->timer);
        prev_edj->timer = ecore_timer_add(VIEW_CACHING_TIME, view_del_timer_cb,
                                          prev_edj->vd);
     }
   em->edj = view_data_get(vd);

   return vd;
}

Evas_Object *
edj_mgr_obj_get(edj_mgr *em)
{
   return em->layout;
}

void
edj_mgr_reload_need_set(edj_mgr *em, Eina_Bool reload)
{
   em->reload_need = reload;
}

Eina_Bool
edj_mgr_reload_need_get(edj_mgr *em)
{
   return em->reload_need;
}
