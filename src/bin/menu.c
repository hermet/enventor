#include <Elementary.h>
#include "common.h"

struct menu_s
{
   Evas_Object *menu_layout;
   Evas_Object *setting_layout;
   Evas_Object *warning_layout;
   Evas_Object *fileselector_layout;
   Evas_Object *about_layout;
   Evas_Object *img_path_entry;
   Evas_Object *snd_path_entry;
   Evas_Object *fnt_path_entry;
   Evas_Object *data_path_entry;
   Evas_Object *slider_font;
   Evas_Object *slider_view;
   Evas_Object *toggle_tools;
   Evas_Object *toggle_stats;
   Evas_Object *toggle_linenum;
   Evas_Object *toggle_highlight;
   Evas_Object *toggle_swallow;
   Evas_Object *toggle_indent;
   Evas_Object *ctxpopup;

   const char *last_accessed_path;

   int open_depth;

   edit_data *ed;
};

typedef struct menu_s menu_data;

static menu_data *g_md = NULL;

static void warning_no_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                              void *event_info EINA_UNUSED);
static void new_save_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED);
static void edc_file_save(menu_data *md);

static void
fileselector_close()
{
   menu_data *md = g_md;
   elm_object_signal_emit(md->fileselector_layout, "elm,state,dismiss", "");
}

static void
about_close()
{
   menu_data *md = g_md;
   elm_object_signal_emit(md->about_layout, "elm,state,dismiss", "");
}

static void
setting_close()
{
   menu_data *md = g_md;
   elm_object_signal_emit(md->setting_layout, "elm,state,dismiss", "");
}

static void
warning_close()
{
   menu_data *md = g_md;
   elm_object_signal_emit(md->warning_layout, "elm,state,dismiss", "");
}

static void
menu_close()
{
   menu_data *md = g_md;
   if (!md->menu_layout) return;
   elm_object_signal_emit(md->menu_layout, "elm,state,dismiss", "");
}

static void
fileselector_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                          const char *emission EINA_UNUSED,
                          const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->fileselector_layout);
   md->fileselector_layout = NULL;
   md->open_depth--;
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
   md->open_depth--;
   if (!md->menu_layout) return;
   elm_object_disabled_set(md->menu_layout, EINA_FALSE);
   elm_object_focus_set(md->menu_layout, EINA_TRUE);
}

static void
about_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->about_layout);
   md->about_layout = NULL;
   md->open_depth--;
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
   md->open_depth--;
}

