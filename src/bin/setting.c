#include <Elementary.h>
#include "common.h"

struct setting_s
{
   Evas_Object *setting_layout;
   Evas_Object *img_path_entry;
   Evas_Object *snd_path_entry;
   Evas_Object *fnt_path_entry;
   Evas_Object *dat_path_entry;

   Evas_Object *slider_font;
   Evas_Object *slider_view;
   Evas_Object *toggle_tools;
   Evas_Object *toggle_stats;
   Evas_Object *toggle_linenum;
   Evas_Object *toggle_highlight;
   Evas_Object *toggle_swallow;
   Evas_Object *toggle_indent;
   Evas_Object *toggle_autocomp;
};

typedef struct setting_s setting_data;

static setting_data *g_sd = NULL;

static void
img_path_entry_update(Evas_Object *entry, Eina_List *edc_img_paths)
{
   elm_entry_entry_set(entry, NULL);

   Eina_List *l;
   char *edc_img_path;
   EINA_LIST_FOREACH(edc_img_paths, l, edc_img_path)
     {
        elm_entry_entry_append(entry, edc_img_path);
        elm_entry_entry_append(entry, ";");
     }
}

static void
fnt_path_entry_update(Evas_Object *entry, Eina_List *edc_fnt_paths)
{
   elm_entry_entry_set(entry, NULL);

   Eina_List *l;
   char *edc_fnt_path;
   EINA_LIST_FOREACH(edc_fnt_paths, l, edc_fnt_path)
     {
        elm_entry_entry_append(entry, edc_fnt_path);
        elm_entry_entry_append(entry, ";");
     }
}

static void
dat_path_entry_update(Evas_Object *entry, Eina_List *edc_dat_paths)
{
   elm_entry_entry_set(entry, NULL);

   Eina_List *l;
   char *edc_dat_path;
   EINA_LIST_FOREACH(edc_dat_paths, l, edc_dat_path)
     {
        elm_entry_entry_append(entry, edc_dat_path);
        elm_entry_entry_append(entry, ";");
     }
}

static void
snd_path_entry_update(Evas_Object *entry, Eina_List *edc_snd_paths)
{
   elm_entry_entry_set(entry, NULL);

   Eina_List *l;
   char *edc_snd_path;
   EINA_LIST_FOREACH(edc_snd_paths, l, edc_snd_path)
     {
        elm_entry_entry_append(entry, edc_snd_path);
        elm_entry_entry_append(entry, ";");
     }
}

static void
setting_dismiss_done_cb(void *data, Evas_Object *obj EINA_UNUSED,
                        const char *emission EINA_UNUSED,
                        const char *source EINA_UNUSED)
{
   setting_data *sd = data;
   evas_object_del(sd->setting_layout);
   sd->setting_layout = NULL;
   menu_deactivate_request();

   free(sd);
   g_sd = NULL;
}

static void
setting_apply_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   setting_data *sd = data;

   config_edc_img_path_set(elm_object_text_get(sd->img_path_entry));
   config_edc_snd_path_set(elm_object_text_get(sd->snd_path_entry));
   config_edc_fnt_path_set(elm_object_text_get(sd->fnt_path_entry));
   config_edc_dat_path_set(elm_object_text_get(sd->dat_path_entry));
   config_font_scale_set((float) elm_slider_value_get(sd->slider_font));
   config_view_scale_set(elm_slider_value_get(sd->slider_view));
   config_tools_set(elm_check_state_get(sd->toggle_tools));
   config_stats_bar_set(elm_check_state_get(sd->toggle_stats));
   config_linenumber_set(elm_check_state_get(sd->toggle_linenum));
   config_part_highlight_set(elm_check_state_get(sd->toggle_highlight));
   config_dummy_swallow_set(elm_check_state_get(sd->toggle_swallow));
   config_auto_indent_set(elm_check_state_get(sd->toggle_indent));
   config_auto_complete_set(elm_check_state_get(sd->toggle_autocomp));

   config_apply();

   setting_close();
}

static void
setting_cancel_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   setting_close();
}

