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
   Evas_Object *left_arrow;
   Evas_Object *right_arrow;
   Panes_State state;

   double origin;
   double delta;
} panes_data;

static double panes_last_right_size1 = 0.5;  //when down the panes bar
static double panes_last_right_size2 = 0.5;  //when up the panes bar

static void
transit_op(void *data, Elm_Transit *transit EINA_UNUSED, double progress)
{
   panes_data *pd = data;
   elm_panes_content_right_size_set(pd->panes,
                                    pd->origin + (pd->delta * progress));
}

static void
press_cb(void *data EINA_UNUSED, Evas_Object *obj,
             void *event_info EINA_UNUSED)
{
    panes_last_right_size1 = elm_panes_content_right_size_get(obj);
}

static void
unpress_cb(void *data EINA_UNUSED, Evas_Object *obj,
             void *event_info EINA_UNUSED)
{
    double right_size = elm_panes_content_right_size_get(obj);
    if (panes_last_right_size1 != right_size)
      panes_last_right_size2 = right_size;
}

static void
panes_full_view_cancel(panes_data *pd)
{
   const double TRANSIT_TIME = 0.25;

   pd->origin = elm_panes_content_right_size_get(pd->panes);
   pd->delta = panes_last_right_size2 - pd->origin;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);

   pd->state = PANES_SPLIT_VIEW;
}

static void
left_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   const double TRANSIT_TIME = 0.25;

   panes_data *pd = data;

   //Revert state if the current state is full view left already.
   if (pd->state == PANES_FULL_VIEW_LEFT)
     {
        panes_full_view_cancel(pd);
        elm_object_text_set(obj, "<");
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
   elm_object_text_set(pd->right_arrow, ">");
   elm_object_text_set(obj, "|");
}

static void
right_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   const double TRANSIT_TIME = 0.25;

   panes_data *pd = data;

   //Revert state if the current state is full view left already.
   if (pd->state == PANES_FULL_VIEW_RIGHT)
     {
        panes_full_view_cancel(pd);
        elm_object_text_set(obj, ">");
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
   elm_object_text_set(pd->left_arrow, "<");
   elm_object_text_set(obj, "|");
}

void
panes_full_view_right(Evas_Object *panes)
{
   panes_data *pd = evas_object_data_get(panes, PANES_DATA);
   right_clicked_cb(pd, pd->right_arrow, NULL);
}

void
panes_full_view_left(Evas_Object *panes)
{
   panes_data *pd = evas_object_data_get(panes, PANES_DATA);
   left_clicked_cb(pd, pd->left_arrow, NULL);
}

static void
panes_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   panes_data *pd = data;
   free(pd);
}

Evas_Object *
panes_create(Evas_Object *parent)
{
   panes_data *pd = malloc(sizeof(panes_data));

   //Panes
   Evas_Object *panes = elm_panes_add(parent);
   elm_object_style_set(panes, elm_app_name_get());
   evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(panes, "press",
                                  press_cb, NULL);
   evas_object_smart_callback_add(panes, "unpress",
                                  unpress_cb, NULL);
   evas_object_event_callback_add(panes, EVAS_CALLBACK_DEL, panes_del_cb, pd);

   evas_object_show(panes);

   //Left Button
   Evas_Object *left_arrow = elm_button_add(panes);
   elm_object_text_set(left_arrow, "<");
   elm_object_focus_allow_set(left_arrow, EINA_FALSE);
   evas_object_smart_callback_add(left_arrow, "clicked", left_clicked_cb, pd);
   evas_object_show(left_arrow);

   elm_object_part_content_set(panes, "elm.swallow.left_arrow", left_arrow);

   //Right Button
   Evas_Object *right_arrow = elm_button_add(panes);
   elm_object_text_set(right_arrow, ">");
   elm_object_focus_allow_set(right_arrow, EINA_FALSE);
   evas_object_smart_callback_add(right_arrow, "clicked", right_clicked_cb, pd);
   evas_object_show(right_arrow);

   elm_object_part_content_set(panes, "elm.swallow.right_arrow", right_arrow);

   pd->panes = panes;
   pd->left_arrow = left_arrow;
   pd->right_arrow = right_arrow;
   pd->state = PANES_SPLIT_VIEW;

   evas_object_data_set(panes, PANES_DATA, pd);

   return panes;
}

