#include <Elementary.h>
#include "common.h"

typedef struct ctxpopup_data_s {
   Evas_Smart_Cb selected_cb;
   Evas_Smart_Cb relay_cb;
   Evas_Object *ctxpopup;
   void *data;
} ctxpopup_data;

static void
btn_plus_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   Evas_Object *slider = data;
   attr_value *attr = evas_object_data_get(slider, "attr");
   double value = elm_slider_value_get(slider);

   if (attr->type & ATTR_VALUE_INTEGER) elm_slider_value_set(slider, value + 1);
   else elm_slider_value_set(slider, value + 0.01);
}

static void
btn_minus_cb(void *data, Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   Evas_Object *slider = data;
   attr_value *attr = evas_object_data_get(slider, "attr");
   double value = elm_slider_value_get(slider);

   if (attr->type & ATTR_VALUE_INTEGER) elm_slider_value_set(slider, value - 1);
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
   if (attr->type & ATTR_VALUE_INTEGER)
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

static void
slider_layout_set(Evas_Object *ctxpopup, attr_value *attr, double slider_val,
                  void *data, Eina_Bool integer)
{
   //Layout
   Evas_Object *layout = elm_layout_add(ctxpopup);
   elm_layout_file_set(layout, EDJE_PATH, "slider_layout");
   elm_object_content_set(ctxpopup, layout);

   //Slider
   Evas_Object *slider = elm_slider_add(layout);
   if (integer) elm_slider_unit_format_set(slider, "%1.0f");
   else elm_slider_unit_format_set(slider, "%1.2f");
   elm_slider_span_size_set(slider, 120);
   elm_slider_indicator_show_set(slider, EINA_FALSE);
   elm_slider_min_max_set(slider, attr->min, attr->max);
   elm_slider_value_set(slider, slider_val);
   evas_object_data_set(slider, "attr", attr);
   elm_object_part_content_set(layout, "elm.swallow.slider", slider);

   Evas_Object *btn;
   Evas_Object *img;

   //Minus Button
   btn = elm_button_add(layout);
   evas_object_smart_callback_add(btn, "clicked", btn_minus_cb, slider);
   elm_object_part_content_set(layout, "elm.swallow.minus", btn);

   //Minus Image
   img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, "minus");
   elm_object_content_set(btn, img);

   //Plus Button
   btn = elm_button_add(layout);
   evas_object_smart_callback_add(btn, "clicked", btn_plus_cb, slider);
   elm_object_part_content_set(layout, "elm.swallow.plus", btn);

   //Plus Image
   img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, "plus");
   elm_object_content_set(btn, img);

   evas_object_smart_callback_add(ctxpopup, "dismissed", slider_dismiss_cb,
                                  data);
}

static void
constant_candidate_set(Evas_Object *ctxpopup, attr_value *attr, void *data)
{
   Eina_Stringshare *candidate;
   Eina_Array_Iterator itr;
   unsigned int i;

   EINA_ARRAY_ITER_NEXT(attr->strs, i, candidate, itr)
     elm_ctxpopup_item_append(ctxpopup, candidate, NULL, ctxpopup_it_cb, data);
}

static Eina_Bool
part_candidate_set(Evas_Object *ctxpopup, void *data)
{
   view_data *vd = edj_mgr_view_get(NULL);
   if (!vd) return EINA_FALSE;
   Eina_List *parts = view_parts_list_get(vd);
   Eina_List *l;
   char *part;
   EINA_LIST_FOREACH(parts, l, part)
     elm_ctxpopup_item_append(ctxpopup, part, NULL, ctxpopup_it_cb, data);
   view_string_list_free(parts);
   return EINA_TRUE;
}

static Eina_Bool
image_candidate_set(Evas_Object *ctxpopup, void *data)
{
   view_data *vd = edj_mgr_view_get(NULL);
   if (!vd) return EINA_FALSE;
   Eina_List *parts = view_images_list_get(vd);
   Eina_List *l;
   char *part;
   EINA_LIST_FOREACH(parts, l, part)
     elm_ctxpopup_item_append(ctxpopup, part, NULL, ctxpopup_it_cb, data);
   view_string_list_free(parts);
   return EINA_TRUE;
}

static Eina_Bool
program_candidate_set(Evas_Object *ctxpopup, void *data)
{
   view_data *vd = edj_mgr_view_get(NULL);
   if (!vd) return EINA_FALSE;
   Eina_List *parts = view_programs_list_get(vd);
   Eina_List *l;
   char *part;
   EINA_LIST_FOREACH(parts, l, part)
     elm_ctxpopup_item_append(ctxpopup, part, NULL, ctxpopup_it_cb, data);
   view_string_list_free(parts);
   return EINA_TRUE;
}

static Eina_Bool
state_candidate_set(Evas_Object *ctxpopup, void *data, Eina_Bool prog)
{
   view_data *vd = edj_mgr_view_get(NULL);
   if (!vd) return EINA_FALSE;

   edit_data *ed = data;
   Eina_Stringshare *program = NULL;
   Eina_List *targets = NULL;
   Eina_Stringshare *target = NULL;
   Eina_Bool ret = EINA_FALSE;

   //Trace the part name from the program.
   if (prog)
     {
        program = edit_cur_prog_name_get(ed);
        if (!program) return EINA_FALSE;
        targets = view_program_targets_get(vd, program);
        target = eina_list_data_get(targets);
        if (!target) goto end;
     }
   //Trace the part name from the part.
   else
     {
        target = edit_cur_part_name_get(ed);
        if (!target) goto end;
     }

   Eina_List *states = view_part_states_list_get(vd, target);

   /* Since the states have the name + float values, it needs to filterout the
      values. */
   Eina_List *converted = parser_states_filtered_name_get(states);

   Eina_List *l;
   char *state;
   EINA_LIST_FOREACH(converted, l, state)
     {
        elm_ctxpopup_item_append(ctxpopup, state, NULL, ctxpopup_it_cb, data);
        free(state);
     }
   view_string_list_free(states);
   eina_list_free(converted);
   ret = EINA_TRUE;
end:
   eina_stringshare_del(program);
   view_string_list_free(targets);
   return ret;
}

