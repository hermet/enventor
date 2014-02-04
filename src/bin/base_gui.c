#include <Elementary.h>
#include "common.h"

struct base_s
{
   Evas_Object *win;
   Evas_Object *layout;
   Evas_Object *panes;
};

static base_data *g_bd = NULL;

static void
win_delete_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
  menu_exit();
}

void
base_statusbar_toggle()
{
   base_data *bd = g_bd;

   if (config_stats_bar_get())
     elm_object_signal_emit(bd->layout, "elm,state,statusbar,show", "");
   else
     elm_object_signal_emit(bd->layout, "elm,state,statusbar,hide", "");
}

//This function is used in panes. Maybe layout should be separated from main.
void
base_hotkey_toggle()
{
   base_data *bd = g_bd;

   config_hotkeys_set(!config_hotkeys_get());

   if (config_hotkeys_get())
     elm_object_signal_emit(bd->layout, "elm,state,hotkeys,show", "");
   else
     elm_object_signal_emit(bd->layout, "elm,state,hotkeys,hide", "");
}

Evas_Object *
base_win_get()
{
   base_data *bd = g_bd;
   return bd->win;
}

Evas_Object *
base_layout_get()
{
   base_data *bd = g_bd;
   return bd->layout;
}

void
base_win_resize_object_add(Evas_Object *resize_obj)
{
   base_data *bd = g_bd;
   elm_win_resize_object_add(bd->win, resize_obj);
}

void base_hotkeys_set(Evas_Object *hotkeys)
{
   base_data *bd = g_bd;
   elm_object_part_content_set(bd->layout, "elm.swallow.hotkeys", hotkeys);
}

void
base_full_view_left()
{
   base_data *bd = g_bd;
   panes_full_view_left(bd->panes);
}

void
base_full_view_right()
{
   base_data *bd = g_bd;
   panes_full_view_right(bd->panes);
}

void
base_right_view_set(Evas_Object *right)
{
   base_data *bd = g_bd;
   elm_object_part_content_set(bd->panes, "right", right);
}

void
base_left_view_set(Evas_Object *left)
{
   base_data *bd = g_bd;
   elm_object_part_content_set(bd->panes, "left", left);
}

void
base_gui_term()
{
   base_data *bd = g_bd;
   free(bd);
}

Eina_Bool
base_gui_init()
{
   char buf[PATH_MAX];

   base_data *bd = calloc(1, sizeof(base_data));
   g_bd = bd;

   //Window
   Evas_Object *win = elm_win_util_standard_add(elm_app_name_get(),
                                                "Enventor");
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb,
                                  NULL);
   evas_object_show(win);

   //Window icon
   Evas_Object *icon = evas_object_image_add(evas_object_evas_get(win));
   snprintf(buf, sizeof(buf), "%s/images/logo.png", elm_app_data_dir_get());
   evas_object_image_file_set(icon, buf, NULL);
   evas_object_show(icon);
   elm_win_icon_object_set(win, icon);

   //Base Layout
   Evas_Object *layout = elm_layout_add(win);
   elm_layout_file_set(layout, EDJE_PATH,  "main_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, layout);
   evas_object_show(layout);

   //Panes
   Evas_Object *panes = panes_create(layout);
   elm_object_part_content_set(layout, "elm.swallow.panes", panes);

   bd->win = win;
   bd->layout = layout;
   bd->panes = panes;

   return EINA_TRUE;
}
