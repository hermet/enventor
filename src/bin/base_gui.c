#include <Elementary.h>
#include "common.h"

typedef struct base_s
{
   Evas_Object *win;
   Evas_Object *layout;
   Evas_Object *console;
} base_data;

static base_data *g_bd = NULL;

static void
win_delete_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
  menu_exit();
}

static void
win_focused_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   goto_close();
}

void
base_title_set(const char *path)
{
   base_data *bd = g_bd;
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s - Enventor", path);
   elm_win_title_set(bd->win, buf);
}

void
base_statusbar_toggle(Eina_Bool config)
{
   base_data *bd = g_bd;

   if (config) config_stats_bar_set(!config_stats_bar_get());

   if (config_stats_bar_get())
     elm_object_signal_emit(bd->layout, "elm,state,statusbar,show", "");
   else
     elm_object_signal_emit(bd->layout, "elm,state,statusbar,hide", "");
}

void
base_tools_toggle(Eina_Bool config)
{
   base_data *bd = g_bd;

   if (config) config_tools_set(!config_tools_get());

   if (config_tools_get())
     elm_object_signal_emit(bd->layout, "elm,state,tools,show", "");
   else
     elm_object_signal_emit(bd->layout, "elm,state,tools,hide", "");
}

Evas_Object *
base_win_get(void)
{
   base_data *bd = g_bd;
   return bd->win;
}

Evas_Object *
base_layout_get(void)
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

void base_tools_set(Evas_Object *tools)
{
   base_data *bd = g_bd;
   elm_object_part_content_set(bd->layout, "elm.swallow.tools", tools);

   if (config_tools_get())
     elm_object_signal_emit(bd->layout, "elm,state,tools,show", "");
   else
     elm_object_signal_emit(bd->layout, "elm,state,tools,hide", "");
}

void
base_live_view_full_view(void)
{
   panes_live_view_full_view();
}

void
base_text_editor_full_view(void)
{
   panes_text_editor_full_view();
}

void
base_editors_full_view(void)
{
   panes_editors_full_view();
}

void
base_console_full_view(void)
{
   panes_console_full_view();
}

void
base_text_editor_set(Evas_Object *text_editor)
{
   panes_text_editor_set(text_editor);
}

void
base_live_view_set(Evas_Object *live_view)
{
   panes_live_view_set(live_view);
}

void
base_console_toggle()
{
   base_data *bd = g_bd;
   panes_editors_full_view();
}

void
base_gui_term(void)
{
   base_data *bd = g_bd;
   panes_term();
   free(bd);
}

static void
err_noti_cb(void *data, const char *msg)
{
   base_data *bd = data;
   if (panes_editors_full_view_get()) base_editors_full_view();
   elm_object_signal_emit(bd->layout, "elm,state,alert,show", "");
   console_text_set(bd->console, msg);
}

void
base_console_reset(void)
{
   base_data *bd = g_bd;
   console_text_set(bd->console, "");
}

Eina_Bool
base_gui_init(void)
{
   char buf[PATH_MAX];

   base_data *bd = calloc(1, sizeof(base_data));
   if (!bd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return EINA_FALSE;
     }
   g_bd = bd;

   //Window
   Evas_Object *win = elm_win_util_standard_add(elm_app_name_get(),
                                                "Enventor");
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb,
                                  NULL);
   evas_object_smart_callback_add(win, "focused", win_focused_cb, NULL);

   //Window icon
   Evas_Object *icon = evas_object_image_add(evas_object_evas_get(win));
   snprintf(buf, sizeof(buf), "%s/images/logo.png", elm_app_data_dir_get());
   evas_object_image_file_set(icon, buf, NULL);
   elm_win_icon_object_set(win, icon);

   //Base Layout
   Evas_Object *layout = elm_layout_add(win);
   elm_layout_file_set(layout, EDJE_PATH,  "main_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, layout);
   evas_object_show(layout);

   //Panes
   Evas_Object *panes = panes_init(layout);
   elm_object_part_content_set(layout, "elm.swallow.panes", panes);

   //Console
   Evas_Object *console = console_create(panes);
   panes_console_set(console);

   build_err_noti_cb_set(err_noti_cb, bd);

   bd->win = win;
   bd->layout = layout;
   bd->console = console;

   return EINA_TRUE;
}

void
base_gui_show(void)
{
   evas_object_show(g_bd->win);
}