Evas_Object *
ctxpopup_candidate_list_create(edit_data *ed, attr_value *attr,
                               double slider_val,
                               Evas_Smart_Cb ctxpopup_dismiss_cb,
                               Evas_Smart_Cb ctxpopup_selected_cb)
{
   //create ctxpopup
   Evas_Object *ctxpopup = elm_ctxpopup_add(base_layout_get());
   if (!ctxpopup) return NULL;

   elm_object_style_set(ctxpopup, elm_app_name_get());

   //ctxpopup data
   ctxpopup_data *ctxdata = malloc(sizeof(ctxpopup_data));
   if (!ctxdata)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        goto err;
     }
   ctxdata->selected_cb = ctxpopup_selected_cb;
   ctxdata->data = ed;
   evas_object_data_set(ctxpopup, "ctxpopup_data", ctxdata);

   switch (attr->type)
     {
        case ATTR_VALUE_INTEGER:
          {
             slider_layout_set(ctxpopup, attr, slider_val, ed, EINA_TRUE);
             break;
          }
        case ATTR_VALUE_FLOAT:
          {
             slider_layout_set(ctxpopup, attr, slider_val, ed, EINA_FALSE);
             break;
          }
        case ATTR_VALUE_CONSTANT:
          {
             constant_candidate_set(ctxpopup, attr, ed);
             break;
          }
        case ATTR_VALUE_PART:
          {
             if (!part_candidate_set(ctxpopup, ed)) goto err;
             break;
          }
        case ATTR_VALUE_STATE:
          {
             if (!state_candidate_set(ctxpopup, ed, attr->program)) goto err;
             break;
          }
        case ATTR_VALUE_IMAGE:
          {
             if (!image_candidate_set(ctxpopup, ed)) goto err;
             break;
          }
        case ATTR_VALUE_PROGRAM:
          {
             if (!program_candidate_set(ctxpopup, ed)) goto err;
             break;
          }
   }
   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL, ctxpopup_del_cb,
                                  ctxdata);
   evas_object_smart_callback_add(ctxpopup, "dismissed", ctxpopup_dismiss_cb,
                                  ed);
   return ctxpopup;

err:
   free(ctxdata);
   evas_object_del(ctxpopup);

   return NULL;
}

static void
ctxpopup_key_down_cb(void *data, Evas *e EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   ctxpopup_data *ctxdata = data;

   if (!strcmp(ev->key, "Down"))
     ctxdata->relay_cb(ctxdata->data, ctxdata->ctxpopup, (void *) 1);
   else if (!strcmp(ev->key, "Up"))
     ctxdata->relay_cb(ctxdata->data, ctxdata->ctxpopup, (void *) 0);
}

Evas_Object *
ctxpopup_img_preview_create(edit_data *ed,
                            const char *imgpath,
                            Evas_Smart_Cb ctxpopup_dismiss_cb,
                            Evas_Smart_Cb ctxpopup_relay_cb)
{
   //create ctxpopup
   Evas_Object *ctxpopup = elm_ctxpopup_add(base_layout_get());
   if (!ctxpopup) return NULL;

   elm_object_style_set(ctxpopup, elm_app_name_get());
   elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_LEFT,
                                       ELM_CTXPOPUP_DIRECTION_UP,
                                       ELM_CTXPOPUP_DIRECTION_DOWN,
                                       ELM_CTXPOPUP_DIRECTION_RIGHT);
   //ctxpopup data
   ctxpopup_data *ctxdata = malloc(sizeof(ctxpopup_data));
   if (!ctxdata)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }
   ctxdata->relay_cb = ctxpopup_relay_cb;
   ctxdata->data = ed;
   ctxdata->ctxpopup = ctxpopup;
   evas_object_data_set(ctxpopup, "ctxpopup_data", ctxdata);

   //Layout
   Evas_Object *layout = elm_layout_add(ctxpopup);
   elm_layout_file_set(layout, EDJE_PATH, "preview_layout");
   elm_object_content_set(ctxpopup, layout);

   Evas *e = evas_object_evas_get(ctxpopup);
   Evas_Object *img = evas_object_image_filled_add(e);
   evas_object_image_file_set(img, imgpath, NULL);
   Evas_Coord w, h;
   evas_object_image_size_get(img, &w, &h);
   evas_object_size_hint_aspect_set(img, EVAS_ASPECT_CONTROL_BOTH, w, h);
   elm_object_part_content_set(layout, "elm.swallow.img", img);

   evas_object_smart_callback_add(ctxpopup, "dismissed", ctxpopup_dismiss_cb,
                                  ed);
   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL, ctxpopup_del_cb,
                                  ctxdata);
   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_KEY_DOWN,
                                  ctxpopup_key_down_cb, ctxdata);
   evas_object_focus_set(ctxpopup, EINA_TRUE);
   return ctxpopup;
}
