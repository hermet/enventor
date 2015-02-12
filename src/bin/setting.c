#include "common.h"
#include "text_setting.h"

struct setting_s
{
   Evas_Object *setting_layout;

   Evas_Object *general_layout;

   Evas_Object *img_path_entry;
   Evas_Object *snd_path_entry;
   Evas_Object *fnt_path_entry;
   Evas_Object *dat_path_entry;

   Evas_Object *slider_view;
   Evas_Object *view_size_w_entry;
   Evas_Object *view_size_h_entry;
   Evas_Object *toggle_view_size;
   Evas_Object *toggle_highlight;
   Evas_Object *toggle_swallow;
   Evas_Object *toggle_stats;
   Evas_Object *toggle_tools;
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

   text_setting_term();

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
   config_view_scale_set(elm_slider_value_get(sd->slider_view));
   config_tools_set(elm_check_state_get(sd->toggle_tools));
   config_stats_bar_set(elm_check_state_get(sd->toggle_stats));
   config_part_highlight_set(elm_check_state_get(sd->toggle_highlight));
   config_dummy_swallow_set(elm_check_state_get(sd->toggle_swallow));
   config_view_size_configurable_set(elm_check_state_get(sd->toggle_view_size));
   text_setting_config_set();

   Evas_Coord w = (Evas_Coord)atoi(elm_entry_entry_get(sd->view_size_w_entry));
   Evas_Coord h = (Evas_Coord)atoi(elm_entry_entry_get(sd->view_size_h_entry));
   config_view_size_set(w, h);

   text_setting_syntax_color_save();

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

   elm_slider_value_set(sd->slider_view, (double) config_view_scale_get());

   elm_check_state_set(sd->toggle_tools, config_tools_get());
   elm_check_state_set(sd->toggle_stats, config_stats_bar_get());
   elm_check_state_set(sd->toggle_highlight, config_part_highlight_get());
   elm_check_state_set(sd->toggle_swallow, config_dummy_swallow_get());

   text_setting_font_scale_set((double) config_font_scale_get());
   text_setting_linenumber_set(config_linenumber_get());
   text_setting_auto_indent_set(config_auto_indent_get());
   text_setting_auto_complete_set(config_auto_complete_get());

