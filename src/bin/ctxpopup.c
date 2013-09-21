#include <Elementary.h>
#include "common.h"

struct ctxpopup_data_s {
   Evas_Smart_Cb selected_cb;
   Evas_Smart_Cb relay_cb;
   Evas_Object *ctxpopup;
   void *data;
};

typedef struct ctxpopup_data_s ctxpopup_data;

static void
btn_plus_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   Evas_Object *slider = data;
   attr_value *attr = evas_object_data_get(slider, "attr");
   double value = elm_slider_value_get(slider);

   if (attr->integer) elm_slider_value_set(slider, value + 1);
   else elm_slider_value_set(slider, value + 0.01);
}

static void
btn_minus_cb(void *data, Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   Evas_Object *slider = data;
   attr_value *attr = evas_object_data_get(slider, "attr");
   double value = elm_slider_value_get(slider);

   if (attr->integer) elm_slider_value_set(slider, value - 1);
   else elm_slider_value_set(slider, value - 0.01);
}

static void
ctxpopup_it_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Elm_Object_Item *it = event_info;
   ctxpopup_data *ctxdata = evas_object_data_get(obj, "ctxpopup_data");
   ctxdata->selected_cb(ctxdata->data, obj,
                        (void *) elm_object_item_text_get(it));
}

static void
slider_dismiss_cb(void *data EINA_UNUSED, Evas_Object *obj,
                  void *event_info EINA_UNUSED)
{
   Evas_Object *layout = elm_object_content_get(obj);
   Evas_Object *slider = elm_object_part_content_get(layout,
                                                     "elm.swallow.slider");
   char buf[128];
   attr_value *attr = evas_object_data_get(slider, "attr");
   if (attr->integer)
     {
        snprintf(buf, sizeof(buf), "%d",
                 (int) roundf(elm_slider_value_get(slider)));
     }
   else
     {
        //if the last digit number is 0 then round up.
        double val = elm_slider_value_get(slider);
        snprintf(buf, sizeof(buf), "%0.2f", val);
        double round_down = atof(buf);
        snprintf(buf, sizeof(buf), "%0.1f", val);
        double round_down2 = atof(buf);
        if (fabs(round_down - round_down2) < 0.0005)
          snprintf(buf, sizeof(buf), "%0.1f", val);
        else
          snprintf(buf, sizeof(buf), "%0.2f", val);
     }

   ctxpopup_data *ctxdata = evas_object_data_get(obj, "ctxpopup_data");
   ctxdata->selected_cb(ctxdata->data, obj, buf);
}

static void
ctxpopup_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   free(data);
}

Evas_Object *
ctxpopup_candidate_list_create(Evas_Object *parent, attr_value *attr,
                               double slider_val,
                               Evas_Smart_Cb ctxpopup_dismiss_cb,
                               Evas_Smart_Cb ctxpopup_selected_cb, void *data)
{
   //create ctxpopup
   Evas_Object *ctxpopup = elm_ctxpopup_add(parent);
   if (!ctxpopup) return NULL;

   elm_object_style_set(ctxpopup, elm_app_name_get());

   //ctxpopup data
   ctxpopup_data *ctxdata = malloc(sizeof(ctxpopup_data));
   if (!ctxdata) return NULL;
   ctxdata->selected_cb = ctxpopup_selected_cb;
   ctxdata->data = data;
   evas_object_data_set(ctxpopup, "ctxpopup_data", ctxdata);

   //case of strings
   if (attr->strs)
     {
        Eina_List *l;
        Eina_Stringshare *candidate;
        EINA_LIST_FOREACH(attr->strs, l, candidate)
          elm_ctxpopup_item_append(ctxpopup, candidate, NULL, ctxpopup_it_cb,
                                   data);
     }
   //case of numbers
   else
     {
        //Layout
        Evas_Object *layout = elm_layout_add(ctxpopup);
        elm_layout_file_set(layout, EDJE_PATH, "slider_layout");
        evas_object_show(layout);

        elm_object_content_set(ctxpopup, layout);

        //Slider
        Evas_Object *slider = elm_slider_add(layout);
        if (attr->integer) elm_slider_unit_format_set(slider, "%1.0f");
        else elm_slider_unit_format_set(slider, "%1.2f");
        elm_slider_span_size_set(slider, 120);
        elm_slider_indicator_show_set(slider, EINA_FALSE);
        elm_slider_min_max_set(slider, attr->min, attr->max);
        elm_slider_value_set(slider, slider_val);
        evas_object_data_set(slider, "attr", attr);
        evas_object_show(slider);

        elm_object_part_content_set(layout, "elm.swallow.slider", slider);

        Evas_Object *btn;
        Evas_Object *img;

        //Minus Button
        btn = elm_button_add(layout);
        evas_object_show(btn);
        evas_object_smart_callback_add(btn, "clicked", btn_minus_cb, slider);

        elm_object_part_content_set(layout, "elm.swallow.minus", btn);

        //Minus Image
        img = elm_image_add(btn);
        elm_image_file_set(img, EDJE_PATH, "minus_img");
        evas_object_show(img);

        elm_object_content_set(btn, img);

        //Plus Button
        btn = elm_button_add(layout);
        evas_object_show(btn);
        evas_object_smart_callback_add(btn, "clicked", btn_plus_cb, slider);

        elm_object_part_content_set(layout, "elm.swallow.plus", btn);

        //Plus Image
        img = elm_image_add(btn);
        elm_image_file_set(img, EDJE_PATH, "plus_img");
        evas_object_show(img);

        elm_object_content_set(btn, img);

        evas_object_smart_callback_add(ctxpopup, "dismissed",
                                       slider_dismiss_cb, data);
     }

   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL, ctxpopup_del_cb,
                                  ctxdata);
   evas_object_smart_callback_add(ctxpopup, "dismissed", ctxpopup_dismiss_cb,
                                  data);
   return ctxpopup;
}

