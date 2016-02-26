#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include "enventor_private.h"

#define CTXPOPUP_BORDER_SIZE 10
#define COLORSELECTOR_PRESET_CNT 9

typedef struct ctxpopup_data_s {
   Evas_Smart_Cb changed_cb;
   Evas_Smart_Cb relay_cb;
   void *data;
   Evas_Object *ctxpopup;
   attr_value *attr;
   char candidate[256];

   Eina_List *toggles;

   //colorselector properties
   Evas_Object *colorselector;
   Evas_Object *color_view;
   Evas_Object *input_colors[4];
   Eina_Bool color_changed : 1;

   Ecore_Animator *animator;

   Eina_Bool integer : 1;
} ctxpopup_data;

typedef struct rgba_value_s
{
   int r, g, b, a;
} rgba_value;

typedef struct preset_colors_data_s
{
  Eina_Bool is_created;
  rgba_value rgba[9];
} preset_colors_data;

static rgba_value preset_cur_color;
static preset_colors_data preset_colors =
       { 0,
         255, 0, 0, 255,
         0, 255, 0, 255,
         0, 0, 255, 255,
         255, 255, 0, 255,
         255, 0, 255, 255,
         0, 255, 255, 255,
         0, 0, 0, 255,
         255, 255, 255, 255,
         128, 128, 128, 255};

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
update_preset_colors(Eina_Bool update)
{
   if (update)
     {
        int i, index;
        index = -1;

        for (i=0; i < COLORSELECTOR_PRESET_CNT; ++i)
          if (preset_colors.rgba[i].r == preset_cur_color.r &&
              preset_colors.rgba[i].g == preset_cur_color.g &&
              preset_colors.rgba[i].b == preset_cur_color.b &&
              preset_colors.rgba[i].a == preset_cur_color.a)
            index = i;

        if (index == -1)
          {
             for (i = COLORSELECTOR_PRESET_CNT-1; i > 0; --i)
                preset_colors.rgba[i] = preset_colors.rgba[i-1];
             preset_colors.rgba[0] = preset_cur_color;
          }
        else
          {
             for (i = index; i > 0; --i)
                preset_colors.rgba[i] = preset_colors.rgba[i-1];
             preset_colors.rgba[0] = preset_cur_color;
          }
     }
}

static void
ctxpopup_it_cb(void *data, Evas_Object *obj, void *event_info)
{
   ctxpopup_data *ctxdata = data;
   Elm_Object_Item *it = event_info;
   const char *text = elm_object_item_text_get(it);

   snprintf(ctxdata->candidate, sizeof(ctxdata->candidate), "%s %s%s",
            ctxdata->attr->prepend_str, text, ctxdata->attr->append_str);

   ctxdata->changed_cb(ctxdata->data, obj, ctxdata->candidate);
   elm_ctxpopup_dismiss(obj);
}

static void
ctxpopup_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   elm_config_focus_autoscroll_mode_set(ELM_FOCUS_AUTOSCROLL_MODE_SHOW);
   ctxpopup_data *ctxdata = data;
   eina_list_free(ctxdata->toggles);
   ecore_animator_del(ctxdata->animator);
   free(ctxdata);
}

static void
colorselector_ctxpopup_dismiss_cb(void *data, Evas_Object *obj EINA_UNUSED,
                                  void *event_info EINA_UNUSED)
{
   ctxpopup_data *ctxdata = data;
   update_preset_colors(ctxdata->color_changed);
}