   text_setting_syntax_color_reset();
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
label_create(Evas_Object *parent, const char *text)
{
   Evas_Object *label = elm_label_add(parent);
   elm_object_text_set(label, text);
   evas_object_show(label);

   return label;
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

static void
toggle_view_size_changed_cb(void *data, Evas_Object *obj,
                            void *event_info EINA_UNUSED)
{
   setting_data *sd = data;
   Eina_Bool toggle_on = elm_check_state_get(obj);

   elm_object_disabled_set(sd->view_size_w_entry, !toggle_on);
   elm_object_disabled_set(sd->view_size_h_entry, !toggle_on);
}

static Evas_Object *
general_layout_create(setting_data *sd, Evas_Object *parent)
{
   static Elm_Entry_Filter_Accept_Set digits_filter_data;
   static Elm_Entry_Filter_Limit_Size limit_filter_data;

   //Layout
   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "general_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);

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
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   elm_object_content_set(scroller, box);

   //View Scale (Slider)
   Evas_Object *slider_view = elm_slider_add(box);
   evas_object_size_hint_weight_set(slider_view, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(slider_view, 0, 0);
   elm_slider_span_size_set(slider_view, 190);
   elm_slider_indicator_show_set(slider_view, EINA_FALSE);
   elm_slider_unit_format_set(slider_view, "%1.2fx");
   elm_slider_min_max_set(slider_view, MIN_VIEW_SCALE, MAX_VIEW_SCALE);
   elm_slider_value_set(slider_view, (double) config_view_scale_get());
   elm_object_text_set(slider_view, "View Scale");
   evas_object_show(slider_view);

   elm_box_pack_end(box, slider_view);

   //View Size
   Evas_Object *box2 = elm_box_add(box);
   elm_box_horizontal_set(box2, EINA_TRUE);
   evas_object_size_hint_weight_set(box2, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(box2, EVAS_HINT_FILL, 0);
   evas_object_show(box2);

   elm_box_pack_end(box, box2);

   //Box for View Size Entries
   Evas_Object *box3 = elm_box_add(box2);
   elm_box_horizontal_set(box3, EINA_TRUE);
   evas_object_size_hint_weight_set(box3, 0.25, 0);
   evas_object_size_hint_align_set(box3, EVAS_HINT_FILL, 0);
   evas_object_show(box3);

   elm_box_pack_end(box2, box3);

   //Label (View Size)
   Evas_Object *label_view_size = label_create(box2, "View Size");
   elm_box_pack_end(box3, label_view_size);

   Evas_Coord w, h;
   char w_str[5], h_str[5];
   config_view_size_get(&w, &h);
   snprintf(w_str, sizeof(w_str), "%d", w);
   snprintf(h_str, sizeof(h_str), "%d", h);

   //Entry (View Width)
   Evas_Object *entry_view_size_w = entry_create(box3);
   evas_object_size_hint_weight_set(entry_view_size_w, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(entry_view_size_w, EVAS_HINT_FILL, 0);

   digits_filter_data.accepted = "0123456789";
   digits_filter_data.rejected = NULL;
   elm_entry_markup_filter_append(entry_view_size_w,
                                  elm_entry_filter_accept_set,
                                  &digits_filter_data);
   limit_filter_data.max_char_count = 4;
   limit_filter_data.max_byte_count = 0;
   elm_entry_markup_filter_append(entry_view_size_w,
                                  elm_entry_filter_limit_size,
                                  &limit_filter_data);

   elm_object_text_set(entry_view_size_w, w_str);
   elm_object_disabled_set(entry_view_size_w,
                           !config_view_size_configurable_get());
   elm_box_pack_end(box3, entry_view_size_w);

   //Label (X)
   Evas_Object *label_view_size_x = label_create(box3, "X");
   elm_box_pack_end(box3, label_view_size_x);

   //Entry (View Height)
   Evas_Object *entry_view_size_h = entry_create(box3);
   evas_object_size_hint_weight_set(entry_view_size_h, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(entry_view_size_h, EVAS_HINT_FILL, 0);

   elm_entry_markup_filter_append(entry_view_size_h,
                                  elm_entry_filter_accept_set,
                                  &digits_filter_data);
   elm_entry_markup_filter_append(entry_view_size_h,
                                  elm_entry_filter_limit_size,
                                  &limit_filter_data);

   elm_object_text_set(entry_view_size_h, h_str);
   elm_object_disabled_set(entry_view_size_h,
                           !config_view_size_configurable_get());
   elm_box_pack_end(box3, entry_view_size_h);

   //Toggle (View Size)
   Evas_Object *toggle_view_size;
   toggle_view_size = toggle_create(box2, NULL,
                                    config_view_size_configurable_get());
   evas_object_smart_callback_add(toggle_view_size, "changed",
                                  toggle_view_size_changed_cb, sd);
   elm_box_pack_end(box2, toggle_view_size);

   //Toggle (Part Highlighting)
   Evas_Object *toggle_highlight = toggle_create(box, "Part Highlighting",
                                                 config_part_highlight_get());
   elm_box_pack_end(box, toggle_highlight);

   //Toggle (Dummy Swallow)
   Evas_Object *toggle_swallow = toggle_create(box, "Dummy Swallow",
                                               config_dummy_swallow_get());
   elm_box_pack_end(box, toggle_swallow);

   //Toggle (Status)
   Evas_Object *toggle_stats = toggle_create(box, "Status",
                                             config_stats_bar_get());
   elm_box_pack_end(box, toggle_stats);

   //Toggle (Tools)
   Evas_Object *toggle_tools = toggle_create(box, "Tools",
                                             config_tools_get());
   elm_box_pack_end(box, toggle_tools);

   sd->general_layout = layout;
   sd->img_path_entry = img_path_entry;
   sd->snd_path_entry = snd_path_entry;
   sd->fnt_path_entry = fnt_path_entry;
   sd->dat_path_entry = dat_path_entry;
   sd->slider_view = slider_view;
   sd->view_size_w_entry = entry_view_size_w;
   sd->view_size_h_entry = entry_view_size_h;
   sd->toggle_view_size = toggle_view_size;
   sd->toggle_highlight = toggle_highlight;
   sd->toggle_swallow = toggle_swallow;
   sd->toggle_stats = toggle_stats;
   sd->toggle_tools = toggle_tools;

   return layout;
}

static void
general_tab_cb(void *data, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   setting_data *sd = data;
   Evas_Object *content;

   if (!sd->setting_layout) return;

   content = elm_object_part_content_get(sd->setting_layout,
                                         "elm.swallow.content");

   if (content == sd->general_layout) return;

   elm_object_part_content_unset(sd->setting_layout, "elm.swallow.content");
   evas_object_hide(content);

   elm_object_part_content_set(sd->setting_layout, "elm.swallow.content",
                               sd->general_layout);
   elm_object_focus_set(sd->img_path_entry, EINA_TRUE);
}

static void
text_setting_tab_cb(void *data, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   setting_data *sd = data;
   text_setting_layout_show(sd->setting_layout);
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

   text_setting_init();

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

   //Tabbar
   Evas_Object *tabbar = elm_toolbar_add(layout);
   elm_toolbar_shrink_mode_set(tabbar, ELM_TOOLBAR_SHRINK_EXPAND);
   elm_toolbar_select_mode_set(tabbar, ELM_OBJECT_SELECT_MODE_ALWAYS);
   evas_object_size_hint_weight_set(tabbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   elm_toolbar_item_append(tabbar, NULL, "General", general_tab_cb, sd);
   elm_toolbar_item_append(tabbar, NULL, "Text Editor", text_setting_tab_cb, sd);

   elm_object_part_content_set(layout, "elm.swallow.tabbar", tabbar);

   //General layout
   Evas_Object *general_layout = general_layout_create(sd, layout);
   elm_object_part_content_set(layout, "elm.swallow.content", general_layout);

   //Text setting layout
   Evas_Object *text_setting_layout;
   text_setting_layout = text_setting_layout_create(layout);
   evas_object_hide(text_setting_layout);

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