static void
ctxpopup_key_down_cb(void *data, Evas *e EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   ctxpopup_data *ctxdata = data;

   if (!strcmp(ev->keyname, "Down"))
     ctxdata->relay_cb(ctxdata->data, ctxdata->ctxpopup, (void *) 1);
   else if (!strcmp(ev->keyname, "Up"))
     ctxdata->relay_cb(ctxdata->data, ctxdata->ctxpopup, (void *) 0);
}

Evas_Object *
ctxpopup_img_preview_create(Evas_Object *parent, const char *imgpath,
                            Evas_Smart_Cb ctxpopup_dismiss_cb,
                            Evas_Smart_Cb ctxpopup_relay_cb, void *data)
{
   //create ctxpopup
   Evas_Object *ctxpopup = elm_ctxpopup_add(parent);
   if (!ctxpopup) return NULL;

   elm_object_style_set(ctxpopup, elm_app_name_get());
   elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_LEFT,
                                       ELM_CTXPOPUP_DIRECTION_UP,
                                       ELM_CTXPOPUP_DIRECTION_DOWN,
                                       ELM_CTXPOPUP_DIRECTION_RIGHT);
   //ctxpopup data
   ctxpopup_data *ctxdata = malloc(sizeof(ctxpopup_data));
   if (!ctxdata) return NULL;
   ctxdata->relay_cb = ctxpopup_relay_cb;
   ctxdata->data = data;
   ctxdata->ctxpopup = ctxpopup;
   evas_object_data_set(ctxpopup, "ctxpopup_data", ctxdata);

   //Layout
   Evas_Object *layout = elm_layout_add(ctxpopup);
   elm_layout_file_set(layout, EDJE_PATH, "preview_layout");
   evas_object_show(layout);

   elm_object_content_set(ctxpopup, layout);

   Evas *e = evas_object_evas_get(ctxpopup);
   Evas_Object *img = evas_object_image_filled_add(e);
   evas_object_image_file_set(img, imgpath, NULL);
   Evas_Coord w, h;
   evas_object_image_size_get(img, &w, &h);
   evas_object_size_hint_aspect_set(img, EVAS_ASPECT_CONTROL_BOTH, w, h);
   evas_object_show(img);
   elm_object_part_content_set(layout, "elm.swallow.img", img);

   evas_object_smart_callback_add(ctxpopup, "dismissed", ctxpopup_dismiss_cb,
                                  data);
   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL, ctxpopup_del_cb,
                                  ctxdata);
   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_KEY_DOWN,
                                  ctxpopup_key_down_cb, ctxdata);
   evas_object_focus_set(ctxpopup, EINA_TRUE);
   return ctxpopup;
}
