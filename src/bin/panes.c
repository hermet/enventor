#include <Elementary.h>
#include "common.h"

typedef enum
{
   PANES_FULL_VIEW_LEFT,
   PANES_FULL_VIEW_RIGHT,
   PANES_SPLIT_VIEW
} Panes_State;

const char *PANES_DATA = "_panes_data";

typedef struct _panes_data
{
   Evas_Object *panes;
   Panes_State state;

   double origin;
   double delta;
   double last_right_size1;  //when down the panes bar
   double last_right_size2;  //when up the panes bar
} panes_data;

static panes_data *g_pd = NULL;

static void
transit_op(void *data, Elm_Transit *transit EINA_UNUSED, double progress)
{
   panes_data *pd = data;
   elm_panes_content_right_size_set(pd->panes,
                                    pd->origin + (pd->delta * progress));
}

static void
press_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
    panes_data *pd = data;
    pd->last_right_size1 = elm_panes_content_right_size_get(obj);
}

static void
unpress_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
    panes_data *pd = data;
    double right_size = elm_panes_content_right_size_get(obj);
    if (pd->last_right_size1 != right_size)
      pd->last_right_size2 = right_size;
}

static void
panes_full_view_cancel(panes_data *pd)
{
   const double TRANSIT_TIME = 0.25;

   pd->origin = elm_panes_content_right_size_get(pd->panes);
   pd->delta = pd->last_right_size2 - pd->origin;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);

   pd->state = PANES_SPLIT_VIEW;
}

void
panes_full_view_right(void)
{
   const double TRANSIT_TIME = 0.25;

   panes_data *pd = g_pd;

   //Revert state if the current state is full view left already.
   if (pd->state == PANES_FULL_VIEW_RIGHT)
     {
        panes_full_view_cancel(pd);
        return;
     }

   double origin = elm_panes_content_right_size_get(pd->panes);
   if (origin == 0.0) return;

   pd->origin = origin;
   pd->delta = 0.0 - pd->origin;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);

   pd->state = PANES_FULL_VIEW_RIGHT;
}

void
panes_full_view_left(void)
{
   const double TRANSIT_TIME = 0.25;

   panes_data *pd = g_pd;

   //Revert state if the current state is full view left already.
   if (pd->state == PANES_FULL_VIEW_LEFT)
     {
        panes_full_view_cancel(pd);
        return;
     }

   double origin = elm_panes_content_right_size_get(pd->panes);
   if (origin == 1.0) return;

   pd->origin = origin;
   pd->delta = 1.0 - pd->origin;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);

   pd->state = PANES_FULL_VIEW_LEFT;

}

void
panes_content_set(const char *part, Evas_Object *content)
{
   panes_data *pd = g_pd;
   elm_object_part_content_set(pd->panes, part, content);
}

void
panes_term(void)
{
   panes_data *pd = g_pd;
   evas_object_del(pd->panes);
   free(pd);
}

Evas_Object *
panes_init(Evas_Object *parent)
{
   panes_data *pd = malloc(sizeof(panes_data));
   if (!pd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }
   g_pd = pd;

   //Panes
   Evas_Object *panes = elm_panes_add(parent);
   elm_object_style_set(panes, elm_app_name_get());
   evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(panes, "press", press_cb, pd);
   evas_object_smart_callback_add(panes, "unpress", unpress_cb, pd);
   pd->panes = panes;
   pd->state = PANES_SPLIT_VIEW;
   pd->last_right_size1 = 0.5;
   pd->last_right_size2 = 0.5;

   evas_object_data_set(panes, PANES_DATA, pd);

   return panes;
}
