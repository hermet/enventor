#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include "enventor_private.h"

#define CTXPOPUP_BORDER_SIZE 10

typedef struct ctxpopup_data_s {
   Evas_Smart_Cb selected_cb;
   Evas_Smart_Cb relay_cb;
   Evas_Object *ctxpopup;
   void *data;
} ctxpopup_data;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
btn_up_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   Evas_Object *layout = data;
   Evas_Object *slider = elm_object_part_content_get(layout,
                                                     "elm.swallow.slider");
   Evas_Object *entry = elm_object_part_content_get(layout,
                                                    "elm.swallow.entry");
   attr_value *attr = evas_object_data_get(slider, "attr");
   void *integer = evas_object_data_get(slider, "integer");
   double value = elm_slider_value_get(slider);
   char buf[128];

   if (attr->type & ATTR_VALUE_INTEGER) value += 1;
   else value += 0.01;

   if (integer) snprintf(buf, sizeof(buf), "%1.0f", value);
   else snprintf(buf, sizeof(buf), "%1.2f", value);
   elm_object_text_set(entry, buf);
}

static void
btn_down_cb(void *data, Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   Evas_Object *layout = data;
   Evas_Object *slider = elm_object_part_content_get(layout,
                                                     "elm.swallow.slider");
   Evas_Object *entry = elm_object_part_content_get(layout,
                                                    "elm.swallow.entry");
   attr_value *attr = evas_object_data_get(slider, "attr");
   void *integer = evas_object_data_get(slider, "integer");
   double value = elm_slider_value_get(slider);
   char buf[128];

   if (attr->type & ATTR_VALUE_INTEGER) value -= 1;
   else value -= 0.01;

   if (integer) snprintf(buf, sizeof(buf), "%1.0f", value);
   else snprintf(buf, sizeof(buf), "%1.2f", value);
   elm_object_text_set(entry, buf);
}

static void
ctxpopup_it_cb(void *data, Evas_Object *obj, void *event_info)
{
   attr_value *attr = data;
   Elm_Object_Item *it = event_info;
   ctxpopup_data *ctxdata = evas_object_data_get(obj, "ctxpopup_data");
   const char *text = elm_object_item_text_get(it);
   char candidate[128];

   snprintf(candidate, sizeof(candidate), "%s%s%s", attr->prepend_str, text,
            attr->append_str);

   ctxdata->selected_cb(ctxdata->data, obj, (void *) candidate);
}

static void
toggle_dismiss_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   attr_value *attr = data;
   Evas_Object *box = elm_object_content_get(obj);
   Evas_Object *layout;
   Evas_Object *toggle;
   Eina_List *box_children = elm_box_children_get(box);
   Eina_List *l;
   char candidate[512] = {0};
   char buf[128];

   if (eina_list_count(box_children) == 0) return;

   snprintf(candidate, sizeof(candidate), "%s", attr->prepend_str);

   EINA_LIST_FOREACH(box_children, l, layout)
     {
        toggle = elm_object_part_content_get(layout,
                                             "elm.swallow.toggle");
        snprintf(buf, sizeof(buf), "%d", (int) elm_check_state_get(toggle));
        strcat(candidate, buf);
        strcat(candidate, " ");
     }
   candidate[strlen(candidate) - 1] = '\0'; //Remove last appended " ".
   strcat(candidate, attr->append_str);

   ctxpopup_data *ctxdata = evas_object_data_get(obj, "ctxpopup_data");
   ctxdata->selected_cb(ctxdata->data, obj, candidate);
}