static void
warning_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                     const char *emission EINA_UNUSED,
                     const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->warning_layout);
   md->warning_layout = NULL;
   md->open_depth--;
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
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "warning_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  warning_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   Evas_Object *btn;

   //Save Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "Save");
   evas_object_smart_callback_add(btn, "clicked", save_cb, md);
   evas_object_show(btn);
   elm_object_focus_set(btn, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.save", btn);

   //No Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "No");
   evas_object_smart_callback_add(btn, "clicked", warning_no_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.no", btn);

   //Yes Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "Yes");
   evas_object_smart_callback_add(btn, "clicked", yes_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.yes", btn);

   if (md->menu_layout)
     elm_object_disabled_set(md->menu_layout, EINA_TRUE);

   md->warning_layout = layout;
   md->open_depth++;
}

static void
setting_apply_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   menu_data *md = data;

   config_edc_img_path_set(elm_object_text_get(md->img_path_entry));
   config_edc_snd_path_set(elm_object_text_get(md->snd_path_entry));
   config_edc_fnt_path_set(elm_object_text_get(md->fnt_path_entry));
   config_edc_data_path_set(elm_object_text_get(md->data_path_entry));
   config_font_size_set((float) elm_slider_value_get(md->slider_font));
   config_view_scale_set(elm_slider_value_get(md->slider_view));
   config_tools_set(elm_check_state_get(md->toggle_tools));
   config_stats_bar_set(elm_check_state_get(md->toggle_stats));
   config_linenumber_set(elm_check_state_get(md->toggle_linenum));
   config_part_highlight_set(elm_check_state_get(md->toggle_highlight));
   config_dummy_swallow_set(elm_check_state_get(md->toggle_swallow));
   config_auto_indent_set(elm_check_state_get(md->toggle_indent));

   config_apply();

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

   img_path_entry_update(md->img_path_entry,
                         (Eina_List *)config_edc_img_path_list_get());
   snd_path_entry_update(md->snd_path_entry,
                         (Eina_List *)config_edc_snd_path_list_get());
   fnt_path_entry_update(md->fnt_path_entry,
                         (Eina_List *)config_edc_fnt_path_list_get());
   data_path_entry_update(md->data_path_entry,
                         (Eina_List *)config_edc_data_path_list_get());

   elm_slider_value_set(md->slider_font, (double) config_font_size_get());
   elm_slider_value_set(md->slider_view, (double) config_view_scale_get());

   elm_check_state_set(md->toggle_tools, config_tools_get());
   elm_check_state_set(md->toggle_stats, config_stats_bar_get());
   elm_check_state_set(md->toggle_linenum, config_linenumber_get());
   elm_check_state_set(md->toggle_highlight, config_part_highlight_get());
   elm_check_state_set(md->toggle_swallow, config_dummy_swallow_get());
   elm_check_state_set(md->toggle_indent, config_auto_indent_get());
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
   evas_object_size_hint_weight_set(toggle, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(toggle, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(toggle, text);
   evas_object_show(toggle);

   return toggle;
}

static void
setting_open(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "setting_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  setting_dismiss_done, md);
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
   Evas_Object *data_path_entry = entry_create(layout);
   data_path_entry_update(data_path_entry,
                         (Eina_List *)config_edc_data_path_list_get());
   elm_object_part_content_set(layout, "elm.swallow.data_path_entry",
                               data_path_entry);

   //Preference
   Evas_Object *scroller = elm_scroller_add(layout);
   elm_object_part_content_set(layout, "elm.swallow.preference", scroller);

   Evas_Object *box = elm_box_add(scroller);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   elm_object_content_set(scroller, box);

   Evas_Object *label, *box2;

   //Font Size
   box2 = elm_box_add(box);
   elm_box_horizontal_set(box2, EINA_TRUE);
   evas_object_size_hint_weight_set(box2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box2);

   elm_box_pack_end(box, box2);

   //Font Size (Label)
   label = elm_label_add(box2);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, 0, EVAS_HINT_FILL);

   elm_object_text_set(label, "Font Size");
   evas_object_show(label);

   elm_box_pack_end(box2, label);

   //Font Size (Slider)
   Evas_Object *slider_font = elm_slider_add(box2);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(slider_font, 1, EVAS_HINT_FILL);
   elm_slider_span_size_set(slider_font, 400);
   elm_slider_indicator_show_set(slider_font, EINA_FALSE);
   elm_slider_unit_format_set(slider_font, "%1.1fx");
   elm_slider_min_max_set(slider_font, MIN_FONT_SIZE, MAX_FONT_SIZE);
   elm_slider_value_set(slider_font, (double) config_font_size_get());
   evas_object_show(slider_font);

   elm_box_pack_end(box2, slider_font);

   //View Scale
   box2 = elm_box_add(box);
   elm_box_horizontal_set(box2, EINA_TRUE);
   evas_object_size_hint_weight_set(box2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box2);

   elm_box_pack_end(box, box2);

   //View Scale (Label)
   label = elm_label_add(box2);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, 0, EVAS_HINT_FILL);

   elm_object_text_set(label, "View Scale");
   evas_object_show(label);

   elm_box_pack_end(box2, label);

   //View Scale (Slider)
   Evas_Object *slider_view = elm_slider_add(box2);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(slider_view, 1, EVAS_HINT_FILL);
   elm_slider_span_size_set(slider_view, 394);
   elm_slider_indicator_show_set(slider_view, EINA_FALSE);
   elm_slider_unit_format_set(slider_view, "%1.2fx");
   elm_slider_min_max_set(slider_view, MIN_VIEW_SCALE, MAX_VIEW_SCALE);
   elm_slider_value_set(slider_view, (double) config_view_scale_get());
   evas_object_show(slider_view);

   elm_box_pack_end(box2, slider_view);

   //Toggle (Tools)
   Evas_Object *toggle_tools = toggle_create(box, "Tools",
                                             config_tools_get());
   elm_box_pack_end(box, toggle_tools);

   //Toggle (Status)
   Evas_Object *toggle_stats = toggle_create(box, "Status",
                                             config_stats_bar_get());
   elm_box_pack_end(box, toggle_stats);

   //Toggle (Line Number)
   Evas_Object *toggle_linenum = toggle_create(box, "Line Number",
                                               config_linenumber_get());
   elm_box_pack_end(box, toggle_linenum);

   //Toggle (Part Highlighting)
   Evas_Object *toggle_highlight = toggle_create(box, "Part Highlighting",
                                   config_part_highlight_get());
   elm_box_pack_end(box, toggle_highlight);

   //Toggle (Dummy Swallow)
   Evas_Object *toggle_swallow = toggle_create(box, "Dummy Swallow",
                                 config_dummy_swallow_get());
   elm_box_pack_end(box, toggle_swallow);

   //Toggle (Auto Indentation)
   Evas_Object *toggle_indent = toggle_create(box, "Auto Indentation",
                                config_auto_indent_get());
   elm_box_pack_end(box, toggle_indent);

   Evas_Object *btn;

   //Apply Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "Apply");
   evas_object_smart_callback_add(btn, "clicked", setting_apply_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.apply_btn", btn);

   //Reset Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "Reset");
   evas_object_smart_callback_add(btn, "clicked", setting_reset_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.reset_btn", btn);

   //Cancel Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "Cancel");
   evas_object_smart_callback_add(btn, "clicked", setting_cancel_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.cancel_btn", btn);

   if (md->menu_layout)
     elm_object_disabled_set(md->menu_layout, EINA_TRUE);

   md->setting_layout = layout;
   md->img_path_entry = img_path_entry;
   md->snd_path_entry = snd_path_entry;
   md->fnt_path_entry = fnt_path_entry;
   md->data_path_entry = data_path_entry;
   md->slider_font = slider_font;
   md->slider_view = slider_view;
   md->toggle_tools = toggle_tools;
   md->toggle_stats = toggle_stats;
   md->toggle_linenum = toggle_linenum;
   md->toggle_highlight = toggle_highlight;
   md->toggle_swallow = toggle_swallow;
   md->toggle_indent = toggle_indent;
   md->open_depth++;
}

static void
about_open(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "about_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  about_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   //Entry
   Evas_Object *entry = elm_entry_add(layout);
   elm_object_style_set(entry, "about");
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_line_wrap_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_FALSE);
   evas_object_show(entry);
   elm_object_focus_set(entry, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.entry", entry);
   elm_entry_entry_append(entry, "<color=#ffffff>");

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
             if (!eina_strbuf_append(strbuf, EOL)) goto err;
          }

        if (!eina_strbuf_append_length(strbuf, line->start, line->length))
          goto err;
        line_num++;
     }
   elm_entry_entry_append(entry, eina_strbuf_string_get(strbuf));
   elm_entry_entry_append(entry, "</font_size></color>");

   if (md->menu_layout)
     elm_object_disabled_set(md->menu_layout, EINA_TRUE);

   md->about_layout = layout;
   md->open_depth++;

err:
   if (strbuf) eina_strbuf_free(strbuf);
   if (itr) eina_iterator_free(itr);
   if (file) eina_file_close(file);
}

static void
about_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   about_open(md);
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
   newfile_new(md->ed, EINA_FALSE);
   warning_close(md);
   menu_close(md);
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

static void
exit_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_exit();
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
   newfile_new(md->ed, EINA_FALSE);
   warning_close(md);
   menu_close(md);
}

static void
new_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   menu_edc_new();
}

