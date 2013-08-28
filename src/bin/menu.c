#include <Elementary.h>
#include "common.h"

struct menu_s
{
   Evas_Object *win;
   Evas_Object *menu_layout;
   Evas_Object *setting_layout;
   Evas_Object *warning_layout;
   Evas_Object *fileselector_layout;
   Evas_Object *help_layout;
   Evas_Object *img_path_entry;
   Evas_Object *snd_path_entry;
   Evas_Object *fnt_path_entry;
   Evas_Object *data_path_entry;
   Evas_Object *slider_font;
   Evas_Object *toggle_stats;
   Evas_Object *toggle_linenumber;
   Evas_Object *toggle_highlight;
   Evas_Object *toggle_swallow;
   Evas_Object *toggle_indent;

   Evas_Object *ctxpopup;

   void (*close_cb)(void *data);
   void *close_cb_data;

   config_data *cd;
   edit_data *ed;
   view_data *vd;

   Eina_Bool menu_open : 1;
};

static menu_data *g_md;

static void warning_no_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                              void *event_info EINA_UNUSED);
static void new_save_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED);
static void edc_reload(menu_data *md, const char *edc_path);
static void edc_file_save(menu_data *md);

static void
fileselector_close(menu_data *md)
{
   elm_object_signal_emit(md->fileselector_layout, "elm,state,dismiss", "");
}

static void
help_close(menu_data *md)
{
   elm_object_signal_emit(md->help_layout, "elm,state,dismiss", "");
}

static void
setting_close(menu_data *md)
{
   elm_object_signal_emit(md->setting_layout, "elm,state,dismiss", "");
}

static void
warning_close(menu_data *md)
{
   elm_object_signal_emit(md->warning_layout, "elm,state,dismiss", "");
}

static void
menu_close(menu_data *md, Eina_Bool toggled)
{
   if (md->menu_layout)
     elm_object_signal_emit(md->menu_layout, "elm,state,dismiss", "");

   if (!toggled)
     md->close_cb(md->close_cb_data);
}

static void
fileselector_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                          const char *emission EINA_UNUSED,
                          const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->fileselector_layout);
   md->fileselector_layout = NULL;
   if (md->menu_layout)
     {
        elm_object_disabled_set(md->menu_layout, EINA_FALSE);
        elm_object_focus_set(md->menu_layout, EINA_TRUE);
     }
}

static void
setting_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                     const char *emission EINA_UNUSED,
                     const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->setting_layout);
   md->setting_layout = NULL;
   if (!md->menu_layout) return;
   elm_object_disabled_set(md->menu_layout, EINA_FALSE);
   elm_object_focus_set(md->menu_layout, EINA_TRUE);
}

static void
help_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->help_layout);
   md->help_layout = NULL;
   if (!md->menu_layout) return;
   elm_object_disabled_set(md->menu_layout, EINA_FALSE);
   elm_object_focus_set(md->menu_layout, EINA_TRUE);
}

static void
menu_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->menu_layout);
   md->menu_layout = NULL;
   md->menu_open = EINA_FALSE;
}

static void
warning_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                     const char *emission EINA_UNUSED,
                     const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->warning_layout);
   md->warning_layout = NULL;
   if (!md->menu_layout) return;
   elm_object_disabled_set(md->menu_layout, EINA_FALSE);
   elm_object_focus_set(md->menu_layout, EINA_TRUE);
}

