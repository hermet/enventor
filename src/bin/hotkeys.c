#include <Elementary.h>
#include "common.h"

#define TOOLBAR_ICON_SIZE 16

Eina_Bool
unselect_anim_cb(void *data)
{
   Elm_Object_Item *it = data;
   elm_toolbar_item_selected_set(it, EINA_FALSE);
   return ECORE_CALLBACK_CANCEL;
}

static void
item_unselect(Elm_Object_Item *it)
{
   ecore_timer_add(0.1, unselect_anim_cb, it);
}

static void
f2_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   menu_edc_new();
   item_unselect((Elm_Object_Item *)event_info);
}

static void
f3_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   menu_edc_save();
   item_unselect((Elm_Object_Item *)event_info);
}

static void
f4_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   menu_edc_load();
   item_unselect((Elm_Object_Item *)event_info);
}

static void
f5_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   edit_data *ed = data;
   config_part_highlight_set(!config_part_highlight_get());
   edit_part_highlight_toggle(ed, EINA_TRUE);
   item_unselect((Elm_Object_Item *)event_info);
}

static void
f6_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   config_dummy_swallow_set(!config_dummy_swallow_get());
   view_dummy_toggle(VIEW_DATA, EINA_TRUE);
   item_unselect((Elm_Object_Item *)event_info);
}

static void
f9_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   edit_data *ed = data;
   config_linenumber_set(!config_linenumber_get());
   edit_line_number_toggle(ed);
   item_unselect((Elm_Object_Item *)event_info);
}

static void
f10_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   config_stats_bar_set(!config_stats_bar_get());
   base_statusbar_toggle();
   item_unselect((Elm_Object_Item *)event_info);
}

static void
f11_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   if (search_is_opened()) search_close();
   else search_open();
   item_unselect((Elm_Object_Item *)event_info);
}

static void
f12_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   menu_setting();
   item_unselect((Elm_Object_Item *)event_info);
}

Evas_Object *
hotkeys_create(Evas_Object *parent, edit_data *ed)
{
   Evas_Object *toolbar = elm_toolbar_add(parent);
   elm_object_style_set(toolbar, "item_horizontal");
   elm_toolbar_icon_size_set(toolbar,
                             TOOLBAR_ICON_SIZE * elm_config_scale_get());
   elm_toolbar_transverse_expanded_set(toolbar, EINA_TRUE);
   elm_toolbar_homogeneous_set(toolbar, EINA_FALSE);
   elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_EXPAND);
   evas_object_size_hint_weight_set(toolbar, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(toolbar, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_focus_allow_set(toolbar, EINA_FALSE);

   Elm_Object_Item *it;
   Evas_Object *icon;
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/images/new.png", elm_app_data_dir_get());
   it = elm_toolbar_item_append(toolbar, buf, "New", f2_cb, NULL);
   snprintf(buf, sizeof(buf), "%s/images/file.png", elm_app_data_dir_get());
   it = elm_toolbar_item_append(toolbar, buf, "Save", f3_cb, NULL);
   snprintf(buf, sizeof(buf), "%s/images/file.png", elm_app_data_dir_get());
   it = elm_toolbar_item_append(toolbar, buf, "Load", f4_cb, NULL);
   snprintf(buf, sizeof(buf), "%s/images/highlight.png",
            elm_app_data_dir_get());
   it = elm_toolbar_item_append(toolbar, buf, "Highlight", f5_cb, ed);
   snprintf(buf, sizeof(buf), "%s/images/swallow.png", elm_app_data_dir_get());
   it = elm_toolbar_item_append(toolbar, buf, "Swallow", f6_cb, NULL);
   snprintf(buf, sizeof(buf), "%s/images/lines.png", elm_app_data_dir_get());
   it = elm_toolbar_item_append(toolbar, buf, "Lines", f9_cb, ed);
   snprintf(buf, sizeof(buf), "%s/images/status.png", elm_app_data_dir_get());
   it = elm_toolbar_item_append(toolbar, buf, "Status", f10_cb, NULL);
   snprintf(buf, sizeof(buf), "%s/images/find.png", elm_app_data_dir_get());
   it = elm_toolbar_item_append(toolbar, buf, "Find", f11_cb, NULL);
   snprintf(buf, sizeof(buf), "%s/images/setting.png", elm_app_data_dir_get());
   it = elm_toolbar_item_append(toolbar, buf, "Setting", f12_cb, NULL);

   return toolbar;
}