static void
slider_dismiss_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   attr_value *attr = data;
   Evas_Object *box = elm_object_content_get(obj);
   Evas_Object *layout;
   Evas_Object *slider;
   Eina_List *box_children = elm_box_children_get(box);
   Eina_List *l;
   char candidate[512] = {0};
   char buf[128];

   if (eina_list_count(box_children) == 0) return;

   snprintf(candidate, sizeof(candidate), "%s", attr->prepend_str);

   EINA_LIST_FOREACH(box_children, l, layout)
     {
        slider = elm_object_part_content_get(layout,
                                             "elm.swallow.slider");
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
        strcat(candidate, buf);
        strcat(candidate, " ");
     }
   candidate[strlen(candidate) - 1] = '\0'; //Remove last appended " ".
   strcat(candidate, attr->append_str);

   ctxpopup_data *ctxdata = evas_object_data_get(obj, "ctxpopup_data");
   ctxdata->selected_cb(ctxdata->data, obj, candidate);
}

static void
ctxpopup_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   elm_config_focus_autoscroll_mode_set(ELM_FOCUS_AUTOSCROLL_MODE_SHOW);
   free(data);
}

static void
slider_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *entry = data;
   double val = elm_slider_value_get(obj);
   char buf[128];
   void *integer = evas_object_data_get(obj, "integer");

   if (integer) snprintf(buf, sizeof(buf), "%1.0f", val);
   else snprintf(buf, sizeof(buf), "%1.2f", val);
   elm_object_text_set(entry, buf);
}

static void
entry_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *slider = data;
   double text_val, val, min_val, max_val;
   char buf[128];
   void *integer = evas_object_data_get(slider, "integer");

   text_val = atof(elm_object_text_get(obj));
   elm_slider_min_max_get(slider, &min_val, &max_val);

   if (text_val < min_val) val = min_val;
   else if (text_val > max_val) val = max_val;
   else val = text_val;

   if (val != text_val)
     {
        if (integer) snprintf(buf, sizeof(buf), "%1.0f", val);
        else snprintf(buf, sizeof(buf), "%1.2f", val);
        elm_object_text_set(obj, buf);
     }
   else
     elm_slider_value_set(slider, val);
}

static Evas_Object *
toggle_layout_create(Evas_Object *parent, attr_value *attr,
                     const char *type, Eina_Bool toggle_val)
{
   //Layout
   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "toggle_layout");
   evas_object_show(layout);

   //Type
   if (type)
     elm_object_part_text_set(layout, "elm.text.type", type);

   //Toggle
   Evas_Object *toggle = elm_check_add(layout);
   elm_object_style_set(toggle, "toggle");
   elm_object_part_text_set(toggle, "on", "On");
   elm_object_part_text_set(toggle, "off", "Off");
   elm_check_state_set(toggle, toggle_val);
   evas_object_data_set(toggle, "attr", attr);
   elm_object_part_content_set(layout, "elm.swallow.toggle", toggle);

   return layout;
}

static void
toggle_layout_set(Evas_Object *ctxpopup, attr_value *attr)
{
   Eina_Stringshare *type;
   Eina_Array_Iterator itr;
   unsigned int i;
   Evas_Coord layout_w = 0, edit_w = 0;

   Evas_Object *edit = elm_object_parent_widget_get(ctxpopup);
   evas_object_geometry_get(edit, NULL, NULL, &edit_w, NULL);

   //Box
   Evas_Object *box = elm_box_add(ctxpopup);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   //Layout
   Evas_Object *layout = NULL;
   EINA_ARRAY_ITER_NEXT(attr->strs, i, type, itr)
     {
        layout = toggle_layout_create(box, attr, type,
                                      (Eina_Bool) roundf(attr->val[i]));
        if (i % 2) elm_object_signal_emit(layout, "odd,item,set", "");
        elm_box_pack_end(box, layout);
     }

   elm_object_content_set(ctxpopup, box);

   Evas_Object *edje = elm_layout_edje_get(layout);
   edje_object_size_min_calc(edje, &layout_w, NULL);

   evas_object_smart_callback_add(ctxpopup, "dismissed", toggle_dismiss_cb,
                                  (void *) attr);
   if (edit_w <= layout_w + CTXPOPUP_BORDER_SIZE)
     evas_object_del(ctxpopup);
}

