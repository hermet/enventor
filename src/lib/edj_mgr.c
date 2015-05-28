#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include "enventor_private.h"

const double VIEW_CACHING_TIME = 60 * 30;

typedef struct edj_data_s
{
   view_data *vd;
   Ecore_Timer *timer;
} edj_data;

typedef struct edj_mgr_s
{
   Eina_List *edjs;
   edj_data *edj;
   Evas_Object *enventor;
   Evas_Object *layout;
   double view_scale;

   Eina_Bool reload_need : 1;
} edj_mgr;

static edj_mgr *g_em = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
view_del_cb(void *data)
{
   edj_mgr *em = g_em;
   edj_data *edj = data;
   em->edjs = eina_list_remove(em->edjs, edj);
   ecore_timer_del(edj->timer);
   if (em->edj == edj) em->edj = NULL;
   free(edj);
}

static Eina_Bool
view_del_timer_cb(void *data)
{
   view_data *vd = data;
   edj_data *edj = view_data_get(vd);
   edj->timer = NULL;
   edj_mgr_view_del(vd);
   return ECORE_CALLBACK_CANCEL;
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
edj_mgr_clear(void)
{
   edj_data *edj;
   edj_mgr *em = g_em;

   EINA_LIST_FREE(em->edjs, edj)
     {
        ecore_timer_del(edj->timer);
        view_term(edj->vd);
        free(edj);
     }
   em->edjs = NULL;
   em->edj = NULL;
   em->reload_need = EINA_FALSE;
}

void
edj_mgr_init(Evas_Object *enventor)
{
   edj_mgr *em = calloc(1, sizeof(edj_mgr));
   if (!em)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }
   g_em = em;

   Evas_Object *layout = elm_layout_add(enventor);
   elm_layout_file_set(layout, EDJE_PATH, "viewer_layout");
   em->enventor = enventor;
   em->layout = layout;
   em->view_scale = 1;
}

void
edj_mgr_term(void)
{
   edj_mgr *em = g_em;

   edj_mgr_clear();
   evas_object_del(em->layout);
   free(em);
}

view_data *
edj_mgr_view_get(Eina_Stringshare *group)
{
   edj_mgr *em = g_em;
   if (!em) return NULL;

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
edj_mgr_view_del(view_data *vd)
{
   edj_mgr *em = g_em;
   edj_data *edj = view_data_get(vd);
   em->edjs = eina_list_remove(em->edjs, edj);
   ecore_timer_del(edj->timer);
   view_term(vd);
   free(edj);
}

view_data *
edj_mgr_view_new(const char *group)
{
   edj_mgr *em = g_em;

   edj_data *edj = calloc(1, sizeof(edj_data));
   if (!edj)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }

   view_data *vd = view_init(em->enventor, group, view_del_cb, edj);
   if (!vd)
     {
        free(edj);
        return NULL;
     }

   edj->vd = vd;
   edj_mgr_view_switch_to(vd);

   em->edjs = eina_list_append(em->edjs, edj);

   return vd;
}

view_data *
edj_mgr_view_switch_to(view_data *vd)
{
   edj_mgr *em = g_em;

   if (em->edj && (em->edj->vd == vd)) return vd;

   //Switch views
   Evas_Object *prev =
      elm_object_part_content_unset(em->layout, "elm.swallow.content");
   elm_object_part_content_set(em->layout, "elm.swallow.content",
                               view_obj_get(vd));

   view_scale_set(vd, em->view_scale);

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
   ecore_timer_del(cur_edj->timer);
   cur_edj->timer = NULL;

   edj_data *prev_edj = em->edj;
   if (prev_edj)
     {
        ecore_timer_del(prev_edj->timer);
        prev_edj->timer = ecore_timer_add(VIEW_CACHING_TIME, view_del_timer_cb,
                                          prev_edj->vd);
     }
   em->edj = view_data_get(vd);

   return vd;
}

Evas_Object *
edj_mgr_obj_get(void)
{
   edj_mgr *em = g_em;
   return em->layout;
}

void
edj_mgr_reload_need_set(Eina_Bool reload)
{
   edj_mgr *em = g_em;
   em->reload_need = reload;
}

Eina_Bool
edj_mgr_reload_need_get(void)
{
   edj_mgr *em = g_em;
   return em->reload_need;
}

void
edj_mgr_view_scale_set(double view_scale)
{
   edj_mgr *em = g_em;
   em->view_scale = view_scale;
   view_scale_set(VIEW_DATA, view_scale);
}

double
edj_mgr_view_scale_get(void)
{
   edj_mgr *em = g_em;
   return em->view_scale;
}

void
edj_mgr_all_views_reload(void)
{
   edj_mgr *em = g_em;
   if (!em) return;
   Eina_List *l = NULL;
   edj_data *edj = NULL;

   EINA_LIST_FOREACH(em->edjs, l, edj)
     view_obj_need_reload_set(edj->vd);
}

