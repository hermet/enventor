#include <Elementary.h>
#include "common.h"

typedef struct _transit_data
{
   double origin;
   double delta;
   Evas_Object *panes;
} transit_data;

static double panes_last_right_size1 = 0.5;  //when down the panes bar
static double panes_last_right_size2 = 0.5;  //when up the panes bar

static void
transit_op(void *data, Elm_Transit *transit EINA_UNUSED, double progress)
{
   transit_data *td = data;
   elm_panes_content_right_size_set(td->panes,
                                    td->origin + (td->delta * progress));
}

static void
transit_free(void *data, Elm_Transit *transit EINA_UNUSED)
{
   transit_data *td = data;
   free(td);
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
double_click_cb(void *data EINA_UNUSED, Evas_Object *obj,
                void *event_info EINA_UNUSED)
{
   const double TRANSIT_TIME = 0.25;

   transit_data *td = malloc(sizeof(transit_data));
   if (!td) return;

   td->origin = elm_panes_content_right_size_get(obj);
   td->delta = panes_last_right_size2 - td->origin;
   td->panes = obj;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op, td, transit_free);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);
}

static void
left_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   const double TRANSIT_TIME = 0.25;

   Evas_Object *panes = data;
   double origin = elm_panes_content_right_size_get(panes);
   if (origin == 1.0) return;

   transit_data *td = malloc(sizeof(transit_data));
   if (!td) return;

   td->origin = origin;
   td->delta = 1.0 - td->origin;
   td->panes = panes;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op, td, transit_free);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);
}

static void
right_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   const double TRANSIT_TIME = 0.25;

   Evas_Object *panes = data;
   double origin = elm_panes_content_right_size_get(panes);
   if (origin == 0.0) return;

   transit_data *td = malloc(sizeof(transit_data));
   if (!td) return;

   td->origin = origin;
   td->delta = 0.0 - td->origin;
   td->panes = panes;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op, td, transit_free);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);
}

void
panes_full_view_right(Evas_Object *panes)
{
   right_clicked_cb(panes, NULL, NULL);
}

void
panes_full_view_left(Evas_Object *panes)
{
   left_clicked_cb(panes, NULL, NULL);
}

void
panes_full_view_cancel(Evas_Object *panes)
{
   double_click_cb(NULL, panes, NULL);
}

Evas_Object *
panes_create(Evas_Object *parent)
{
   //Panes
   Evas_Object *panes = elm_panes_add(parent);
   elm_object_style_set(panes, elm_app_name_get());
   evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(panes, "clicked,double",
                                  double_click_cb, NULL);
   evas_object_smart_callback_add(panes, "press",
                                  press_cb, NULL);
   evas_object_smart_callback_add(panes, "unpress",
                                  unpress_cb, NULL);
   evas_object_show(panes);

   //Left Button
   Evas_Object *left_arrow = elm_button_add(panes);
   elm_object_text_set(left_arrow, "<");
   elm_object_focus_allow_set(left_arrow, EINA_FALSE);
   evas_object_smart_callback_add(left_arrow, "clicked", left_clicked_cb,
                                  panes);
   evas_object_show(left_arrow);

   elm_object_part_content_set(panes, "elm.swallow.left_arrow", left_arrow);

   //Right Button
   Evas_Object *right_arrow = elm_button_add(panes);
   elm_object_text_set(right_arrow, ">");
   elm_object_focus_allow_set(right_arrow, EINA_FALSE);
   evas_object_smart_callback_add(right_arrow, "clicked", right_clicked_cb,
                                  panes);
   evas_object_show(right_arrow);

   elm_object_part_content_set(panes, "elm.swallow.right_arrow", right_arrow);

   return panes;
}