static Evas_Object *
slider_layout_create(Evas_Object *parent, attr_value *attr,
                     const char *type, double slider_val, Eina_Bool integer)
{
   //Layout
   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "slider_layout");
   evas_object_show(layout);

   //Type
   if (type)
     elm_object_part_text_set(layout, "elm.text.type", type);

   //Slider
   Evas_Object *slider = elm_slider_add(layout);
   elm_slider_span_size_set(slider, 120);
   elm_slider_indicator_show_set(slider, EINA_FALSE);
   elm_slider_min_max_set(slider, attr->min, attr->max);
   elm_slider_value_set(slider, slider_val);
   evas_object_data_set(slider, "attr", attr);
   evas_object_data_set(slider, "integer", integer ? &integer : NULL);

   char slider_min[16];
   char slider_max[16];
   if (integer)
     {
        snprintf(slider_min, sizeof(slider_min), "%1.0f", attr->min);
        snprintf(slider_max, sizeof(slider_max), "%1.0f", attr->max);
     }
   else
     {
        snprintf(slider_min, sizeof(slider_min), "%1.2f", attr->min);
        snprintf(slider_max, sizeof(slider_max), "%1.2f", attr->max);
     }
   elm_object_part_text_set(layout, "elm.text.slider_min", slider_min);
   elm_object_part_text_set(layout, "elm.text.slider_max", slider_max);
   elm_object_part_content_set(layout, "elm.swallow.slider", slider);

   //Entry
   char buf[128];
   Evas_Object *entry = elm_entry_add(layout);
   elm_entry_context_menu_disabled_set(entry, EINA_TRUE);
   elm_entry_single_line_set(entry, EINA_TRUE);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   if (integer) snprintf(buf, sizeof(buf), "%1.0f", slider_val);
   else snprintf(buf, sizeof(buf), "%1.2f", slider_val);
   elm_object_text_set(entry, buf);
   elm_object_part_content_set(layout, "elm.swallow.entry", entry);

   Elm_Entry_Filter_Accept_Set digits_filter_data;
   Elm_Entry_Filter_Limit_Size limit_filter_data;
   if (integer)
     {
        digits_filter_data.accepted = "0123456789";
        digits_filter_data.rejected = NULL;
        limit_filter_data.max_char_count = 4;
     }
   else
     {
        digits_filter_data.accepted = ".0123456789";
        digits_filter_data.rejected = NULL;
        limit_filter_data.max_char_count = 5;
     }
   elm_entry_markup_filter_append(entry, elm_entry_filter_accept_set,
                                  &digits_filter_data);
   elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size,
                                  &limit_filter_data);

   evas_object_smart_callback_add(slider, "changed", slider_changed_cb, entry);
   evas_object_smart_callback_add(entry, "changed", entry_changed_cb, slider);

   Evas_Object *btn;
   Evas_Object *img;

   //Down Button
   btn = elm_button_add(layout);
   evas_object_smart_callback_add(btn, "clicked", btn_down_cb, layout);
   elm_object_part_content_set(layout, "elm.swallow.down", btn);

   //Down Image
   img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, "down");
   elm_object_content_set(btn, img);

   //Up Button
   btn = elm_button_add(layout);
   evas_object_smart_callback_add(btn, "clicked", btn_up_cb, layout);
   elm_object_part_content_set(layout, "elm.swallow.up", btn);

   //Up Image
   img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, "up");
   elm_object_content_set(btn, img);

   return layout;
}