static void
warning_layout_create(menu_data *md, Evas_Smart_Cb yes_cb,
                      Evas_Smart_Cb save_cb)
{
   if (md->warning_layout) return;

   //Layout
   Evas_Object *layout = elm_layout_add(md->win);
   elm_layout_file_set(layout, EDJE_PATH, "warning_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  warning_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(md->win, layout);
   evas_object_show(layout);

   Evas_Object *btn;

   //Save Button
   btn = elm_button_add(layout);
   elm_object_style_set(btn, elm_app_name_get());
   elm_object_text_set(btn, "Save");
   evas_object_smart_callback_add(btn, "clicked", save_cb, md);
   evas_object_show(btn);
   elm_object_focus_set(btn, EINA_TRUE);

   elm_object_part_content_set(layout, "elm.swallow.save", btn);

   //No Button
   btn = elm_button_add(layout);
   elm_object_style_set(btn, elm_app_name_get());
   elm_object_text_set(btn, "No");
   evas_object_smart_callback_add(btn, "clicked", warning_no_btn_cb, md);
   evas_object_show(btn);

   elm_object_part_content_set(layout, "elm.swallow.no", btn);

   //Yes Button
   btn = elm_button_add(layout);
   elm_object_style_set(btn, elm_app_name_get());
   elm_object_text_set(btn, "Yes");
   evas_object_smart_callback_add(btn, "clicked", yes_cb, md);
   evas_object_show(btn);

   elm_object_part_content_set(layout, "elm.swallow.yes", btn);

   if (md->menu_layout)
     elm_object_disabled_set(md->menu_layout, EINA_TRUE);

   md->warning_layout = layout;
}

static void
setting_apply_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   config_data *cd = md->cd;

   config_edc_img_path_set(cd, elm_object_text_get(md->img_path_entry));
   config_edc_snd_path_set(cd, elm_object_text_get(md->snd_path_entry));
   config_edc_fnt_path_set(cd, elm_object_text_get(md->fnt_path_entry));
   config_edc_data_path_set(cd, elm_object_text_get(md->data_path_entry));
   config_font_size_set(cd, (float) elm_slider_value_get(md->slider_font));
   config_stats_bar_set(cd, elm_check_state_get(md->toggle_stats));
   config_linenumber_set(cd, elm_check_state_get(md->toggle_linenumber));
   config_part_highlight_set(cd, elm_check_state_get(md->toggle_highlight));
   config_dummy_swallow_set(cd, elm_check_state_get(md->toggle_swallow));
   config_auto_indent_set(cd, elm_check_state_get(md->toggle_indent));

   config_apply(cd);

   setting_close(md);
}

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
data_path_entry_update(Evas_Object *entry, Eina_List *edc_data_paths)
{
   elm_entry_entry_set(entry, NULL);

   Eina_List *l;
   char *edc_data_path;
   EINA_LIST_FOREACH(edc_data_paths, l, edc_data_path)
     {
        elm_entry_entry_append(entry, edc_data_path);
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
setting_cancel_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   setting_close(md);
}

static void
setting_reset_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   config_data *cd = md->cd;

   img_path_entry_update(md->img_path_entry,
                         (Eina_List *)config_edc_img_path_list_get(md->cd));
   snd_path_entry_update(md->snd_path_entry,
                         (Eina_List *)config_edc_snd_path_list_get(md->cd));
   fnt_path_entry_update(md->fnt_path_entry,
                         (Eina_List *)config_edc_fnt_path_list_get(md->cd));
   data_path_entry_update(md->data_path_entry,
                         (Eina_List *)config_edc_data_path_list_get(md->cd));

   elm_slider_value_set(md->slider_font, (double) config_font_size_get(cd));

   elm_check_state_set(md->toggle_stats, config_stats_bar_get(cd));
   elm_check_state_set(md->toggle_linenumber, config_linenumber_get(cd));
   elm_check_state_set(md->toggle_highlight, config_part_highlight_get(cd));
   elm_check_state_set(md->toggle_swallow, config_dummy_swallow_get(cd));
   elm_check_state_set(md->toggle_indent, config_auto_indent_get(cd));
}

