#include <Elementary.h>
#include "common.h"

static const double TRANSIT_TIME = 0.25;

typedef enum
{
   PANES_FULL_VIEW_LEFT,
   PANES_FULL_VIEW_RIGHT,
   PANES_FULL_VIEW_TOP,
   PANES_FULL_VIEW_BOTTOM,
   PANES_SPLIT_VIEW
} Panes_State;

typedef struct _panes_data
{
   Evas_Object *panes_h;
   Evas_Object *panes_v;
   Panes_State state_v;
   Panes_State state_h;

   double origin_h;
   double origin_v;

   double delta_h;
   double delta_v;

   double last_bottom_size1;  //when down the panes bar
   double last_bottom_size2;  //when up the panes bar
   double last_right_size1;   //when down the panes bar
   double last_right_size2;   //when up the panes bar
} panes_data;

static panes_data *g_pd = NULL;

static void
transit_op_v(void *data, Elm_Transit *transit EINA_UNUSED, double progress)
{
   panes_data *pd = data;
   elm_panes_content_right_size_set(pd->panes_v,
                                    pd->origin_v + (pd->delta_v * progress));
}

static void
transit_op_h(void *data, Elm_Transit *transit EINA_UNUSED, double progress)
{
   panes_data *pd = data;
   elm_panes_content_right_size_set(pd->panes_h,
                                    pd->origin_h + (pd->delta_h * progress));
}

static void
v_press_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
    panes_data *pd = data;
    pd->last_bottom_size1 = elm_panes_content_right_size_get(obj);
}

static void
v_unpress_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
    panes_data *pd = data;
    double bottom_size = elm_panes_content_right_size_get(obj);
    if (pd->last_bottom_size1 != bottom_size)
      pd->last_right_size2 = bottom_size;
}

static void
h_press_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
    panes_data *pd = data;
    pd->last_right_size1 = elm_panes_content_right_size_get(obj);
}

static void
h_unpress_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
    panes_data *pd = data;
    double right_size = elm_panes_content_right_size_get(obj);
    if (pd->last_right_size1 != right_size)
      pd->last_right_size2 = right_size;
}

static void
panes_h_full_view_cancel(panes_data *pd)
{
   pd->origin_h = elm_panes_content_right_size_get(pd->panes_h);
   pd->delta_h = pd->last_right_size2 - pd->origin_h;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op_h, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);

   pd->state_h = PANES_SPLIT_VIEW;
}

static void
panes_v_full_view_cancel(panes_data *pd)
{
   pd->origin_v = elm_panes_content_right_size_get(pd->panes_v);
   pd->delta_v = pd->last_bottom_size2 - pd->origin_v;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op_v, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);

   pd->state_v = PANES_SPLIT_VIEW;
}

void
panes_full_view_right(void)
{
   panes_data *pd = g_pd;

   //Revert state if the current state is full view right already.
   if (pd->state_h == PANES_FULL_VIEW_RIGHT)
     {
        panes_h_full_view_cancel(pd);
        return;
     }

   double origin_h = elm_panes_content_right_size_get(pd->panes_h);
   if (origin_h == 0.0) return;

   pd->origin_h = origin_h;
   pd->delta_h = 0.0 - pd->origin_h;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op_h, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);

   pd->state_h = PANES_FULL_VIEW_RIGHT;
}

void
panes_full_view_left(void)
{
   panes_data *pd = g_pd;

   //Revert state if the current state is full view left already.
   if (pd->state_h == PANES_FULL_VIEW_LEFT)
     {
        panes_h_full_view_cancel(pd);
        return;
     }

   double origin_h = elm_panes_content_right_size_get(pd->panes_h);
   if (origin_h == 1.0) return;

   pd->origin_h = origin_h;
   pd->delta_h = 1.0 - pd->origin_h;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op_h, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);

   pd->state_h = PANES_FULL_VIEW_LEFT;

}

void
panes_full_view_bottom(void)
{
   panes_data *pd = g_pd;

   //Revert state if the current state is full view bottom already.
   if (pd->state_v == PANES_FULL_VIEW_BOTTOM)
     {
        panes_v_full_view_cancel(pd);
        return;
     }
   double origin_v = elm_panes_content_right_size_get(pd->panes_v);
   if (origin_v == 0.0) return;

   pd->origin_v = origin_v;
   pd->delta_v = 0.0 - pd->origin_v;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op_v, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);

   pd->state_v = PANES_FULL_VIEW_BOTTOM;
}

void
panes_full_view_top(void)
{
   panes_data *pd = g_pd;

   //Revert state if the current state is full view top already.
   if (pd->state_v == PANES_FULL_VIEW_TOP)
     {
        panes_v_full_view_cancel(pd);
        return;
     }

   double origin_v = elm_panes_content_right_size_get(pd->panes_v);
   if (origin_v == 1.0) return;

   pd->origin_v = origin_v;
   pd->delta_v = 1.0 - pd->origin_v;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op_v, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);

   pd->state_v = PANES_FULL_VIEW_TOP;
}


void
panes_content_set(const char *part, Evas_Object *content)
{
   panes_data *pd = g_pd;
   elm_object_part_content_set(pd->panes_h, part, content);
}

void
panes_term(void)
{
   panes_data *pd = g_pd;
   evas_object_del(pd->panes_v);
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

   //Panes Vertical
   Evas_Object *panes_v = elm_panes_add(parent);
   elm_object_style_set(panes_v, "flush");
   elm_panes_horizontal_set(panes_v, EINA_FALSE);
   evas_object_size_hint_weight_set(panes_v, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(panes_v, "press", v_press_cb, pd);
   evas_object_smart_callback_add(panes_v, "unpress", v_unpress_cb, pd);

   pd->panes_v = panes_v;
   pd->state_v = PANES_SPLIT_VIEW;
   pd->last_bottom_size1 = 0.5;
   pd->last_bottom_size2 = 0.5;

   //Panes Horizontal
   Evas_Object *panes_h = elm_panes_add(parent);
   elm_object_style_set(panes_h, elm_app_name_get());
   elm_panes_horizontal_set(panes_v, EINA_TRUE);
   evas_object_size_hint_weight_set(panes_h, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(panes_h, "press", h_press_cb, pd);
   evas_object_smart_callback_add(panes_h, "unpress", h_unpress_cb, pd);

   elm_object_part_content_set(panes_v, "top", panes_h);

   pd->panes_h = panes_h;
   pd->state_h = PANES_SPLIT_VIEW;
   pd->last_right_size1 = 0.5;
   pd->last_right_size2 = 0.5;

   return panes_v;
}