static void
slider_layout_set(Evas_Object *ctxpopup, attr_value *attr, Eina_Bool integer)
{
   Eina_Stringshare *type;
   Eina_Array_Iterator itr;
   unsigned int i;
   Evas_Coord layout_w = 0, edit_w = 0;

   Evas_Object *edit = elm_object_parent_widget_get(ctxpopup);
   evas_object_geometry_get(edit, NULL, NULL, &edit_w, NULL);

   //Box
   Evas_Object *box = elm_box_add(ctxpopup);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   //Layout
   Evas_Object *layout = NULL;
   EINA_ARRAY_ITER_NEXT(attr->strs, i, type, itr)
     {
        layout = slider_layout_create(box, attr, type, attr->val[i], integer);
        if (i % 2) elm_object_signal_emit(layout, "odd,item,set", "");
        elm_box_pack_end(box, layout);
     }

   elm_object_content_set(ctxpopup, box);
   evas_object_smart_callback_add(ctxpopup, "dismissed", slider_dismiss_cb,
                                  (void *) attr);
   Evas_Object *edje = elm_layout_edje_get(layout);
   edje_object_size_min_calc(edje, &layout_w, NULL);

   if (edit_w <= layout_w + CTXPOPUP_BORDER_SIZE)
     evas_object_del(ctxpopup);

}

static void
constant_candidate_set(Evas_Object *ctxpopup, attr_value *attr)
{
   Eina_Stringshare *candidate;
   Eina_Array_Iterator itr;
   unsigned int i;

   EINA_ARRAY_ITER_NEXT(attr->strs, i, candidate, itr)
      elm_ctxpopup_item_append(ctxpopup, candidate, NULL, ctxpopup_it_cb,
                               (void *) attr);
}

static Eina_Bool
part_candidate_set(Evas_Object *ctxpopup, attr_value *attr)
{
   view_data *vd = edj_mgr_view_get(NULL);
   if (!vd) return EINA_FALSE;
   Eina_List *parts = view_parts_list_get(vd);
   Eina_List *l;
   char *part;
   char candidate[128];
   EINA_LIST_FOREACH(parts, l, part)
     {
        snprintf(candidate, sizeof(candidate), "\"%s\"", part);
        elm_ctxpopup_item_append(ctxpopup, candidate, NULL, ctxpopup_it_cb,
                                 (void *) attr);
     }
   view_string_list_free(parts);
   return EINA_TRUE;
}

static Eina_Bool
image_candidate_set(Evas_Object *ctxpopup, attr_value *attr)
{
   view_data *vd = edj_mgr_view_get(NULL);
   if (!vd) return EINA_FALSE;
   Eina_List *parts = view_images_list_get(vd);
   Eina_List *l;
   char *part;
   EINA_LIST_FOREACH(parts, l, part)
      elm_ctxpopup_item_append(ctxpopup, part, NULL, ctxpopup_it_cb,
                               (void *) attr);
   view_string_list_free(parts);
   return EINA_TRUE;
}

static Eina_Bool
program_candidate_set(Evas_Object *ctxpopup, attr_value *attr)
{
   view_data *vd = edj_mgr_view_get(NULL);
   if (!vd) return EINA_FALSE;
   Eina_List *parts = view_programs_list_get(vd);
   Eina_List *l;
   char *part;
   char candidate[128];
   EINA_LIST_FOREACH(parts, l, part)
     {
        snprintf(candidate, sizeof(candidate), "\"%s\"", part);
        elm_ctxpopup_item_append(ctxpopup, candidate, NULL, ctxpopup_it_cb,
                                 (void *) attr);
     }
   view_string_list_free(parts);
   return EINA_TRUE;
}

static Eina_Bool
state_candidate_set(Evas_Object *ctxpopup, attr_value *attr, edit_data *ed)
{
   view_data *vd = edj_mgr_view_get(NULL);
   if (!vd) return EINA_FALSE;

   Eina_Stringshare *program = NULL;
   Eina_List *targets = NULL;
   Eina_Stringshare *target = NULL;
   Eina_Bool ret = EINA_FALSE;

   //Trace the part name from the program.
   if (attr->program)
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
   char candidate[128];
   EINA_LIST_FOREACH(converted, l, state)
     {
        snprintf(candidate, sizeof(candidate), "\"%s\"", state);
        elm_ctxpopup_item_append(ctxpopup, candidate, NULL, ctxpopup_it_cb,
                                 (void *) attr);
        free(state);
     }
   view_string_list_free(states);
   eina_list_free(converted);
   ret = EINA_TRUE;
end:
   eina_stringshare_del(program);
   view_string_list_free(targets);
   if (!attr->program && target) eina_stringshare_del(target);
   return ret;
}