static void
setting_reset_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   setting_data *sd = data;

   img_path_entry_update(sd->img_path_entry,
                         (Eina_List *)config_edc_img_path_list_get());
   snd_path_entry_update(sd->snd_path_entry,
                         (Eina_List *)config_edc_snd_path_list_get());
   fnt_path_entry_update(sd->fnt_path_entry,
                         (Eina_List *)config_edc_fnt_path_list_get());
   dat_path_entry_update(sd->dat_path_entry,
                         (Eina_List *)config_edc_dat_path_list_get());

   elm_slider_value_set(sd->slider_font, (double) config_font_scale_get());
   elm_slider_value_set(sd->slider_view, (double) config_view_scale_get());

   elm_check_state_set(sd->toggle_tools, config_tools_get());
   elm_check_state_set(sd->toggle_stats, config_stats_bar_get());
   elm_check_state_set(sd->toggle_linenum, config_linenumber_get());
   elm_check_state_set(sd->toggle_highlight, config_part_highlight_get());
   elm_check_state_set(sd->toggle_swallow, config_dummy_swallow_get());
   elm_check_state_set(sd->toggle_indent, config_auto_indent_get());
   elm_check_state_set(sd->toggle_autocomp, config_auto_complete_get());
}

static Evas_Object *
entry_create(Evas_Object *parent)
{
   Evas_Object *entry = elm_entry_add(parent);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   evas_object_show(entry);

   return entry;
}