static void
setting_open(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(md->win);
   elm_layout_file_set(layout, EDJE_PATH, "setting_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  setting_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(md->win, layout);
   evas_object_show(layout);

   //Image Path Entry
   Evas_Object *img_path_entry = elm_entry_add(layout);
   elm_object_style_set(img_path_entry, elm_app_name_get());
   elm_entry_single_line_set(img_path_entry, EINA_TRUE);
   elm_entry_scrollable_set(img_path_entry, EINA_TRUE);
   img_path_entry_update(img_path_entry,
                         (Eina_List *)config_edc_img_path_list_get(md->cd));
   evas_object_show(img_path_entry);
   elm_object_focus_set(img_path_entry, EINA_TRUE);

   elm_object_part_content_set(layout, "elm.swallow.img_path_entry",
                               img_path_entry);

   //Sound Path Entry
   Evas_Object *snd_path_entry = elm_entry_add(layout);
   elm_object_style_set(snd_path_entry, elm_app_name_get());
   elm_entry_single_line_set(snd_path_entry, EINA_TRUE);
   elm_entry_scrollable_set(snd_path_entry, EINA_TRUE);
   snd_path_entry_update(snd_path_entry,
                         (Eina_List *)config_edc_snd_path_list_get(md->cd));
   evas_object_show(snd_path_entry);

   elm_object_part_content_set(layout, "elm.swallow.snd_path_entry",
                               snd_path_entry);
   //Font Path Entry
   Evas_Object *fnt_path_entry = elm_entry_add(layout);
   elm_object_style_set(fnt_path_entry, elm_app_name_get());
   elm_entry_single_line_set(fnt_path_entry, EINA_TRUE);
   elm_entry_scrollable_set(fnt_path_entry, EINA_TRUE);
   fnt_path_entry_update(fnt_path_entry,
                         (Eina_List *)config_edc_fnt_path_list_get(md->cd));
   evas_object_show(fnt_path_entry);

   elm_object_part_content_set(layout, "elm.swallow.fnt_path_entry",
                               fnt_path_entry);

   //Data Path Entry
   Evas_Object *data_path_entry = elm_entry_add(layout);
   elm_object_style_set(data_path_entry, elm_app_name_get());
   elm_entry_single_line_set(data_path_entry, EINA_TRUE);
   elm_entry_scrollable_set(data_path_entry, EINA_TRUE);
   data_path_entry_update(data_path_entry,
                         (Eina_List *)config_edc_data_path_list_get(md->cd));
   evas_object_show(data_path_entry);

   elm_object_part_content_set(layout, "elm.swallow.data_path_entry",
                               data_path_entry);

   //Preference
   Evas_Object *scroller = elm_scroller_add(layout);
   elm_object_part_content_set(layout, "elm.swallow.preference", scroller);
   evas_object_show(scroller);

   Evas_Object *box = elm_box_add(scroller);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   elm_object_content_set(scroller, box);

   //Font Size
   Evas_Object *box2 = elm_box_add(box);
   elm_box_horizontal_set(box2, EINA_TRUE);
   evas_object_size_hint_weight_set(box2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box2);

   elm_box_pack_end(box, box2);

   //Font Size (Label)
   Evas_Object *label = elm_label_add(box2);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, 0, EVAS_HINT_FILL);

   elm_object_text_set(label, "Font Size");
   evas_object_show(label);

   elm_box_pack_end(box2, label);

   //Font Size (Slider)
   Evas_Object *slider = elm_slider_add(box2);
   elm_object_scale_set(slider, 1.2125);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(slider, 1, EVAS_HINT_FILL);
   elm_slider_span_size_set(slider, 300);
   elm_slider_indicator_show_set(slider, EINA_FALSE);
   elm_slider_unit_format_set(slider, "%1.1fx");
   elm_slider_min_max_set(slider, MIN_FONT_SIZE, MAX_FONT_SIZE);
   elm_slider_value_set(slider, (double) config_font_size_get(md->cd));
   evas_object_show(slider);

   elm_box_pack_end(box2, slider);

   Evas_Object *toggle;

   //Toggle (Tab bar)
   toggle = elm_check_add(box);
   elm_object_style_set(toggle, "toggle");
   elm_object_disabled_set(toggle, EINA_TRUE);
   evas_object_size_hint_weight_set(toggle, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(toggle, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(toggle, "Tab Bar");
   evas_object_show(toggle);

   elm_box_pack_end(box, toggle);

   //Toggle (Tools)
   toggle = elm_check_add(box);
   elm_object_style_set(toggle, "toggle");
   elm_object_disabled_set(toggle, EINA_TRUE);
   evas_object_size_hint_weight_set(toggle, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(toggle, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(toggle, "Tool Bar");
   evas_object_show(toggle);

   elm_box_pack_end(box, toggle);

   //Toggle (Status bar)
   Evas_Object *toggle_stats = elm_check_add(box);
   elm_object_style_set(toggle_stats, "toggle");
   elm_check_state_set(toggle_stats, config_stats_bar_get(md->cd));
   evas_object_size_hint_weight_set(toggle_stats, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(toggle_stats, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   elm_object_text_set(toggle_stats, "Status Bar");
   evas_object_show(toggle_stats);

   elm_box_pack_end(box, toggle_stats);

   //Toggle (Line Number)
   Evas_Object *toggle_linenumber = elm_check_add(box);
   elm_object_style_set(toggle_linenumber, "toggle");
   elm_check_state_set(toggle_linenumber, config_linenumber_get(md->cd));
   evas_object_size_hint_weight_set(toggle_linenumber, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(toggle_linenumber, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   elm_object_text_set(toggle_linenumber, "Line Number");
   evas_object_show(toggle_linenumber);

   elm_box_pack_end(box, toggle_linenumber);

   //Toggle (Part Highlighting)
   Evas_Object *toggle_highlight = elm_check_add(box);
   elm_object_style_set(toggle_highlight, "toggle");
   elm_check_state_set(toggle_highlight, config_part_highlight_get(md->cd));
   evas_object_size_hint_weight_set(toggle_highlight, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(toggle_highlight, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   elm_object_text_set(toggle_highlight, "Part Highlighting");
   evas_object_show(toggle_highlight);

   elm_box_pack_end(box, toggle_highlight);

   //Toggle (Dummy Swallow)
   Evas_Object *toggle_swallow = elm_check_add(box);
   elm_object_style_set(toggle_swallow, "toggle");
   elm_check_state_set(toggle_swallow, config_dummy_swallow_get(md->cd));
   evas_object_size_hint_weight_set(toggle_swallow, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(toggle_swallow, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   elm_object_text_set(toggle_swallow, "Dummy Swallow");
   evas_object_show(toggle_swallow);

   elm_box_pack_end(box, toggle_swallow);

   //Toggle (Auto Indentation)
   Evas_Object *toggle_indent = elm_check_add(box);
   elm_object_style_set(toggle_indent, "toggle");
   elm_check_state_set(toggle_indent, config_auto_indent_get(md->cd));
   evas_object_size_hint_weight_set(toggle_indent, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(toggle_indent, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   elm_object_text_set(toggle_indent, "Auto Indentation");
   evas_object_show(toggle_indent);

   elm_box_pack_end(box, toggle_indent);

   //Toggle (Separate Window)
   toggle = elm_check_add(box);
   elm_object_style_set(toggle, "toggle");
   elm_object_disabled_set(toggle, EINA_TRUE);
   evas_object_size_hint_weight_set(toggle, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(toggle, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(toggle, "Separate Window");
   evas_object_show(toggle);

   elm_box_pack_end(box, toggle);

   Evas_Object *btn;

   //Apply Button
   btn = elm_button_add(layout);
   elm_object_style_set(btn, elm_app_name_get());
   elm_object_text_set(btn, "Apply");
   evas_object_smart_callback_add(btn, "clicked", setting_apply_btn_cb, md);
   evas_object_show(btn);

   elm_object_part_content_set(layout, "elm.swallow.apply_btn", btn);

   //Reset Button
   btn = elm_button_add(layout);
   elm_object_style_set(btn, elm_app_name_get());
   elm_object_text_set(btn, "Reset");
   evas_object_smart_callback_add(btn, "clicked", setting_reset_btn_cb, md);
   evas_object_show(btn);

   elm_object_part_content_set(layout, "elm.swallow.reset_btn", btn);

   //Cancel Button
   btn = elm_button_add(layout);
   elm_object_style_set(btn, elm_app_name_get());
   elm_object_text_set(btn, "Cancel");
   evas_object_smart_callback_add(btn, "clicked", setting_cancel_btn_cb, md);
   evas_object_show(btn);

   elm_object_part_content_set(layout, "elm.swallow.cancel_btn", btn);

   if (md->menu_layout)
     elm_object_disabled_set(md->menu_layout, EINA_TRUE);

   md->setting_layout = layout;
   md->img_path_entry = img_path_entry;
   md->snd_path_entry = snd_path_entry;
   md->fnt_path_entry = fnt_path_entry;
   md->data_path_entry = data_path_entry;
   md->slider_font = slider;
   md->toggle_stats = toggle_stats;
   md->toggle_linenumber = toggle_linenumber;
   md->toggle_highlight = toggle_highlight;
   md->toggle_swallow = toggle_swallow;
   md->toggle_indent = toggle_indent;
}

static void
help_open(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(md->win);
   elm_layout_file_set(layout, EDJE_PATH, "help_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  help_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(md->win, layout);
   evas_object_show(layout);

   //Entry
   Evas_Object *entry = elm_entry_add(layout);
   elm_object_style_set(entry, "help");
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_line_wrap_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_FALSE);
   evas_object_show(entry);

   elm_object_focus_set(entry, EINA_TRUE);

   elm_object_part_content_set(layout, "elm.swallow.entry", entry);

   elm_entry_entry_append(entry, "<color=#ffffff><font_size=12>");

   //Read README
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/docs/README", elm_app_data_dir_get());

   Eina_File *file = eina_file_open(buf, EINA_FALSE);
   if (!file) goto err;

   Eina_Iterator *itr = eina_file_map_lines(file);
   if (!itr) goto err;

   Eina_Strbuf *strbuf = eina_strbuf_new();
   if (!strbuf) goto err;

   Eina_File_Line *line;
   int line_num = 0;

   EINA_ITERATOR_FOREACH(itr, line)
     {
        //Append edc ccde
        if (line_num > 0)
          {
             if (!eina_strbuf_append(strbuf, "<br/>")) goto err;
          }

        if (!eina_strbuf_append_length(strbuf, line->start, line->length))
          goto err;
        line_num++;
     }
   elm_entry_entry_append(entry, eina_strbuf_string_get(strbuf));
   elm_entry_entry_append(entry, "</font_size></color>");

   if (md->menu_layout)
     elm_object_disabled_set(md->menu_layout, EINA_TRUE);

   md->help_layout = layout;

err:
   if (strbuf) eina_strbuf_free(strbuf);
   if (itr) eina_iterator_free(itr);
   if (file) eina_file_close(file);
}

static void
help_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   help_open(md);
}

static void
setting_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   setting_open(md);
}

static void
new_yes_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   menu_data *md = data;

   edc_reload(md, PROTO_EDC_PATH);
   warning_close(md);
   menu_close(md, EINA_FALSE);
}

static void
exit_yes_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   elm_exit();
}

static void
exit_save_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   edit_save(md->ed);
   elm_exit();
}

void
menu_exit(menu_data *md)
{
   if (edit_changed_get(md->ed))
     warning_layout_create(md, exit_yes_btn_cb, exit_save_btn_cb);
   else
     elm_exit();
}

static void
exit_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   menu_exit(md);
}

static Evas_Object *
btn_create(Evas_Object *parent, const char *label, Evas_Smart_Cb cb, void *data)
{
   Evas_Object *layout, *btn;

   layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "button_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);

   btn  = elm_button_add(layout);
   elm_object_style_set(btn, elm_app_name_get());
   evas_object_smart_callback_add(btn, "clicked", cb, data);
   elm_object_text_set(btn, label);
   evas_object_show(btn);

   elm_object_part_content_set(layout, "elm.swallow.btn", btn);

   return layout;
}

static Eina_Bool
btn_effect_timer_cb(void *data)
{
   Evas_Object *btn = data;
   elm_object_signal_emit(btn, "elm,action,btn,zoom", "");
   return ECORE_CALLBACK_CANCEL;
}

static void
edc_reload(menu_data *md, const char *edc_path)
{
   config_edc_path_set(md->cd, edc_path);
   edit_new(md->ed);
   view_reload_need_set(md->vd, EINA_TRUE);
   config_apply(md->cd);
}

static void
warning_no_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   warning_close(md);
}

static void
new_save_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   menu_data *md = data;

   edit_save(md->ed);
   edc_reload(md, PROTO_EDC_PATH);
   warning_close(md);
   menu_close(md, EINA_FALSE);
}

static void
new_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   menu_edc_new(md);
}

static void
save_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   edc_file_save(md);
}

static void
fileselector_save_done_cb(void *data, Evas_Object *obj EINA_UNUSED,
                          void *event_info)
{
   menu_data *md = data;
   const char *selected = event_info;

   if (!selected)
     {
        fileselector_close(md);
        return;
     }

   //Filter to read only edc extension file.
   char *ext = strrchr(selected, '.');
   if (!ext || strcmp(ext, ".edc"))
     {
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg",
                                 "Support only .edc file.");
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   //Directory?
   if (ecore_file_is_dir(selected))
     {
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg", "Choose a file to save.");
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   //Update the edc file and try to save.
   if (strcmp(config_edc_path_get(md->cd), selected))
     edit_changed_set(md->ed, EINA_TRUE);

   config_edc_path_set(md->cd, selected);

   if (!edit_save(md->ed))
     {
        char buf[PATH_MAX];
        snprintf(buf, sizeof(buf), "Failed to load: %s.", selected);
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg", buf);
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   view_reload_need_set(md->vd, EINA_TRUE);
   config_apply(md->cd);

   fileselector_close(md);
   menu_close(md, EINA_FALSE);
}

static void
fileselector_save_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   fileselector_save_done_cb(data, obj, event_info);
}

static void
fileselector_load_done_cb(void *data, Evas_Object *obj EINA_UNUSED,
                          void *event_info)
{
   menu_data *md = data;
   const char *selected = event_info;

   if (!selected)
     {
        fileselector_close(md);
        return;
     }

   //Filter to read only edc extension file.
   char *ext = strrchr(selected, '.');
   if (!ext || strcmp(ext, ".edc"))
     {
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg",
                                 "Support only .edc file.");
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   //Directory?
   if (ecore_file_is_dir(selected))
     {
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg", "Choose a file to load.");
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   //Show a message if it failed to load the file.
   if (!ecore_file_can_read(selected))
     {
        char buf[PATH_MAX];
        const char *filename = ecore_file_file_get(selected);
        snprintf(buf, sizeof(buf), "Failed to load: %s.", filename);
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg", buf);
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   edc_reload(md, selected);
   fileselector_close(md);
   menu_close(md, EINA_FALSE);
}

static void
fileselector_load_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   fileselector_load_done_cb(data, obj, event_info);
}

static void
edc_file_save(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(md->win);
   elm_layout_file_set(layout, EDJE_PATH, "fileselector_layout");
   elm_object_part_text_set(layout, "elm.text.title", "Save");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  fileselector_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(md->win, layout);
   evas_object_show(layout);

   Evas_Object *fs = elm_fileselector_add(layout);
   elm_object_part_text_set(fs, "ok", "Save");
   elm_object_part_text_set(fs, "cancel", "Close");
   elm_fileselector_path_set(fs, getenv("HOME"));
   elm_fileselector_expandable_set(fs, EINA_FALSE);
   elm_fileselector_is_save_set(fs, EINA_TRUE);
   evas_object_smart_callback_add(fs, "done", fileselector_save_done_cb, md);
   evas_object_smart_callback_add(fs, "selected", fileselector_save_selected_cb,
                                  md);
   evas_object_show(fs);

   elm_object_part_content_set(layout, "elm.swallow.fileselector", fs);

   if (md->menu_layout)
     elm_object_disabled_set(md->menu_layout, EINA_TRUE);
   elm_object_focus_set(fs, EINA_TRUE);

   md->fileselector_layout = layout;
}

static void
edc_file_load(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(md->win);
   elm_layout_file_set(layout, EDJE_PATH, "fileselector_layout");
   elm_object_part_text_set(layout, "elm.text.title", "Load");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  fileselector_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(md->win, layout);
   evas_object_show(layout);

   Evas_Object *fs = elm_fileselector_add(layout);
   elm_fileselector_path_set(fs, getenv("HOME"));
   elm_object_part_text_set(fs, "ok", "Load");
   elm_object_part_text_set(fs, "cancel", "Close");
   elm_fileselector_expandable_set(fs, EINA_FALSE);
   elm_fileselector_is_save_set(fs, EINA_TRUE);
   evas_object_smart_callback_add(fs, "done", fileselector_load_done_cb, md);
   evas_object_smart_callback_add(fs, "selected", fileselector_load_selected_cb,
                                  md);
   evas_object_show(fs);

   elm_object_part_content_set(layout, "elm.swallow.fileselector", fs);

   if (md->menu_layout)
     elm_object_disabled_set(md->menu_layout, EINA_TRUE);
   elm_object_focus_set(fs, EINA_TRUE);

   md->fileselector_layout = layout;
}

static void
load_yes_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   edc_file_load(md);
   warning_close(md);
}

static void
load_save_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   edit_save(md->ed);
   edc_file_load(md);
   warning_close(md);
}

Eina_Bool
menu_help(menu_data *md)
{
   help_open(md);
   return EINA_TRUE;
}

Eina_Bool
menu_setting(menu_data *md)
{
   setting_open(md);
   return EINA_TRUE;
}

Eina_Bool
menu_edc_new(menu_data *md)
{
   if (edit_changed_get(md->ed))
     {
        warning_layout_create(md, new_yes_btn_cb, new_save_btn_cb);
        return EINA_TRUE;
     }
   edc_reload(md, PROTO_EDC_PATH);
   menu_close(md, EINA_FALSE);

   return EINA_FALSE;
}

Eina_Bool
menu_edc_save(menu_data *md)
{
   edc_file_save(md);
   return EINA_TRUE;
}

Eina_Bool
menu_edc_load(menu_data *md)
{
   if (edit_changed_get(md->ed))
     warning_layout_create(md, load_yes_btn_cb, load_save_btn_cb);
   else
     edc_file_load(md);

   return EINA_TRUE;
}

static void
load_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   menu_edc_load(md);
}

static void
menu_open(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(md->win);
   elm_layout_file_set(layout, EDJE_PATH, "menu_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  menu_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(md->win, layout);
   evas_object_show(layout);

   //Button
   Evas_Object *btn;

   //Button(New)
   btn = btn_create(layout, "New", new_btn_cb, md);
   elm_object_focus_set(btn, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.new_btn", btn);
   ecore_timer_add(0, btn_effect_timer_cb, btn);

   //Button(Save)
   btn = btn_create(layout, "Save", save_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.save_btn", btn);
   ecore_timer_add(0.03, btn_effect_timer_cb, btn);

   //Button(Load)
   btn = btn_create(layout, "Load", load_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.load_btn", btn);
   ecore_timer_add(0.06, btn_effect_timer_cb, btn);

   //Button(Setting)
   btn = btn_create(layout, "Setting", setting_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.setting_btn", btn);
   ecore_timer_add(0.09, btn_effect_timer_cb, btn);

   //Button(Help)
   btn = btn_create(layout, "Help", help_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.help_btn", btn);
   ecore_timer_add(0.12, btn_effect_timer_cb, btn);

   //Button(Exit)
   btn = btn_create(layout, "Exit", exit_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.exit_btn", btn);
   ecore_timer_add(0.15, btn_effect_timer_cb, btn);

   md->menu_layout = layout;

   md->menu_open = EINA_TRUE;
}

menu_data *
menu_init(Evas_Object *win, edit_data *ed, config_data *cd, view_data *vd,
          void (*close_cb)(void *data), void *data)
{
   menu_data *md = calloc(1, sizeof(menu_data));
   md->win = win;
   md->ed = ed;
   md->cd = cd;
   md->vd = vd;
   md->close_cb = close_cb;
   md->close_cb_data = data;
   g_md = md;
   return md;
}

void
menu_term(menu_data *md)
{
   if (!md) return;
   free(md);
}

Eina_Bool
menu_toggle()
{
   menu_data *md = g_md;

   //Level 2 Menus
   if (md->menu_open)
     {
        if (md->setting_layout)
          {
             setting_close(md);
             return EINA_TRUE;
          }
        else if (md->warning_layout)
          {
             warning_close(md);
             return EINA_TRUE;
          }
        else if (md->fileselector_layout)
          {
             fileselector_close(md);
             return EINA_TRUE;
          }
        else if (md->help_layout)
          {
             help_close(md);
             return EINA_TRUE;
          }
     }
   //Short Cut Key Open Case
   else
     {
        if (md->help_layout)
          {
             help_close(md);
             return EINA_FALSE;
          }
        if (md->fileselector_layout)
          {
             fileselector_close(md);
             return EINA_FALSE;
          }
        if (md->setting_layout)
          {
             setting_close(md);
             return EINA_FALSE;
          }
     }

   //Ctxpopup
   if (md->ctxpopup)
     {
        elm_ctxpopup_dismiss(md->ctxpopup);
        return EINA_FALSE;
     }
   //Warning
   if (md->warning_layout)
     {
        warning_close(md);
        return EINA_FALSE;
     }
   //Main Menu 
   if (md->menu_open)
     {
        menu_close(md, EINA_TRUE);
        return EINA_FALSE;
     }
   else
     {
        menu_open(md);
        return EINA_TRUE;
     }
}

static void
ctxpopup_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   md->ctxpopup = NULL;
}

void
menu_ctxpopup_register(Evas_Object *ctxpopup)
{
   menu_data *md = g_md;
   md->ctxpopup = ctxpopup;
   if (!ctxpopup) return;
   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL, ctxpopup_del_cb,
                                  md);
}
