#include <Elementary.h>
#include "common.h"

#define TOOLBAR_ICON_SIZE 16

static void
menu_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   edit_data *ed = data;

   if (search_is_opened() || goto_is_opened())
     {
        goto_close();
        search_close();
        edit_focus_set(ed);
     }

   menu_toggle();
}

static void
save_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   menu_edc_save();
}

static void
load_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   menu_edc_load();
}

static void
highlight_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
             void *event_info)
{
   edit_data *ed = data;
   config_part_highlight_set(!config_part_highlight_get());
   edit_part_highlight_toggle(ed, EINA_TRUE);
}

static void
swallow_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
           void *event_info)
{
   config_dummy_swallow_set(!config_dummy_swallow_get());
   view_dummy_toggle(VIEW_DATA, EINA_TRUE);
}

static void
lines_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   edit_data *ed = data;
   config_linenumber_set(!config_linenumber_get());
   edit_line_number_toggle(ed);
}

static void
status_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
          void *event_info)
{
   base_statusbar_toggle(EINA_TRUE);
}

static void
find_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   edit_data *ed = data;
   if (search_is_opened()) search_close();
   else search_open(ed);
}

static void
goto_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   edit_data *ed = data;
   if (goto_is_opened()) goto_close();
   else goto_open(ed);
}

static void
console_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
           void *event_info)
{
   base_console_toggle();
}

static void
live_edit_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   live_edit_toggle();
}

static Evas_Object *
tools_btn_create(Evas_Object *parent, const char *icon, const char *label,
                 Evas_Smart_Cb func, void *data)
{
   Evas_Object *btn = elm_button_add(parent);
   elm_object_style_set(btn, "anchor");
   elm_object_focus_allow_set(btn, EINA_FALSE);

   Evas_Object *img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, icon);
   elm_object_content_set(btn, img);

   elm_object_text_set(btn, label);
   evas_object_smart_callback_add(btn, "clicked", func, data);
   evas_object_show(btn);

   return btn;
}

Evas_Object *
tools_create(Evas_Object *parent, edit_data *ed)
{
   Evas_Object *box = elm_box_add(parent);
   elm_box_horizontal_set(box, EINA_TRUE);
   elm_box_padding_set(box, 10, 0);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);

   Evas_Object *btn;
   btn = tools_btn_create(box, "menu", "Menu", menu_cb, ed);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   Evas_Object *sp;
   sp = elm_separator_add(box);
   evas_object_show(sp);
   elm_box_pack_end(box, sp);

   btn = tools_btn_create(box, "highlight", "Highlight", highlight_cb, ed);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   btn = tools_btn_create(box, "swallow_s", "Swallow",
                          swallow_cb, NULL);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   btn = tools_btn_create(box, "live_edit", "LiveEdit", live_edit_cb, NULL);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   sp = elm_separator_add(box);
   evas_object_show(sp);
   elm_box_pack_end(box, sp);

   btn = tools_btn_create(box, "lines", "Lines", lines_cb, ed);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   btn = tools_btn_create(box, "find", "Find", find_cb, ed);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   btn = tools_btn_create(box, "line", "Goto", goto_cb, ed);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   sp = elm_separator_add(box);
   evas_object_show(sp);
   elm_box_pack_end(box, sp);

   btn = tools_btn_create(box, "console", "Console", console_cb, NULL);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 1.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   btn = tools_btn_create(box, "status", "Status", status_cb, NULL);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 1.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   evas_object_show(box);

   return box;
}
