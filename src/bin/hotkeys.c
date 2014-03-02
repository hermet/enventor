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
   config_stats_bar_set(!config_stats_bar_get());
   base_statusbar_toggle();
}

static void
f12_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
       void *event_info EINA_UNUSED)
{
   menu_setting();
}

static Evas_Object *
btn_create(Evas_Object *parent, const char *text, Evas_Smart_Cb cb, void *data, const char *img_name)
{
   Evas_Object *btn = elm_button_add(parent);
   elm_object_style_set(btn, "anchor");
   elm_object_text_set(btn, text);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   evas_object_smart_callback_add(btn, "clicked", cb, data);
   evas_object_show(btn);

   Evas_Object *img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, img_name);
   elm_object_content_set(btn, img);

   return btn;
}

Evas_Object *
hotkeys_create(Evas_Object *parent, edit_data *ed)
{
   Evas_Object *box = elm_box_add(parent);
   elm_box_horizontal_set(box, EINA_TRUE);

   Evas_Object *btn;

   btn = btn_create(box, "About", f1_cb, NULL, "logo");
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "New", f2_cb, NULL, "new");
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "Save", f3_cb, NULL, "save");
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "Load", f4_cb, NULL, "load");
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "Lines", f5_cb, ed, "lines");
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "Status", f6_cb, NULL, "status");
   elm_box_pack_end(box, btn);

   btn = btn_create(box, "Setting", f12_cb, NULL, "setting");
   elm_box_pack_end(box, btn);

   return box;
}