static void
save_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   edc_file_save(md);
}

static void
fileselector_save_done_cb(void *data, Evas_Object *obj, void *event_info)
{
   menu_data *md = data;
   const char *selected = event_info;

   eina_stringshare_refplace(&(md->last_accessed_path),
                            elm_fileselector_path_get(obj));

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
   if (strcmp(config_edc_path_get(), selected))
     edit_changed_set(md->ed, EINA_TRUE);

   config_edc_path_set(selected);

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

   edj_mgr_reload_need_set(EINA_TRUE);
   config_apply();

   fileselector_close(md);
   menu_close(md);
}

static void
fileselector_save_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   fileselector_save_done_cb(data, obj, event_info);
}

static void
fileselector_load_done_cb(void *data, Evas_Object *obj, void *event_info)
{
   menu_data *md = data;
   const char *selected = event_info;

   eina_stringshare_refplace(&(md->last_accessed_path),
                             elm_fileselector_path_get(obj));

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
   edit_edc_reload(md->ed, selected);
   fileselector_close(md);
   menu_close(md);
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
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "fileselector_layout");
   elm_object_part_text_set(layout, "elm.text.title", "Save");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  fileselector_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   Evas_Object *fs = elm_fileselector_add(layout);
   elm_object_part_text_set(fs, "ok", "Save");
   elm_object_part_text_set(fs, "cancel", "Close");
   elm_fileselector_path_set(fs, md->last_accessed_path ? md->last_accessed_path : getenv("HOME"));
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
   md->open_depth++;
}