static void
image_relay(ctxpopup_data *ctxdata, Eina_Bool up)
{
   if (up)
     ctxdata->relay_cb(ctxdata->data, ctxdata->ctxpopup, (void *) 0);
   else
     ctxdata->relay_cb(ctxdata->data, ctxdata->ctxpopup, (void *) 1);
}

static void
ctxpopup_mouse_wheel_cb(void *data, Evas *e EINA_UNUSED,
                        Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Wheel *ev = event_info;
   ctxpopup_data *ctxdata = data;

   if (ev->z > 0) image_relay(ctxdata, EINA_FALSE);
   else if (ev->z < 0) image_relay(ctxdata, EINA_TRUE);
}

static void
ctxpopup_key_down_cb(void *data, Evas *e EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   ctxpopup_data *ctxdata = data;

   if (!strcmp(ev->key, "Down")) image_relay(ctxdata, EINA_FALSE);
   else if (!strcmp(ev->key, "Up")) image_relay(ctxdata, EINA_TRUE);
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

Evas_Object *
ctxpopup_img_preview_create(edit_data *ed,
                            const char *imgpath,
                            Evas_Smart_Cb ctxpopup_dismiss_cb,
                            Evas_Smart_Cb ctxpopup_relay_cb)
{
   //create ctxpopup
   Evas_Object *ctxpopup = elm_ctxpopup_add(edit_obj_get(ed));
   if (!ctxpopup) return NULL;

   elm_config_focus_autoscroll_mode_set(ELM_FOCUS_AUTOSCROLL_MODE_NONE);

   elm_object_style_set(ctxpopup, "enventor");
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
   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_MOUSE_WHEEL,
                                  ctxpopup_mouse_wheel_cb, ctxdata);
   evas_object_focus_set(ctxpopup, EINA_TRUE);

   return ctxpopup;
}

Evas_Object *
ctxpopup_candidate_list_create(edit_data *ed, attr_value *attr,
                               Evas_Smart_Cb ctxpopup_dismiss_cb,
                               Evas_Smart_Cb ctxpopup_selected_cb)
{
   //create ctxpopup
   Evas_Object *ctxpopup = elm_ctxpopup_add(edit_obj_get(ed));
   if (!ctxpopup) return NULL;

   elm_config_focus_autoscroll_mode_set(ELM_FOCUS_AUTOSCROLL_MODE_NONE);

   elm_object_style_set(ctxpopup, "enventor");
   elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_RIGHT,
                                       ELM_CTXPOPUP_DIRECTION_LEFT,
                                       ELM_CTXPOPUP_DIRECTION_UP,
                                       ELM_CTXPOPUP_DIRECTION_DOWN);
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
        case ATTR_VALUE_BOOLEAN:
          {
             toggle_layout_set(ctxpopup, attr);
             break;
          }
        case ATTR_VALUE_INTEGER:
          {
             slider_layout_set(ctxpopup, attr, EINA_TRUE);
             break;
          }
        case ATTR_VALUE_FLOAT:
          {
             slider_layout_set(ctxpopup, attr, EINA_FALSE);
             break;
          }
        case ATTR_VALUE_CONSTANT:
          {
             constant_candidate_set(ctxpopup, attr);
             break;
          }
        case ATTR_VALUE_PART:
          {
             if (!part_candidate_set(ctxpopup, attr)) goto err;
             break;
          }
        case ATTR_VALUE_STATE:
          {
             if (!state_candidate_set(ctxpopup, attr, ed)) goto err;
             break;
          }
        case ATTR_VALUE_IMAGE:
          {
             if (!image_candidate_set(ctxpopup, attr)) goto err;
             break;
          }
        case ATTR_VALUE_PROGRAM:
          {
             if (!program_candidate_set(ctxpopup, attr)) goto err;
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

