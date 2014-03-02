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
static panes_data *g_pd = NULL;

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
hotkeys_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj,
                   void *event_info EINA_UNUSED)
{
   base_hotkey_toggle();

   Evas_Object *img = elm_object_content_get(obj);

   if (config_hotkeys_get())
     elm_image_file_set(img, EDJE_PATH, "hotkeys_close");
   else
     elm_image_file_set(img, EDJE_PATH, "hotkeys_open");
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
        Evas_Object *left_arrow_img = elm_object_content_get(obj);
        elm_image_file_set(left_arrow_img, EDJE_PATH, "panes_left_arrow");
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
   Evas_Object *right_arrow_img = elm_object_content_get(pd->right_arrow);
   elm_image_file_set(right_arrow_img, EDJE_PATH, "panes_right_arrow");

   Evas_Object *left_arrow_img = elm_object_content_get(obj);
   elm_image_file_set(left_arrow_img, EDJE_PATH, "panes_recover_arrow");
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
        Evas_Object *right_arrow_img = elm_object_content_get(obj);
        elm_image_file_set(right_arrow_img, EDJE_PATH, "panes_right_arrow");
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
   Evas_Object *left_arrow_img = elm_object_content_get(pd->left_arrow);
   elm_image_file_set(left_arrow_img, EDJE_PATH, "panes_left_arrow");

   Evas_Object *right_arrow_img = elm_object_content_get(obj);
   elm_image_file_set(right_arrow_img, EDJE_PATH, "panes_recover_arrow");
}

void
panes_full_view_right()
{
   panes_data *pd = g_pd;
   right_clicked_cb(pd, pd->right_arrow, NULL);
}

void
panes_full_view_left()
{
   panes_data *pd = g_pd;
   left_clicked_cb(pd, pd->left_arrow, NULL);
}

void
panes_content_set(const char *part, Evas_Object *content)
{
   panes_data *pd = g_pd;
   elm_object_part_content_set(pd->panes, part, content);
}

void
panes_term()
{
   panes_data *pd = g_pd;
   evas_object_del(pd->panes);
   free(pd);
}

Evas_Object *
panes_init(Evas_Object *parent)
{
   Evas_Object *img;

   panes_data *pd = malloc(sizeof(panes_data));
   g_pd = pd;

   //Panes
   Evas_Object *panes = elm_panes_add(parent);
   elm_object_style_set(panes, elm_app_name_get());
   evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(panes, "press",
                                  press_cb, NULL);
   evas_object_smart_callback_add(panes, "unpress",
                                  unpress_cb, NULL);
   //Hotkey Button
   Evas_Object *hotkeys_btn = elm_button_add(panes);
   elm_object_focus_allow_set(hotkeys_btn, EINA_FALSE);
   evas_object_smart_callback_add(hotkeys_btn, "clicked", hotkeys_clicked_cb,
                                  NULL);
   elm_object_part_content_set(panes, "elm.swallow.hotkeys", hotkeys_btn);

   //Hotkey Image
   img = elm_image_add(hotkeys_btn);
   if (config_hotkeys_get())
     elm_image_file_set(img, EDJE_PATH, "hotkeys_close");
   else
     elm_image_file_set(img, EDJE_PATH, "hotkeys_open");
   elm_object_content_set(hotkeys_btn, img);

   //Left Button
   Evas_Object *left_arrow = elm_button_add(panes);
   elm_object_focus_allow_set(left_arrow, EINA_FALSE);
   evas_object_smart_callback_add(left_arrow, "clicked", left_clicked_cb, pd);
   elm_object_part_content_set(panes, "elm.swallow.left_arrow", left_arrow);

   //Left Arrow Image
   img = elm_image_add(left_arrow);
   elm_image_file_set(img, EDJE_PATH, "panes_left_arrow");
   elm_object_content_set(left_arrow, img);

   //Right Button
   Evas_Object *right_arrow = elm_button_add(panes);
   elm_object_focus_allow_set(right_arrow, EINA_FALSE);
   evas_object_smart_callback_add(right_arrow, "clicked", right_clicked_cb, pd);
   elm_object_part_content_set(panes, "elm.swallow.right_arrow", right_arrow);

   //Right Arrow Image
   img = elm_image_add(right_arrow);
   elm_image_file_set(img, EDJE_PATH, "panes_right_arrow");
   elm_object_content_set(right_arrow, img);

   pd->panes = panes;
   pd->left_arrow = left_arrow;
   pd->right_arrow = right_arrow;
   pd->state = PANES_SPLIT_VIEW;

   evas_object_data_set(panes, PANES_DATA, pd);

   return panes;
}