static Evas_Object *
toggle_create(Evas_Object *parent, const char *text, Eina_Bool state)
{
   Evas_Object *toggle = elm_check_add(parent);
   elm_object_style_set(toggle, "toggle");
   elm_check_state_set(toggle, state);
   evas_object_size_hint_weight_set(toggle, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(toggle, EVAS_HINT_FILL, 0);
   elm_object_text_set(toggle, text);
   evas_object_show(toggle);

   return toggle;
}

void
setting_open(void)
{
   setting_data *sd = g_sd;
   if (sd) return;

   sd = calloc(1, sizeof(setting_data));
   if (!sd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }
   g_sd = sd;

   search_close();
   goto_close();

   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "setting_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  setting_dismiss_done_cb, sd);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   //Image Path Entry
   Evas_Object *img_path_entry = entry_create(layout);
   img_path_entry_update(img_path_entry,
                         (Eina_List *)config_edc_img_path_list_get());
   elm_object_focus_set(img_path_entry, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.img_path_entry",
                               img_path_entry);

   //Sound Path Entry
   Evas_Object *snd_path_entry = entry_create(layout);
   snd_path_entry_update(snd_path_entry,
                         (Eina_List *)config_edc_snd_path_list_get());
   elm_object_part_content_set(layout, "elm.swallow.snd_path_entry",
                               snd_path_entry);
   //Font Path Entry
   Evas_Object *fnt_path_entry = entry_create(layout);
   fnt_path_entry_update(fnt_path_entry,
                         (Eina_List *)config_edc_fnt_path_list_get());
   elm_object_part_content_set(layout, "elm.swallow.fnt_path_entry",
                               fnt_path_entry);

   //Data Path Entry
   Evas_Object *dat_path_entry = entry_create(layout);
   dat_path_entry_update(dat_path_entry,
                         (Eina_List *)config_edc_dat_path_list_get());
   elm_object_part_content_set(layout, "elm.swallow.dat_path_entry",
                               dat_path_entry);

   //Preference
   Evas_Object *scroller = elm_scroller_add(layout);
   elm_object_part_content_set(layout, "elm.swallow.preference", scroller);

   //Box
   Evas_Object *box = elm_box_add(scroller);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   elm_object_content_set(scroller, box);

   //Left Box
   Evas_Object *box2 = elm_box_add(box);
   elm_box_padding_set(box2, 1, 1);
   evas_object_size_hint_weight_set(box2, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(box2, EVAS_HINT_FILL, 0);
   evas_object_show(box2);
   elm_box_pack_end(box, box2);

   Evas_Object *label, *box3;

   //Font Size
   box3 = elm_box_add(box2);
   elm_box_horizontal_set(box3, EINA_TRUE);
   evas_object_size_hint_weight_set(box3, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(box3, EVAS_HINT_FILL, 0);
   evas_object_show(box3);

   elm_box_pack_end(box2, box3);

   //Font Size (Slider)
   Evas_Object *slider_font = elm_slider_add(box3);
   evas_object_size_hint_weight_set(slider_font, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(slider_font, 0, 0);
   elm_slider_span_size_set(slider_font, 190);
   elm_slider_indicator_show_set(slider_font, EINA_FALSE);
   elm_slider_unit_format_set(slider_font, "%1.1fx");
   elm_slider_min_max_set(slider_font, MIN_FONT_SCALE, MAX_FONT_SCALE);
   elm_slider_value_set(slider_font, (double) config_font_scale_get());
   elm_object_text_set(slider_font, "Font Size");
   evas_object_show(slider_font);

   elm_box_pack_end(box3, slider_font);

   //Toggle (Tools)
   Evas_Object *toggle_tools = toggle_create(box2, "Tools",
                                             config_tools_get());
   elm_box_pack_end(box2, toggle_tools);

   //Toggle (Line Number)
   Evas_Object *toggle_linenum = toggle_create(box2, "Line Number",
                                               config_linenumber_get());
   elm_box_pack_end(box2, toggle_linenum);

   //Toggle (Dummy Swallow)
   Evas_Object *toggle_swallow = toggle_create(box2, "Dummy Swallow",
                                 config_dummy_swallow_get());
   elm_box_pack_end(box2, toggle_swallow);

   //Toggle (Auto Complete)
   Evas_Object *toggle_autocomp = toggle_create(box2, "Auto Completion",
                                  config_auto_complete_get());
   elm_box_pack_end(box2, toggle_autocomp);

   Evas_Object *separator = elm_separator_add(box);
   evas_object_show(separator);
   elm_box_pack_end(box, separator);

   //Right Box
   box2 = elm_box_add(box);
   elm_box_padding_set(box2, 1, 1);
   evas_object_size_hint_weight_set(box2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box2, EVAS_HINT_FILL, 0);
   evas_object_show(box2);

   elm_box_pack_end(box, box2);

   //View Scale
   box3 = elm_box_add(box2);
   elm_box_padding_set(box2, 1, 1);
   elm_box_horizontal_set(box3, EINA_TRUE);
   evas_object_size_hint_weight_set(box3, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(box3, EVAS_HINT_FILL, 0);
   evas_object_show(box3);

   elm_box_pack_end(box2, box3);

   //View Scale (Slider)
   Evas_Object *slider_view = elm_slider_add(box2);
   evas_object_size_hint_weight_set(slider_view, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(slider_view, 0, 0);
   elm_slider_span_size_set(slider_view, 190);
   elm_slider_indicator_show_set(slider_view, EINA_FALSE);
   elm_slider_unit_format_set(slider_view, "%1.2fx");
   elm_slider_min_max_set(slider_view, MIN_VIEW_SCALE, MAX_VIEW_SCALE);
   elm_slider_value_set(slider_view, (double) config_view_scale_get());
   elm_object_text_set(slider_view, "View Scale");
   evas_object_show(slider_view);

   elm_box_pack_end(box3, slider_view);

   //Toggle (Status)
   Evas_Object *toggle_stats = toggle_create(box2, "Status",
                                             config_stats_bar_get());
   elm_box_pack_end(box2, toggle_stats);


   //Toggle (Part Highlighting)
   Evas_Object *toggle_highlight = toggle_create(box2, "Part Highlighting",
                                   config_part_highlight_get());
   elm_box_pack_end(box2, toggle_highlight);


   //Toggle (Auto Indentation)
   Evas_Object *toggle_indent = toggle_create(box2, "Auto Indentation",
                                config_auto_indent_get());
   elm_box_pack_end(box2, toggle_indent);

   Evas_Object *btn;

   //Apply Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "Apply");
   evas_object_smart_callback_add(btn, "clicked", setting_apply_btn_cb, sd);
   elm_object_part_content_set(layout, "elm.swallow.apply_btn", btn);

   //Reset Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "Reset");
   evas_object_smart_callback_add(btn, "clicked", setting_reset_btn_cb, sd);
   elm_object_part_content_set(layout, "elm.swallow.reset_btn", btn);

   //Cancel Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "Cancel");
   evas_object_smart_callback_add(btn, "clicked", setting_cancel_btn_cb, sd);
   elm_object_part_content_set(layout, "elm.swallow.cancel_btn", btn);

   sd->setting_layout = layout;
   sd->setting_layout = layout;
   sd->img_path_entry = img_path_entry;
   sd->snd_path_entry = snd_path_entry;
   sd->fnt_path_entry = fnt_path_entry;
   sd->dat_path_entry = dat_path_entry;
   sd->slider_font = slider_font;
   sd->slider_view = slider_view;
   sd->toggle_tools = toggle_tools;
   sd->toggle_stats = toggle_stats;
   sd->toggle_linenum = toggle_linenum;
   sd->toggle_highlight = toggle_highlight;
   sd->toggle_swallow = toggle_swallow;
   sd->toggle_indent = toggle_indent;
   sd->toggle_autocomp = toggle_autocomp;

   menu_activate_request();
}

void
setting_close()
{
   setting_data *sd = g_sd;
   if (!sd) return;
   elm_object_signal_emit(sd->setting_layout, "elm,state,dismiss", "");
}

Eina_Bool
setting_is_opened(void)
{
   setting_data *sd = g_sd;
   if (!sd) return EINA_FALSE;
   return EINA_TRUE;
}