static Eina_Bool
changed_animator_cb(void *data)
{
   ctxpopup_data *ctxdata = data;
   ctxdata->changed_cb(ctxdata->data, ctxdata->ctxpopup, ctxdata->candidate);
   ctxdata->animator = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
slider_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   ctxpopup_data *ctxdata = data;
   double val = elm_slider_value_get(obj);
   char buf[128];

   Evas_Object *box = elm_object_content_get(ctxdata->ctxpopup);
   Eina_List *box_children = elm_box_children_get(box);
   Eina_List *l;
   Evas_Object *layout;
   Evas_Object *slider;

   snprintf(ctxdata->candidate, sizeof(ctxdata->candidate), "%s",
            ctxdata->attr->prepend_str);

   EINA_LIST_FOREACH(box_children, l, layout)
     {
        slider = elm_object_part_content_get(layout,
                                             "elm.swallow.slider");
        if (ctxdata->attr->type & ATTR_VALUE_INTEGER)
          {
             snprintf(buf, sizeof(buf), " %d",
                      (int) roundf(elm_slider_value_get(slider)));
          }
        else
          {
             //if the last digit number is 0 then round up.
             val = elm_slider_value_get(slider);
             snprintf(buf, sizeof(buf), " %0.2f", val);
             double round_down = atof(buf);
             snprintf(buf, sizeof(buf), " %0.1f", val);
             double round_down2 = atof(buf);
             if (fabs(round_down - round_down2) < 0.0005)
               snprintf(buf, sizeof(buf), " %0.1f", val);
             else
               snprintf(buf, sizeof(buf), " %0.2f", val);
          }
        strcat(ctxdata->candidate, buf);
     }
   strcat(ctxdata->candidate, ctxdata->attr->append_str);
   ecore_animator_del(ctxdata->animator);
   ctxdata->animator = ecore_animator_add(changed_animator_cb, ctxdata);
}

static void
entry_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   ctxpopup_data *ctxdata = data;
   Evas_Object *slider = evas_object_data_get(obj, "slider");
   double text_val, val, min_val, max_val;
   char buf[128];

   text_val = atof(elm_object_text_get(obj));
   val = elm_slider_value_get(slider);

   //no change.
   if (fabs(val - text_val) < 0.000006) return;

   elm_slider_min_max_get(slider, &min_val, &max_val);

   if (text_val < min_val) val = min_val;
   else if (text_val > max_val) val = max_val;
   else val = text_val;

   if (val != text_val)
     {
        if (ctxdata->integer) snprintf(buf, sizeof(buf), "%1.0f", val);
        else snprintf(buf, sizeof(buf), "%1.2f", val);
        elm_object_text_set(obj, buf);
     }
   else
     elm_slider_value_set(slider, val);
}

static void
toggle_changed_cb(void *data, Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   ctxpopup_data *ctxdata = data;
   Evas_Object *toggle;
   Eina_List *l;
   char buf[128];

   snprintf(ctxdata->candidate, sizeof(ctxdata->candidate), " %s",
            ctxdata->attr->prepend_str);

   EINA_LIST_FOREACH(ctxdata->toggles, l, toggle)
     {
        snprintf(buf, sizeof(buf), " %d", (int) elm_check_state_get(toggle));
        strcat(ctxdata->candidate, buf);
     }
   strcat(ctxdata->candidate, ctxdata->attr->append_str);
   ctxdata->changed_cb(ctxdata->data, ctxdata->ctxpopup, ctxdata->candidate);
}

static Evas_Object *
toggle_layout_create(Evas_Object *parent, ctxpopup_data *ctxdata,
                     const char *type, Eina_Bool toggle_val)
{
   //Layout
   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "toggle_layout");
   evas_object_show(layout);

   //Type
   if (type) elm_object_part_text_set(layout, "elm.text.type", type);

   //Toggle
   Evas_Object *toggle = elm_check_add(layout);
   elm_object_style_set(toggle, "toggle");
   evas_object_smart_callback_add(toggle, "changed",
                                  toggle_changed_cb,  ctxdata);
   elm_object_part_text_set(toggle, "on", "On");
   elm_object_part_text_set(toggle, "off", "Off");
   elm_check_state_set(toggle, toggle_val);
   evas_object_data_set(toggle, "ctxdata", ctxdata);
   elm_object_part_content_set(layout, "elm.swallow.toggle", toggle);

   ctxdata->toggles = eina_list_append(ctxdata->toggles, toggle);

   return layout;
}

static void
toggle_layout_set(Evas_Object *ctxpopup, ctxpopup_data *ctxdata)
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
   EINA_ARRAY_ITER_NEXT(ctxdata->attr->strs, i, type, itr)
     {
        layout = toggle_layout_create(box, ctxdata, type,
                                     (Eina_Bool) roundf(ctxdata->attr->val[i]));
        if (i % 2) elm_object_signal_emit(layout, "odd,item,set", "");
        elm_box_pack_end(box, layout);
     }

   elm_object_content_set(ctxpopup, box);

   Evas_Object *edje = elm_layout_edje_get(layout);
   edje_object_size_min_calc(edje, &layout_w, NULL);

   if (edit_w <= layout_w + CTXPOPUP_BORDER_SIZE)
     evas_object_del(ctxpopup);
}

