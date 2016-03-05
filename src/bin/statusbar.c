#include "common.h"

#define VIEW_RESIZE_TYPE_W "W"
#define VIEW_RESIZE_TYPE_H "H"

typedef struct statusbar_s
{
   Evas_Object *layout;
   Evas_Object *ctxpopup;
   Eina_Stringshare *group_name;
   int cur_line;
   int max_line;
} stats_data;

typedef struct invert_transit_data_s
{
   Evas_Coord orig_w;
   Evas_Coord orig_h;
   Evas_Coord diff_w;
   Evas_Coord diff_h;
} invert_data;

stats_data *g_sd = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
view_scale_slider_changed_cb(void *data EINA_UNUSED, Evas_Object *obj,
                             void *event_info EINA_UNUSED)
{
   double scale = elm_slider_value_get(obj);
   double rounded = ROUNDING(scale, 1);

   if ((rounded - scale) > 0) rounded -= 0.05;

   /* Here logic is mostly duplicated with main_mouse_wheel_cb() in main.c */

   config_view_scale_set(rounded);
   scale = config_view_scale_get();
   enventor_object_live_view_scale_set(base_enventor_get(), scale);

   Evas_Coord w, h;
   config_view_size_get(&w, &h);
   enventor_object_live_view_size_set(base_enventor_get(), w, h);

   //Just in live edit mode case.
   live_edit_update();

   stats_view_scale_update(scale);
}

static void
ctxpopup_dismissed_cb(void *data, Evas_Object *obj,
                      void *event_info EINA_UNUSED)
{
   stats_data *sd = data;
   enventor_object_focus_set(base_enventor_get(), EINA_TRUE);
   evas_object_del(obj);
   sd->ctxpopup = NULL;
}

static void
view_invert_transit_op(void *data, Elm_Transit *transit EINA_UNUSED,
                       double progress)
{
   invert_data *id = data;
   Evas_Coord w, h;
   w = id->orig_w + ((double)(id->diff_w)) * progress;
   h = id->orig_h + ((double)(id->diff_h)) * progress;

   enventor_object_live_view_size_set(base_enventor_get(), w, h);

   //Just in live edit mode case.
   live_edit_update();
}

static void
view_invert_transit_end(void *data, Elm_Transit *transit EINA_UNUSED)
{
   invert_data *id = data;
   config_view_size_set((id->orig_w + id->diff_w), (id->orig_h + id->diff_h));
   free(id);
}

static void
view_invert_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   invert_data *id = malloc(sizeof(invert_data));

   Evas_Coord w, h;
   config_view_size_get(&w, &h);
   id->orig_w = w;
   id->orig_h = h;
   id->diff_w = h - w;
   id->diff_h = w - h;

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, view_invert_transit_op, id,
                          view_invert_transit_end);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_go(transit);
}

static void
view_resize_slider_changed_cb(void *data, Evas_Object *obj EINA_UNUSED,
                              void *event_info EINA_UNUSED)
{
   Evas_Object *layout = data;
   Eina_Bool horizontal;
   const char *type = elm_object_part_text_get(layout, "elm.text.type");
   if (type && !strcmp(type, VIEW_RESIZE_TYPE_W))
     horizontal = EINA_TRUE;
   else
     horizontal = EINA_FALSE;

   Evas_Object *slider = elm_object_part_content_get(layout,
                                                     "elm.swallow.slider");
   int val = elm_slider_value_get(slider);
   int w, h;
   config_view_size_get(&w, &h);
   if (horizontal)
     {
        config_view_size_set(val, h);
        enventor_object_live_view_size_set(base_enventor_get(), val, h);
     }
   else
     {
        config_view_size_set(w, val);
        enventor_object_live_view_size_set(base_enventor_get(), w, val);
     }
}

