#include "common.h"

typedef struct base_s
{
   Evas_Object *win;
   Evas_Object *layout;
   Evas_Object *console;
   Enventor_Object *enventor;
   Ecore_Timer *edc_navi_update_timer;
   Eina_Bool console_msg : 1;
} base_data;

static base_data *g_bd = NULL;



/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
win_delete_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
  menu_exit();
}

static void
win_resize_cb(void *data EINA_UNUSED, Evas *o EINA_UNUSED, Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   Evas_Coord w, h;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   config_win_size_set(w, h);
}

static Eina_Bool
edc_navigator_update_timer_cb(void *data)
{
   base_data *bd = data;
   edc_navigator_group_update(stats_group_name_get());
   bd->edc_navi_update_timer = NULL;

   return ECORE_CALLBACK_CANCEL;
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
base_error_msg_set(const char *msg)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   if (config_red_alert_get())
     elm_object_signal_emit(bd->layout, "elm,state,alert,show", "");
   console_text_set(bd->console, msg);
   panes_editors_full_view(EINA_FALSE);
   bd->console_msg = EINA_TRUE;
}

void
base_title_set(const char *path)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), _("%s - Enventor"), path);
   elm_win_title_set(bd->win, buf);
}

void
base_statusbar_toggle(Eina_Bool toggle)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   if (toggle) config_stats_bar_set(!config_stats_bar_get());

   if (config_stats_bar_get())
     elm_object_signal_emit(bd->layout, "elm,state,statusbar,show", "");
   else
     elm_object_signal_emit(bd->layout, "elm,state,statusbar,hide", "");
}

void
base_file_tab_toggle(Eina_Bool toggle)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   if (toggle) config_file_tab_set(!config_file_tab_get());

   if (config_file_tab_get())
     {
        file_tab_disabled_set(EINA_FALSE);
        elm_object_signal_emit(bd->layout, "elm,state,file_tab,show", "");
     }
   else
     {
        file_tab_disabled_set(EINA_TRUE);
        elm_object_signal_emit(bd->layout, "elm,state,file_tab,hide", "");
     }
}

void base_file_browser_toggle(Eina_Bool toggle)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   if (toggle) config_file_browser_set(!config_file_browser_get());

   if (config_file_browser_get())
     elm_object_signal_emit(bd->layout, "elm,state,file_browser,show", "");
   else
     elm_object_signal_emit(bd->layout, "elm,state,file_browser,hide", "");
}

void base_edc_navigator_deselect(void)
{
   if (config_edc_navigator_get())
     edc_navigator_deselect();
}

void base_edc_navigator_toggle(Eina_Bool toggle)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   if (toggle) config_edc_navigator_set(!config_edc_navigator_get());

   if (config_edc_navigator_get())
     {
        base_edc_navigator_group_update();
        elm_object_signal_emit(bd->layout, "elm,state,edc_navigator,show", "");
     }
   else
     {
        elm_object_signal_emit(bd->layout, "elm,state,edc_navigator,hide", "");
        edc_navigator_deselect();
     }
}

void
base_tools_toggle(Eina_Bool config)
{
   if (config) config_tools_set(!config_tools_get());

   Eina_Bool visible = config_tools_get();

   panes_live_view_tools_visible_set(visible);
   panes_text_editor_tools_visible_set(visible);
   file_browser_tools_visible_set(visible);
   edc_navigator_tools_visible_set(visible);
}

Evas_Object *
base_win_get(void)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(bd, NULL);

   return bd->win;
}

Evas_Object *
base_layout_get(void)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(bd, NULL);

   return bd->layout;
}

void
base_win_resize_object_add(Evas_Object *resize_obj)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   elm_win_resize_object_add(bd->win, resize_obj);
}

void base_live_edit_fixed_bar_set(Evas_Object *fixed_bar)
{
   panes_live_edit_fixed_bar_set(fixed_bar);
}