static Evas_Object *
slider_layout_create(Evas_Object *parent, ctxpopup_data *ctxdata,
                     const char *type, double slider_val)
{
   attr_value *attr = ctxdata->attr;

   //Layout
   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "slider_layout");
   evas_object_show(layout);

   //Type
   if (type) elm_object_part_text_set(layout, "elm.text.type", type);

   //Slider
   Evas_Object *slider = elm_slider_add(layout);
   elm_slider_span_size_set(slider, 120);
   elm_slider_indicator_show_set(slider, EINA_FALSE);
   elm_slider_min_max_set(slider, attr->min, attr->max);
   elm_slider_value_set(slider, slider_val);
   evas_object_smart_callback_add(slider, "changed", slider_changed_cb,
                                  ctxdata);
   char slider_min[16];
   char slider_max[16];
   double step_size;

   if (ctxdata->integer)
     {
        snprintf(slider_min, sizeof(slider_min), "%1.0f", attr->min);
        snprintf(slider_max, sizeof(slider_max), "%1.0f", attr->max);
        step_size = 1 / (double) (attr->max - attr->min);
        elm_slider_step_set(slider, step_size);
     }
   else
     {
        snprintf(slider_min, sizeof(slider_min), "%1.2f", attr->min);
        snprintf(slider_max, sizeof(slider_max), "%1.2f", attr->max);
        step_size = 1 / (double) (attr->max * 100 - attr->min);
        elm_slider_step_set(slider, step_size);
     }

   elm_object_part_text_set(layout, "elm.text.slider_min", slider_min);
   elm_object_part_text_set(layout, "elm.text.slider_max", slider_max);
   elm_object_part_content_set(layout, "elm.swallow.slider", slider);

   return layout;
}

static void
slider_layout_set(Evas_Object *ctxpopup, ctxpopup_data *ctxdata)
{
   Eina_Stringshare *type;
   Eina_Array_Iterator itr;
   unsigned int i;
   Evas_Coord layout_w = 0, edit_w = 0;

   Evas_Object *edit = elm_object_parent_widget_get(ctxpopup);
   evas_object_geometry_get(edit, NULL, NULL, &edit_w, NULL);

   //Box
   Evas_Object *box = elm_box_add(ctxpopup);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);

   //Layout
   Evas_Object *layout = NULL;
   EINA_ARRAY_ITER_NEXT(ctxdata->attr->strs, i, type, itr)
     {
        layout = slider_layout_create(box, ctxdata, type,
                                      ctxdata->attr->val[i]);
        if (i % 2) elm_object_signal_emit(layout, "odd,item,set", "");
        elm_box_pack_end(box, layout);
     }

   elm_object_content_set(ctxpopup, box);
   Evas_Object *edje = elm_layout_edje_get(layout);
   edje_object_size_min_calc(edje, &layout_w, NULL);

   //Check if the ctxpopup is useless due to it's space.
   if (edit_w <= layout_w + CTXPOPUP_BORDER_SIZE)
     evas_object_del(ctxpopup);
}

static void
colorselector_changed_cb(void *data, Evas_Object *obj,
                         void *event_info EINA_UNUSED)
{
   ctxpopup_data *ctxdata = data;
   int r,g,b,a;

   elm_colorselector_color_get(ctxdata->colorselector, &r, &g, &b, &a);

   elm_spinner_value_set(ctxdata->input_colors[0], r);
   elm_spinner_value_set(ctxdata->input_colors[1], g);
   elm_spinner_value_set(ctxdata->input_colors[2], b);
   elm_spinner_value_set(ctxdata->input_colors[3], a);

   evas_object_color_set(ctxdata->color_view, (r * a) / 255 , (g * a) / 255, (b * a) / 255, a);

   ecore_animator_del(ctxdata->animator);
   sprintf(ctxdata->candidate, ": %d %d %d %d;", r, g, b, a);
   ctxdata->animator = ecore_animator_add(changed_animator_cb, ctxdata);

   preset_cur_color.r = r;
   preset_cur_color.g = g;
   preset_cur_color.b = b;
   preset_cur_color.a = a;

   ctxdata->color_changed = EINA_TRUE;
}