static Evas_Object *
view_resize_slider_layout_create(Evas_Object *parent, const char *type,
                                 int slider_val)
{
   //Layout
   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "slider_layout");
   evas_object_show(layout);

   //Type
   elm_object_part_text_set(layout, "elm.text.type", type);

   //Slider
   Evas_Object *slider = elm_slider_add(layout);
   elm_slider_span_size_set(slider, 120);
   elm_slider_indicator_show_set(slider, EINA_FALSE);
   double step = 1 / (double) (3840 - 1);
   elm_slider_step_set(slider, step);
   elm_slider_min_max_set(slider, 1, 3840);
   elm_slider_value_set(slider, slider_val);
   evas_object_smart_callback_add(slider, "changed",
                                  view_resize_slider_changed_cb, layout);
   elm_object_part_text_set(layout, "elm.text.slider_min", "1");
   elm_object_part_text_set(layout, "elm.text.slider_max", "3840");
   elm_object_part_content_set(layout, "elm.swallow.slider", slider);

   return layout;
}

static void
view_resize_btn_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   stats_data *sd = data;
   evas_object_del(sd->ctxpopup);

   //Ctxpopup
   Evas_Object *ctxpopup = elm_ctxpopup_add(base_layout_get());
   if (!ctxpopup) return;

   elm_object_style_set(ctxpopup, elm_app_name_get());
   elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_UP,
                                       ELM_CTXPOPUP_DIRECTION_RIGHT,
                                       ELM_CTXPOPUP_DIRECTION_LEFT,
                                       ELM_CTXPOPUP_DIRECTION_DOWN);
   //Slider Layout
   Evas_Object *box = elm_box_add(ctxpopup);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(ctxpopup, box);

   Evas_Object *slider;
   Evas_Coord w, h;
   config_view_size_get(&w, &h);

   //Slider 1
   slider = view_resize_slider_layout_create(box, VIEW_RESIZE_TYPE_W, w);
   elm_box_pack_end(box, slider);

   //Slider 2
   slider = view_resize_slider_layout_create(box, VIEW_RESIZE_TYPE_H, h);
   elm_object_signal_emit(slider, "odd,item,set", "");
   elm_box_pack_end(box, slider);

   //Ctxpopup Position
   Evas_Coord x, y;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_move(ctxpopup, x + (w/2), y);
   evas_object_show(ctxpopup);

   sd->ctxpopup = ctxpopup;
}

static void
view_scale_btn_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   stats_data *sd = data;
   evas_object_del(sd->ctxpopup);

   //Ctxpopup
   Evas_Object *ctxpopup = elm_ctxpopup_add(base_layout_get());
   elm_object_style_set(ctxpopup, elm_app_name_get());

   //Slider
   Evas_Object *slider = elm_slider_add(ctxpopup);
   elm_slider_span_size_set(slider, 150);
   elm_slider_indicator_show_set(slider, EINA_FALSE);
   double step = 0.05 / (double) (MAX_VIEW_SCALE - MIN_VIEW_SCALE);
   elm_slider_step_set(slider, step);
   elm_slider_horizontal_set(slider, EINA_FALSE);
   elm_slider_inverted_set(slider, EINA_TRUE);
   elm_slider_min_max_set(slider, MIN_VIEW_SCALE, MAX_VIEW_SCALE);
   elm_slider_value_set(slider, (double) config_view_scale_get());
   evas_object_smart_callback_add(slider, "changed",
                                  view_scale_slider_changed_cb, sd);

   evas_object_smart_callback_add(ctxpopup, "dismissed", ctxpopup_dismissed_cb,
                                  sd);
   elm_object_content_set(ctxpopup, slider);

   //Ctxpopup Position
   Evas_Coord x, y, w, h;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);

   sd->ctxpopup = ctxpopup;
}

static Evas_Object *
create_statusbar_btn(Evas_Object *layout, const char *image,
                     const char *part_name, const char *tooltip_msg,
                     Evas_Smart_Cb func, void *data)
{
   Evas_Object *box = elm_box_add(layout);

   Evas_Object *btn = elm_button_add(box);
   elm_object_style_set(btn, elm_app_name_get());
   elm_object_focus_allow_set(btn, EINA_FALSE);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_scale_set(btn, 0.8);
   evas_object_smart_callback_add(btn, "clicked", func, data);
   evas_object_show(btn);

   Evas_Object *img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, image);
   elm_object_content_set(btn, img);

   elm_object_tooltip_text_set(box, tooltip_msg);
   elm_object_tooltip_orient_set(box, ELM_TOOLTIP_ORIENT_TOP_RIGHT);

   elm_box_pack_end(box, btn);
   elm_object_part_content_set(layout, part_name, box);

   return btn;
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
stats_line_num_update(int cur_line, int max_line)
{
   stats_data *sd = g_sd;

   char buf[20];
   snprintf(buf, sizeof(buf), "%d", cur_line);
   elm_object_part_text_set(sd->layout, "elm.text.line_cur", buf);
   snprintf(buf, sizeof(buf), "%d", max_line);
   elm_object_part_text_set(sd->layout, "elm.text.line_max", buf);

   sd->cur_line = cur_line;
   sd->max_line = max_line;
}

