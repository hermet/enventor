#include <Elementary.h>
#include "common.h"


static void
f1_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
      void *event_info EINA_UNUSED)
{
   menu_about();
}

static void
f2_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
      void *event_info EINA_UNUSED)
{
   menu_edc_new();
}

static void
f3_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
      void *event_info EINA_UNUSED)
{
   menu_edc_save();
}

static void
f4_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
      void *event_info EINA_UNUSED)
{
   menu_edc_load();
}

static void
f5_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
      void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   config_linenumber_set(!config_linenumber_get());
   edit_line_number_toggle(ed);
}

static void
f6_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
      void *event_info EINA_UNUSED)
{
   //FIXME: ... how ad can be passed here? should be cut off from here.
   app_data *ad = data;
   config_stats_bar_set(!config_stats_bar_get());
   statusbar_toggle(ad);
}

static void
f12_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
       void *event_info EINA_UNUSED)
{
   menu_setting();
}

static Evas_Object *
btn_create(Evas_Object *parent, const char *text, Evas_Smart_Cb cb, void *data)
{
   Evas_Object *btn = elm_button_add(parent);
   elm_object_style_set(btn, "anchor");
   elm_object_text_set(btn, text);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   evas_object_smart_callback_add(btn, "clicked", cb, data);
   evas_object_show(btn);

   return btn;
}

Evas_Object *
hotkeys_create(Evas_Object *parent, app_data *ad, edit_data *ed)
{
   Evas_Object *box = elm_box_add(parent);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_show(box);

   Evas_Object *btn;

   btn = btn_create(box, "F1: About", f1_cb, NULL);
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "F2: New", f2_cb, NULL);
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "F3: Save", f3_cb, NULL);
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "F4: Load", f4_cb, NULL);
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "F5: Line Num", f5_cb, ed);
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "F6: Status", f6_cb, ad);
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "F7: ---", NULL, NULL);
   elm_object_disabled_set(btn, EINA_TRUE);
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "F8: ---", NULL, NULL);
   elm_object_disabled_set(btn, EINA_TRUE);
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "F9: ---", NULL, NULL);
   elm_object_disabled_set(btn, EINA_TRUE);
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "F10: ---", NULL, NULL);
   elm_object_disabled_set(btn, EINA_TRUE);
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "F11: ---", NULL, NULL);
   elm_object_disabled_set(btn, EINA_TRUE);
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "F12: Setting", f12_cb, NULL);
   elm_box_pack_end(box, btn);

   return box;
}