void base_tools_set(Evas_Object *live_view_tools,
                    Evas_Object *text_editor_tools)
{
   panes_live_view_tools_set(live_view_tools);
   panes_text_editor_tools_set(text_editor_tools);
   file_browser_tools_set();
   edc_navigator_tools_set();

   Eina_Bool visible = config_tools_get();

   panes_live_view_tools_visible_set(visible);
   panes_text_editor_tools_visible_set(visible);
   file_browser_tools_visible_set(visible);
   edc_navigator_tools_visible_set(visible);
}

void
base_live_view_full_view(void)
{
   panes_live_view_full_view();
}

void
base_enventor_full_view(void)
{
   panes_text_editor_full_view();
}

void
base_editors_full_view(void)
{
   base_console_toggle();
}

void
base_console_full_view(void)
{
   panes_console_full_view();
   tools_console_update(EINA_TRUE);
}

void
base_live_view_set(Evas_Object *live_view)
{
   panes_live_view_set(live_view);
}

void
base_console_auto_hide(void)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);;

   if (!config_console_get()) return;
   if (bd->console_msg) return;
   panes_editors_full_view(EINA_TRUE);
   tools_console_update(EINA_FALSE);
}

void
base_console_toggle(void)
{
   if (panes_editors_full_view_get())
     {
        tools_console_update(EINA_TRUE);
        panes_editors_full_view(EINA_FALSE);
     }
   else
     {
        tools_console_update(EINA_FALSE);
        panes_editors_full_view(EINA_TRUE);
     }
}

void
base_gui_term(void)
{
   base_data *bd = g_bd;
   if (!bd) return;

   ecore_timer_del(bd->edc_navi_update_timer);
   file_browser_term();
   edc_navigator_term();
   file_tab_term();
   panes_term();

   free(bd);
   g_bd = NULL;
}

void
base_edc_navigator_group_update(void)
{
   if (!config_edc_navigator_get()) return;

   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   ecore_timer_del(bd->edc_navi_update_timer);
   bd->edc_navi_update_timer = ecore_timer_add(EDC_NAVIGATOR_UPDATE_TIME,
                                               edc_navigator_update_timer_cb,
                                               bd);
}

void
base_console_reset(void)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   console_text_set(bd->console, "");
   bd->console_msg = EINA_FALSE;
   if (config_console_get()) panes_editors_full_view(EINA_TRUE);
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
   Evas_Object *win = elm_win_util_standard_add("enventor", "Enventor");
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);

   Evas_Coord w, h;
   config_win_size_get(&w, &h);
   if ((w > 0) && (h > 0))
     evas_object_resize(win, w, h);

   evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, win_resize_cb,
                                  NULL);
   evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb,
                                  NULL);

   //Window icon
   Evas_Object *icon = evas_object_image_add(evas_object_evas_get(win));
   snprintf(buf, sizeof(buf), "%s/images/enventor_logo.png",
            elm_app_data_dir_get());
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

   if (config_console_get())
     panes_editors_full_view(EINA_TRUE);

   //File Browser
   Evas_Object *file_browser = file_browser_init(layout);
   elm_object_part_content_set(layout, "elm.swallow.file_browser",
                               file_browser);
   file_browser_workspace_set(config_workspace_path_get());

   //EDC Navigator
   Evas_Object *edc_navigator = edc_navigator_init(layout);
   elm_object_part_content_set(layout, "elm.swallow.edc_navigator",
                               edc_navigator);
   //File Tab
   Evas_Object *file_tab = file_tab_init(layout);
   elm_object_part_content_set(layout, "elm.swallow.file_tab", file_tab);

   bd->win = win;
   bd->layout = layout;
   bd->console = console;

   return EINA_TRUE;
}

void
base_gui_show(void)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   evas_object_resize(bd->win, INIT_WIN_W, INIT_WIN_H);
   evas_object_show(bd->win);
}

Enventor_Object *
base_enventor_get(void)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(bd, NULL);

   return bd->enventor;
}

void
base_enventor_set(Enventor_Object *enventor)
{
   base_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   bd->enventor = enventor;

   panes_text_editor_set(enventor);
}
