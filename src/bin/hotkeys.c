#include <Elementary.h>
#include "common.h"


static void
f1_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   menu_about(md);
}

static void
f2_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   menu_edc_new(md);
}

static void
f3_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   menu_edc_save(md);
}

static void
f4_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   menu_edc_load(md);
}

static void
f5_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
/*   app_data *ad = data;
   config_linenumber_set(ad->cd, !config_linenumber_get(ad->cd));
   edit_line_number_toggle(ad->ed); */
}

static void
f6_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
/*   app_data *ad = data;
   config_stats_bar_set(ad->cd, !config_stats_bar_get(ad->cd));
   statusbar_toggle(ad); */
}

static void
f12_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   menu_setting(md);
}


Evas_Object *
hotkeys_create(Evas_Object *parent, menu_data *md)
{
   Evas_Object *box = elm_box_add(parent);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_show(box);

   Evas_Object *btn;

   //F1 - About
   btn = elm_button_add(box);
   elm_object_style_set(btn, "anchor");
   elm_object_text_set(btn, "F1: About");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   evas_object_smart_callback_add(btn, "clicked", f1_cb, md);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);

   //F2 - New
   btn = elm_button_add(box);
   elm_object_style_set(btn, "anchor");
   elm_object_text_set(btn, "F2: New");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   evas_object_smart_callback_add(btn, "clicked", f2_cb, md);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);

   //F3 - Save
   btn = elm_button_add(box);
   elm_object_style_set(btn, "anchor");
   elm_object_text_set(btn, "F3: Save");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   evas_object_smart_callback_add(btn, "clicked", f3_cb, md);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);

   //F4 - Load
   btn = elm_button_add(box);
   elm_object_style_set(btn, "anchor");
   elm_object_text_set(btn, "F4: Load");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   evas_object_smart_callback_add(btn, "clicked", f4_cb, md);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);

   //F5 - Line Number
   btn = elm_button_add(box);
   elm_object_style_set(btn, "anchor");
   //elm_object_text_set(btn, "F5: Line Num");
   elm_object_text_set(btn, "F5: --");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   elm_object_disabled_set(btn, EINA_TRUE);
   evas_object_smart_callback_add(btn, "clicked", f5_cb, md);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);

   //F6 - Status
   btn = elm_button_add(box);
   elm_object_style_set(btn, "anchor");
   //elm_object_text_set(btn, "F6: Status");
   elm_object_text_set(btn, "F6: --");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   elm_object_disabled_set(btn, EINA_TRUE);
   evas_object_smart_callback_add(btn, "clicked", f6_cb, md);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);

   //F7 - File Tab
   btn = elm_button_add(box);
   elm_object_style_set(btn, "anchor");
   elm_object_text_set(btn, "F7: --");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   elm_object_disabled_set(btn, EINA_TRUE);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);

   //F8 - Console
   btn = elm_button_add(box);
   elm_object_style_set(btn, "anchor");
   elm_object_text_set(btn, "F8: --");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   elm_object_disabled_set(btn, EINA_TRUE);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);

   //F9 -
   btn = elm_button_add(box);
   elm_object_style_set(btn, "anchor");
   elm_object_text_set(btn, "F9: --");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   elm_object_disabled_set(btn, EINA_TRUE);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);

   //F10 -
   btn = elm_button_add(box);
   elm_object_style_set(btn, "anchor");
   elm_object_text_set(btn, "F10: --");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   elm_object_disabled_set(btn, EINA_TRUE);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);

   //F11 - 
   btn = elm_button_add(box);
   elm_object_style_set(btn, "anchor");
   elm_object_text_set(btn, "F11: --");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   elm_object_disabled_set(btn, EINA_TRUE);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);

   //F12 - Setting
   btn = elm_button_add(box);
   elm_object_style_set(btn, "anchor");
   elm_object_text_set(btn, "F12: Setting");
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   evas_object_smart_callback_add(btn, "clicked", f12_cb, md);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);

   return box;
}