static void
edc_file_load(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "fileselector_layout");
   elm_object_part_text_set(layout, "elm.text.title", "Load");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  fileselector_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   Evas_Object *fs = elm_fileselector_add(layout);
   elm_fileselector_path_set(fs, md->last_accessed_path ? md->last_accessed_path : getenv("HOME"));
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
   md->open_depth++;
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

static void
load_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_edc_load();
}

static void
menu_open(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "menu_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  menu_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

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

   //Button(About)
   btn = btn_create(layout, "About", about_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.about_btn", btn);
   ecore_timer_add(0.12, btn_effect_timer_cb, btn);

   //Button(Exit)
   btn = btn_create(layout, "Exit", exit_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.exit_btn", btn);
   ecore_timer_add(0.15, btn_effect_timer_cb, btn);

   md->menu_layout = layout;
   md->open_depth++;
}

static void
ctxpopup_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   md->ctxpopup = NULL;
}

void
menu_init(edit_data *ed)
{
   menu_data *md = calloc(1, sizeof(menu_data));
   g_md = md;

   md->ed = ed;
}

void
menu_term()
{
   menu_data *md = g_md;
   if (!md) return;
   free(md);
}

void
menu_about()
{
   menu_data *md = g_md;
   about_open(md);
}

void
menu_setting()
{
   menu_data *md = g_md;
   setting_open(md);
}

Eina_Bool
menu_edc_new()
{
   menu_data *md = g_md;
   if (edit_changed_get(md->ed))
     {
        warning_layout_create(md, new_yes_btn_cb, new_save_btn_cb);
        return EINA_TRUE;
     }

   config_edc_path_set(PROTO_EDC_PATH);
   newfile_new(md->ed, EINA_FALSE);
   menu_close(md);

   return EINA_FALSE;
}

void
menu_edc_save()
{
   menu_data *md = g_md;
   edc_file_save(md);
}

void
menu_edc_load()
{
   menu_data *md = g_md;
   if (edit_changed_get(md->ed))
     warning_layout_create(md, load_yes_btn_cb, load_save_btn_cb);
   else
     edc_file_load(md);
}

void
menu_toggle()
{
   menu_data *md = g_md;

   if (md->setting_layout)
     {
        setting_close(md);
        return;
     }
   if (md->warning_layout)
     {
        warning_close(md);
        return;
     }
   if (md->fileselector_layout)
     {
        fileselector_close(md);
        return;
     }
   if (md->about_layout)
     {
        about_close(md);
        return;
     }
   if (md->ctxpopup)
     {
        elm_ctxpopup_dismiss(md->ctxpopup);
        return;
     }

   //Main Menu 
   if (md->open_depth) menu_close(md);
   else menu_open(md);
}

void
menu_ctxpopup_unregister(Evas_Object *ctxpopup)
{
   menu_data *md = g_md;
   evas_object_event_callback_del(ctxpopup, EVAS_CALLBACK_DEL, ctxpopup_del_cb);
   if (ctxpopup == md->ctxpopup) md->ctxpopup = NULL;
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

int
menu_open_depth()
{
   menu_data *md = g_md;
   return md->open_depth;
}

void
menu_exit()
{
   menu_data *md = g_md;
   if (edit_changed_get(md->ed))
     {
        search_close();
        warning_layout_create(md, exit_yes_btn_cb, exit_save_btn_cb);
     }
   else
     elm_exit();
}