void
stats_edc_group_update(Eina_Stringshare *group_name)
{
   stats_data *sd = g_sd;
   elm_object_part_text_set(sd->layout, "elm.text.group_name", group_name);
   sd->group_name = eina_stringshare_add(group_name);
}

Evas_Object *
stats_init(Evas_Object *parent)
{
   stats_data *sd = calloc(1, sizeof(stats_data));
   if (!sd)
     {
        EINA_LOG_ERR(_("Failed to allocate Memory!"));
        return NULL;
     }
   g_sd = sd;

   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "statusbar_layout");

   //View Scale button
   create_statusbar_btn(layout, "expand", "scale_btn",
                        "View Scale (Ctrl + Mouse Wheel)",
                        view_scale_btn_cb, sd);
   //View Resize Button
   create_statusbar_btn(layout, "expand", "resize_btn",
                        "Resize View Size",
                        view_resize_btn_cb, sd);
   //View Invert Button
   create_statusbar_btn(layout, "invert", "invert_btn",
                        "Invert View Size",
                        view_invert_btn_cb, sd);

   sd->layout = layout;

   stats_cursor_pos_update(0, 0, 0, 0);
   stats_edc_group_update(NULL);

   return layout;
}

Eina_Stringshare *stats_group_name_get(void)
{
   stats_data *sd = g_sd;
   return sd->group_name;
}

void
stats_term(void)
{
   stats_data *sd = g_sd;
   eina_stringshare_del(sd->group_name);
   free(sd);
}

void
stats_info_msg_update(const char *msg)
{
   if (!config_stats_bar_get()) return;

   stats_data *sd = g_sd;
   elm_object_part_text_set(sd->layout, "elm.text.info_msg", msg);
   elm_object_signal_emit(sd->layout, "elm,action,info_msg,show", "");
}

void
stats_view_scale_update(double scale)
{
   stats_data *sd = g_sd;

   char buf[10];
   snprintf(buf, sizeof(buf), "%0.2fx", scale);
   elm_object_part_text_set(sd->layout, "elm.text.scale", buf);
}

void
stats_view_size_update(Evas_Coord w, Evas_Coord h)
{
   stats_data *sd = g_sd;

   char buf[10];
   snprintf(buf, sizeof(buf), "%d", w);
   elm_object_part_text_set(sd->layout, "elm.text.size_w", buf);
   snprintf(buf, sizeof(buf), "%d", h);
   elm_object_part_text_set(sd->layout, "elm.text.size_h", buf);
}

void
stats_cursor_pos_update(Evas_Coord x, Evas_Coord y, float rel_x, float rel_y)
{
   stats_data *sd = g_sd;

   char buf[10];
   snprintf(buf, sizeof(buf), "%d", x);
   elm_object_part_text_set(sd->layout, "elm.text.cursor_pxx", buf);
   snprintf(buf, sizeof(buf), "%d", y);
   elm_object_part_text_set(sd->layout, "elm.text.cursor_pxy", buf);

   snprintf(buf, sizeof(buf), "%0.2f", rel_x);
   elm_object_part_text_set(sd->layout, "elm.text.cursor_relx", buf);
   snprintf(buf, sizeof(buf), "%0.2f", rel_y);
   elm_object_part_text_set(sd->layout, "elm.text.cursor_rely", buf);
}

Eina_Bool
stats_ctxpopup_dismiss(void)
{
   stats_data *sd = g_sd;
   if (sd->ctxpopup)
     {
        elm_ctxpopup_dismiss(sd->ctxpopup);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}