static void
colorselector_inputs_changed_cb(void *data, Evas_Object *obj,
                  void *event_info EINA_UNUSED)
{
   ctxpopup_data *ctxdata = data;
   int r,g,b,a;

   r = elm_spinner_value_get(ctxdata->input_colors[0]);
   g = elm_spinner_value_get(ctxdata->input_colors[1]);
   b = elm_spinner_value_get(ctxdata->input_colors[2]);
   a = elm_spinner_value_get(ctxdata->input_colors[3]);

   evas_object_color_set(ctxdata->color_view, (r * a) /255 , (g * a)/255, (b * a)/255, a);
   elm_colorselector_color_set(ctxdata->colorselector, r, g, b, a);

   ecore_animator_del(ctxdata->animator);
   sprintf(ctxdata->candidate, ": %d %d %d %d;", r, g, b, a);
   ctxdata->animator = ecore_animator_add(changed_animator_cb, ctxdata);

   preset_cur_color.r = r;
   preset_cur_color.g = g;
   preset_cur_color.b = b;
   preset_cur_color.a = a;

   ctxdata->color_changed = EINA_TRUE;
}

static void
colorselector_preset_clicked_cb(void *data,  Evas *e EINA_UNUSED, Evas_Object *obj,
                           void *event_info EINA_UNUSED)
{
   ctxpopup_data *ctxdata = data;

   int r,g,b,a;

   int index = (int)(uintptr_t)evas_object_data_get(obj, "index");
   r = preset_colors.rgba[index].r;
   g = preset_colors.rgba[index].g;
   b = preset_colors.rgba[index].b;
   a = preset_colors.rgba[index].a;

   evas_object_color_set(ctxdata->color_view, (r * a)/255 , (g * a)/255, (b * a)/255, a);
   elm_colorselector_color_set(ctxdata->colorselector, r, g, b, a);

   elm_spinner_value_set(ctxdata->input_colors[0], r);
   elm_spinner_value_set(ctxdata->input_colors[1], g);
   elm_spinner_value_set(ctxdata->input_colors[2], b);
   elm_spinner_value_set(ctxdata->input_colors[3], a);

   ecore_animator_del(ctxdata->animator);
   sprintf(ctxdata->candidate, ": %d %d %d %d;", r, g, b, a);
   ctxdata->animator = ecore_animator_add(changed_animator_cb, ctxdata);

   preset_cur_color.r = r;
   preset_cur_color.g = g;
   preset_cur_color.b = b;
   preset_cur_color.a = a;

   ctxdata->color_changed = EINA_TRUE;
}

