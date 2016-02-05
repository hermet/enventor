#include "common.h"

typedef struct statusbar_s
{
   Evas_Object *layout;
   Evas_Object *ctxpopup;
   Eina_Stringshare *group_name;
   int cur_line;
   int max_line;
} stats_data;

stats_data *g_sd = NULL;

static void
slider_changed_cb(void *data, Evas_Object *obj,
                  void *event_info EINA_UNUSED)
{

   stats_data *sd = data;

   double scale = elm_slider_value_get(obj);
   double rounded = ROUNDING(scale, 1);

   if ((rounded - scale) > 0) rounded -= 0.05;

   /* Here logic is mostly duplicated with main_mouse_wheel_cb() in main.c */

   config_view_scale_set(rounded);
   scale = config_view_scale_get();
   enventor_object_live_view_scale_set(base_enventor_get(), scale);

   //Toggle on the configurable view size forcely.
   if (!config_view_size_configurable_get())
     {
        config_view_size_configurable_set(EINA_TRUE);
        Evas_Coord w, h;
        config_view_size_get(&w, &h);
        enventor_object_live_view_size_set(base_enventor_get(), w, h);
     }

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
view_invert_btn_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   stats_data *sd = data;

   //Toggle on the configurable view size forcely.
   if (!config_view_size_configurable_get())
     {
        config_view_size_configurable_set(EINA_TRUE);
     }

   Evas_Coord w, h;
   config_view_size_get(&w, &h);
   config_view_size_set(h, w);
   enventor_object_live_view_size_set(base_enventor_get(), h, w);

   //Just in live edit mode case.
   live_edit_update();
}

static void
view_scale_btn_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   stats_data *sd = data;

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
   evas_object_smart_callback_add(slider, "changed", slider_changed_cb,
                                  sd);

   evas_object_smart_callback_add(ctxpopup, "dismissed", ctxpopup_dismissed_cb,
                                  sd);
   elm_object_content_set(ctxpopup, slider);

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
   elm_object_focus_allow_set(btn, EINA_FALSE);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_scale_set(btn, 0.5);
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