static Evas_Object *
colorselector_layout_create(Evas_Object *ctxpopup, ctxpopup_data *ctxdata)
{
   Eina_Stringshare *type;
   Eina_Array_Iterator itr;
   unsigned int i;

   //Layout
   Evas_Object *layout = elm_layout_add(ctxpopup);
   elm_layout_file_set(layout, EDJE_PATH, "colorselector_layout");
   evas_object_show(layout);

   int r,g,b,a;
   r = ctxdata->attr->val[0];
   g = ctxdata->attr->val[1];
   b = ctxdata->attr->val[2];
   a = ctxdata->attr->val[3];

   //Colorselector
   ctxdata->colorselector = elm_colorselector_add(layout);
   elm_colorselector_color_set(ctxdata->colorselector, r, g, b, a);
   elm_colorselector_mode_set(ctxdata->colorselector, ELM_COLORSELECTOR_COMPONENTS);
   evas_object_size_hint_weight_set(ctxdata->colorselector, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ctxdata->colorselector, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(layout, "elm.swallow.colorselector", ctxdata->colorselector);
   evas_object_smart_callback_add(ctxdata->colorselector, "changed", colorselector_changed_cb, ctxdata);

   //Color View
   ctxdata->color_view = evas_object_rectangle_add(evas_object_evas_get(layout));
   evas_object_color_set(ctxdata->color_view, (r * a) / 255 , (g * a) / 255, (b * a) / 255, a);
   evas_object_show(ctxdata->color_view);
   elm_object_part_content_set(layout, "elm.swallow.color_view", ctxdata->color_view);

   //Color Inputs Text
   elm_object_part_text_set(layout, "elm.text.red", "R:");
   elm_object_part_text_set(layout, "elm.text.green", "G:");
   elm_object_part_text_set(layout, "elm.text.blue", "B:");
   elm_object_part_text_set(layout, "elm.text.alpha", "A:");

   //Color Inputs
   Evas_Object *inputs_box = elm_box_add(layout);
   evas_object_size_hint_weight_set(inputs_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(inputs_box, EVAS_HINT_FILL, EVAS_HINT_FILL);

   EINA_ARRAY_ITER_NEXT(ctxdata->attr->strs, i, type, itr)
     {
        ctxdata->input_colors[i] = elm_spinner_add(inputs_box);
        elm_spinner_editable_set(ctxdata->input_colors[i], EINA_TRUE);
        elm_spinner_step_set(ctxdata->input_colors[i], 1.0);
        elm_spinner_min_max_set(ctxdata->input_colors[i], 0, 255);
        elm_spinner_value_set(ctxdata->input_colors[i], ctxdata->attr->val[i]);
        evas_object_size_hint_align_set(ctxdata->input_colors[i], EVAS_HINT_FILL, 0.5);
        evas_object_size_hint_weight_set(ctxdata->input_colors[i], EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_show(ctxdata->input_colors[i]);
        evas_object_smart_callback_add(ctxdata->input_colors[i], "changed", colorselector_inputs_changed_cb, ctxdata);
     }

   elm_object_part_content_set(layout, "elm.swallow.input_red", ctxdata->input_colors[0]);
   elm_object_part_content_set(layout, "elm.swallow.input_green", ctxdata->input_colors[1]);
   elm_object_part_content_set(layout, "elm.swallow.input_blue", ctxdata->input_colors[2]);
   elm_object_part_content_set(layout, "elm.swallow.input_alpha", ctxdata->input_colors[3]);

   //Preset Colors
   Evas_Object *colors_box = elm_box_add(layout);
   elm_box_horizontal_set(colors_box, EINA_TRUE);
   elm_box_padding_set(colors_box, ELM_SCALE_SIZE(8), 0);
   evas_object_size_hint_weight_set(colors_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(colors_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(colors_box);

   for (i = 0; i < COLORSELECTOR_PRESET_CNT; i++)
     {
        Evas_Object *rect = evas_object_rectangle_add(evas_object_evas_get(layout));
        evas_object_color_set(rect,
                              preset_colors.rgba[i].r * preset_colors.rgba[i].a / 255,
                              preset_colors.rgba[i].g * preset_colors.rgba[i].a / 255,
                              preset_colors.rgba[i].b * preset_colors.rgba[i].a / 255,
                              preset_colors.rgba[i].a);
        evas_object_data_set(rect, "index", (void*)(uintptr_t)i);
        evas_object_size_hint_min_set(rect, ELM_SCALE_SIZE(20), ELM_SCALE_SIZE(20));
        evas_object_show(rect);
        elm_box_pack_end(colors_box, rect);
        evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_DOWN , colorselector_preset_clicked_cb, ctxdata);
     }
   elm_object_part_content_set(layout, "elm.swallow.preset_colors", colors_box);

   return layout;
}

static void
colorselector_layout_set(Evas_Object *ctxpopup, ctxpopup_data *ctxdata)
{
   Evas_Coord layout_w = 0, edit_w = 0;
   Evas_Object *edit = elm_object_parent_widget_get(ctxpopup);
   evas_object_geometry_get(edit, NULL, NULL, &edit_w, NULL);
   evas_object_smart_callback_add(ctxpopup, "dismissed", colorselector_ctxpopup_dismiss_cb,
                                  ctxdata);
   //Box
   Evas_Object *box = elm_box_add(ctxpopup);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);

   Evas_Object *layout = NULL;
   layout = colorselector_layout_create(box, ctxdata);

   elm_box_pack_end(box, layout);

   elm_object_content_set(ctxpopup, box);
   Evas_Object *edje = elm_layout_edje_get(layout);
   edje_object_size_min_calc(edje, &layout_w, NULL);

   //Check if the ctxpopup is useless due to it's space.
   if (edit_w <= layout_w + CTXPOPUP_BORDER_SIZE)
     evas_object_del(ctxpopup);
}

static void
constant_candidate_set(Evas_Object *ctxpopup, ctxpopup_data *ctxdata)
{
   Eina_Stringshare *candidate;
   Eina_Array_Iterator itr;
   unsigned int i;

   EINA_ARRAY_ITER_NEXT(ctxdata->attr->strs, i, candidate, itr)
      elm_ctxpopup_item_append(ctxpopup, candidate, NULL, ctxpopup_it_cb,
                               ctxdata);
}

static Eina_Bool
part_candidate_set(Evas_Object *ctxpopup, ctxpopup_data *ctxdata)
{
   view_data *vd = edj_mgr_view_get(NULL);
   if (!vd) return EINA_FALSE;
   Eina_List *parts = view_parts_list_get(vd);
   Eina_List *l;
   char *part;
   EINA_LIST_FOREACH(parts, l, part)
     {
        snprintf(ctxdata->candidate, sizeof(ctxdata->candidate), "\"%s\"",
                 part);
        elm_ctxpopup_item_append(ctxpopup, ctxdata->candidate, NULL,
                                 ctxpopup_it_cb, ctxdata);
     }
   view_string_list_free(parts);
   return EINA_TRUE;
}

static Eina_Bool
image_candidate_set(Evas_Object *ctxpopup, ctxpopup_data *ctxdata)
{
   view_data *vd = edj_mgr_view_get(NULL);
   if (!vd) return EINA_FALSE;
   Eina_List *parts = view_images_list_get(vd);
   Eina_List *l;
   char *part;
   EINA_LIST_FOREACH(parts, l, part)
      elm_ctxpopup_item_append(ctxpopup, part, NULL, ctxpopup_it_cb,
                               ctxdata);
   view_string_list_free(parts);
   return EINA_TRUE;
}

static Eina_Bool
program_candidate_set(Evas_Object *ctxpopup, ctxpopup_data *ctxdata)
{
   const char *PROGRAM_GEN = "program_0x";
   int PROGRAM_GEN_LEN = 10;
   int candidate_cntr = 0;
   view_data *vd = edj_mgr_view_get(NULL);
   if (!vd) return EINA_FALSE;
   Eina_List *parts = view_programs_list_get(vd);
   Eina_List *l;
   char *part;
   EINA_LIST_FOREACH(parts, l, part)
     {
        if (!strncmp(part, PROGRAM_GEN, PROGRAM_GEN_LEN))
           continue;
        snprintf(ctxdata->candidate, sizeof(ctxdata->candidate), "\"%s\"",
                 part);
        elm_ctxpopup_item_append(ctxpopup, ctxdata->candidate, NULL,
                                 ctxpopup_it_cb, ctxdata);
        candidate_cntr++;
     }
   view_string_list_free(parts);
   return candidate_cntr ? EINA_TRUE : EINA_FALSE;
}

static Eina_Bool
state_candidate_set(Evas_Object *ctxpopup, ctxpopup_data *ctxdata,
                    edit_data *ed)
{
   view_data *vd = edj_mgr_view_get(NULL);
   if (!vd) return EINA_FALSE;

   Eina_Stringshare *program = NULL;
   Eina_List *targets = NULL;
   Eina_Stringshare *target = NULL;
   Eina_Bool ret = EINA_FALSE;

   //Trace the part name from the program.
   if (ctxdata->attr->program)
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

   char *state;
   EINA_LIST_FREE(converted, state)
     {
        snprintf(ctxdata->candidate, sizeof(ctxdata->candidate), "\"%s\"",
                 state);
        elm_ctxpopup_item_append(ctxpopup, ctxdata->candidate, NULL,
                                 ctxpopup_it_cb, ctxdata);
        free(state);
     }
   view_string_list_free(states);
   ret = EINA_TRUE;
end:
   eina_stringshare_del(program);
   view_string_list_free(targets);
   if (!ctxdata->attr->program && target) eina_stringshare_del(target);
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

void
ctxpopup_img_preview_reload(Evas_Object *ctxpopup, const char *imgpath)
{
   if (!ctxpopup) return;

   Evas_Object *layout = elm_object_content_get(ctxpopup);
   Evas_Object *img = elm_object_part_content_get(layout, "elm.swallow.img");
   evas_object_image_file_set(img, imgpath, NULL);
}

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
   elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_UP,
                                       ELM_CTXPOPUP_DIRECTION_DOWN,
                                       ELM_CTXPOPUP_DIRECTION_RIGHT,
                                       ELM_CTXPOPUP_DIRECTION_LEFT);
   //ctxpopup data
   ctxpopup_data *ctxdata = calloc(1, sizeof(ctxpopup_data));
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

static Eina_Bool
is_colorselector_type(ctxpopup_data *ctxdata)
{
   ctxdata->integer = EINA_TRUE;
   const char* integer_type = eina_array_data_get(ctxdata->attr->strs, 0);
   if (!strcmp(integer_type, "R:"))
     {
       if (preset_colors.is_created == EINA_FALSE)
         preset_colors.is_created = EINA_TRUE;

        return EINA_TRUE;
     }
    return EINA_FALSE;
}

Evas_Object *
ctxpopup_candidate_list_create(edit_data *ed, attr_value *attr,
                               Evas_Smart_Cb ctxpopup_dismiss_cb,
                               Evas_Smart_Cb ctxpopup_changed_cb,
                               Enventor_Ctxpopup_Type *type)
{
   //create ctxpopup
   Evas_Object *ctxpopup = elm_ctxpopup_add(edit_obj_get(ed));
   if (!ctxpopup) return NULL;

   elm_config_focus_autoscroll_mode_set(ELM_FOCUS_AUTOSCROLL_MODE_NONE);

   elm_object_style_set(ctxpopup, "enventor");
   elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_UP,
                                       ELM_CTXPOPUP_DIRECTION_DOWN,
                                       ELM_CTXPOPUP_DIRECTION_RIGHT,
                                       ELM_CTXPOPUP_DIRECTION_LEFT);
   //ctxpopup data
   ctxpopup_data *ctxdata = calloc(1, sizeof(ctxpopup_data));
   if (!ctxdata)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        goto err;
     }
   ctxdata->changed_cb = ctxpopup_changed_cb;
   ctxdata->ctxpopup = ctxpopup;
   ctxdata->attr = attr;
   ctxdata->data = ed;

   switch (attr->type)
     {
        case ATTR_VALUE_BOOLEAN:
          {
             toggle_layout_set(ctxpopup, ctxdata);
             *type = ENVENTOR_CTXPOPUP_TYPE_TOGGLE;
             break;
          }
        case ATTR_VALUE_INTEGER:
          {
             ctxdata->integer = EINA_TRUE;
             if (is_colorselector_type(ctxdata))
               colorselector_layout_set(ctxpopup, ctxdata);
             else
               slider_layout_set(ctxpopup, ctxdata);
             *type = ENVENTOR_CTXPOPUP_TYPE_SLIDER;
             break;
          }
        case ATTR_VALUE_FLOAT:
          {
             ctxdata->integer = EINA_FALSE;
             slider_layout_set(ctxpopup, ctxdata);
             *type = ENVENTOR_CTXPOPUP_TYPE_SLIDER;
             break;
          }
        case ATTR_VALUE_CONSTANT:
          {
             constant_candidate_set(ctxpopup, ctxdata);
             *type = ENVENTOR_CTXPOPUP_TYPE_LIST;
             break;
          }
        case ATTR_VALUE_PART:
          {
             if (!part_candidate_set(ctxpopup, ctxdata)) goto err;
             *type = ENVENTOR_CTXPOPUP_TYPE_LIST;
             break;
          }
        case ATTR_VALUE_STATE:
          {
             if (!state_candidate_set(ctxpopup, ctxdata, ed)) goto err;
             *type = ENVENTOR_CTXPOPUP_TYPE_LIST;
             break;
          }
        case ATTR_VALUE_IMAGE:
          {
             if (!image_candidate_set(ctxpopup, ctxdata)) goto err;
             *type = ENVENTOR_CTXPOPUP_TYPE_IMAGE;
             break;
          }
        case ATTR_VALUE_PROGRAM:
          {
             if (!program_candidate_set(ctxpopup, ctxdata)) goto err;
             *type = ENVENTOR_CTXPOPUP_TYPE_LIST;
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
