#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Ecore_Getopt.h>
#include <Eio.h>
#include "common.h"

#ifdef _WIN32
#include <Windows.h>
static HANDLE hMutex = NULL;
#endif

typedef struct app_s
{
   Evas_Object *keygrabber;
   Eina_Bool on_saving : 1;
   Eina_Bool lazy_save : 1;
} app_data;

static Eina_Bool own_lock = EINA_FALSE;  //lock file owner?

int main(int argc, char **argv);

static void
tools_update(void)
{
   tools_lines_update(EINA_FALSE);
   tools_highlight_update(EINA_FALSE);
   tools_dummy_update(EINA_FALSE);
   tools_wireframes_update(EINA_FALSE);
   tools_mirror_mode_update(EINA_FALSE);
   tools_file_tab_update(EINA_FALSE);
   tools_file_browser_update(EINA_FALSE);
   tools_edc_navigator_update(EINA_FALSE);
}

static void
enventor_common_setup(Enventor_Object *enventor)
{
   const char *font_name;
   const char *font_style;
   config_font_get(&font_name, &font_style);
   enventor_object_font_set(enventor, font_name, font_style);
   enventor_object_font_scale_set(enventor, config_font_scale_get());
   enventor_object_auto_indent_set(enventor, config_auto_indent_get());
   enventor_object_auto_complete_set(enventor, config_auto_complete_get());
   enventor_object_smart_undo_redo_set(enventor, config_smart_undo_redo_get());

   Eina_List *list = eina_list_append(NULL, config_output_path_get());
   enventor_object_path_set(enventor, ENVENTOR_PATH_TYPE_EDJ, list);
   eina_list_free(list);

   enventor_object_path_set(enventor, ENVENTOR_PATH_TYPE_IMAGE,
                            config_img_path_list_get());
   enventor_object_path_set(enventor, ENVENTOR_PATH_TYPE_SOUND,
                            config_snd_path_list_get());
   enventor_object_path_set(enventor, ENVENTOR_PATH_TYPE_FONT,
                            config_fnt_path_list_get());
   enventor_object_path_set(enventor, ENVENTOR_PATH_TYPE_DATA,
                            config_dat_path_list_get());
}

static void
syntax_color_update(Enventor_Object *enventor)
{
   const char *config_color;
   const char *enventor_color;
   Eina_Bool color_changed = EINA_FALSE;
   Enventor_Syntax_Color_Type color_type;

   color_type = ENVENTOR_SYNTAX_COLOR_STRING;
   for (; color_type < ENVENTOR_SYNTAX_COLOR_LAST; color_type++)
     {
        config_color = config_syntax_color_get(color_type);
        if (config_color)
          {
             enventor_color = enventor_object_syntax_color_get(enventor,
                                                               color_type);
             if (!enventor_color || strcmp(config_color, enventor_color))
               {
                  enventor_object_syntax_color_set(enventor, color_type,
                                                   config_color);
                  color_changed = EINA_TRUE;
               }
          }
     }

   if (!color_changed) return;

   Enventor_Item *it;

   it = file_mgr_main_item_get();
   enventor_item_syntax_color_full_apply(it, EINA_TRUE);

   Eina_List *l;
   Eina_List *sub_its = (Eina_List *)enventor_object_sub_items_get(enventor);
   EINA_LIST_FOREACH(sub_its, l, it)
     enventor_item_syntax_color_full_apply(it, EINA_TRUE);

}

static void
syntax_color_init(Enventor_Object *enventor)
{
   const char *config_color;
   const char *enventor_color;
   Enventor_Syntax_Color_Type color_type;

   color_type = ENVENTOR_SYNTAX_COLOR_STRING;
   for (; color_type < ENVENTOR_SYNTAX_COLOR_LAST; color_type++)
     {
        config_color = config_syntax_color_get(color_type);
        if (!config_color)
          {
             enventor_color = enventor_object_syntax_color_get(enventor,
                                                               color_type);
             config_syntax_color_set(color_type, enventor_color);
          }
     }
}

static void
main_edc_update(Eina_Bool refresh)
{
   //Update file browser only if main item is changed,
   Enventor_Item *main_it = enventor_object_main_item_get(base_enventor_get());
   const char *prev_path = NULL;
   if (main_it) prev_path = enventor_item_file_get(main_it);
   if (prev_path == config_input_path_get()) return;

   file_mgr_main_file_set(config_input_path_get());

   if (refresh) file_browser_refresh();
}

static void
config_update_cb(void *data EINA_UNUSED)
{
   Enventor_Object *enventor = base_enventor_get();

   enventor_common_setup(enventor);

   syntax_color_update(enventor);

   base_tools_toggle(EINA_FALSE);
   base_statusbar_toggle(EINA_FALSE);
   base_console_auto_hide();

   main_edc_update(EINA_TRUE);
}

static Eina_Bool
main_mouse_wheel_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Mouse_Wheel *event = ev;
   Evas_Coord x, y, w, h;

   if (!EVENT_KEY_MODIFIER_CHECK(CTRL, event->modifiers))
      return ECORE_CALLBACK_PASS_ON;

   //View Scale
   Evas_Object *view = enventor_object_live_view_get(base_enventor_get());
   evas_object_geometry_get(view, &x, &y, &w, &h);

   if ((event->x >= x) && (event->x <= (x + w)) &&
       (event->y >= y) && (event->y <= (y + h)))
     {
        double scale = enventor_object_live_view_scale_get(base_enventor_get());

        if (event->z < 0) scale += 0.05;
        else scale -= 0.05;

        if (scale > MAX_VIEW_SCALE) scale = MAX_VIEW_SCALE;
        else if (scale < MIN_VIEW_SCALE) scale = MIN_VIEW_SCALE;

        enventor_object_live_view_scale_set(base_enventor_get(), scale);

        //Just in live edit mode case.
        live_edit_update();

        stats_view_scale_update(scale);

        return ECORE_CALLBACK_PASS_ON;
     }

   //Font Size
   Enventor_Item *it = file_mgr_focused_item_get();
   evas_object_geometry_get(enventor_item_editor_get(it), &x, &y, &w, &h);

   if ((event->x >= x) && (event->x <= (x + w)) &&
       (event->y >= y) && (event->y <= (y + h)))
     {
        if (event->z < 0)
          {
             config_font_scale_set(config_font_scale_get() + 0.1f);
             enventor_object_font_scale_set(base_enventor_get(),
                                            config_font_scale_get());
          }
        else
          {
             config_font_scale_set(config_font_scale_get() - 0.1f);
             enventor_object_font_scale_set(base_enventor_get(),
                                            config_font_scale_get());
          }

        char buf[128];
        snprintf(buf, sizeof(buf), _("Font Size: %1.1fx"),
                 config_font_scale_get());
        stats_info_msg_update(buf);

        return ECORE_CALLBACK_PASS_ON;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
tools_set(void)
{
   tools_init(base_layout_get());
   base_tools_set(tools_live_view_get(),
                  tools_text_editor_get());
   tools_update();
}

static void
live_edit_set(void)
{
   Evas_Object *fixed_bar = live_edit_init(base_layout_get());
   base_live_edit_fixed_bar_set(fixed_bar);
}

static Eina_Bool
args_dispatch(int argc, char **argv,
              char *edc_path, char *edj_path, char *workspace_path,
              Eina_List **img_path, Eina_List **snd_path,
              Eina_List **fnt_path, Eina_List **dat_path,
              Eina_Bool *default_edc, Eina_Bool *template,
              int path_size)
{

   Eina_List *id = NULL;
   Eina_List *fd = NULL;
   Eina_List *sd = NULL;
   Eina_List *dd = NULL;
   Eina_List *wd = NULL;

   Eina_Bool quit = EINA_FALSE;
   Eina_Bool help = EINA_FALSE;

   //No arguments. set defaults
   if (argc == 1) goto defaults;

   static const Ecore_Getopt optdesc = {
     PACKAGE_NAME,
     ENVENTOR_USAGE,
     VERSION,
     ENVENTOR_COPYRIGHT,
     ENVENTOR_LICENSE,
     ENVENTOR_INFO,
     EINA_TRUE,
       {
          ECORE_GETOPT_STORE_TRUE('t', "to", "Open template menu"),
          ECORE_GETOPT_APPEND_METAVAR('i', "id", "Images path",
                                      "path", ECORE_GETOPT_TYPE_STR),
          ECORE_GETOPT_APPEND_METAVAR('s', "sd", "Sounds path",
                                       "path", ECORE_GETOPT_TYPE_STR),
          ECORE_GETOPT_APPEND_METAVAR('f', "fd", "Fonts path",
                                      "path", ECORE_GETOPT_TYPE_STR),
          ECORE_GETOPT_APPEND_METAVAR('d', "dd", "Data path",
                                      "path", ECORE_GETOPT_TYPE_STR),
          ECORE_GETOPT_APPEND_METAVAR('w', "wd", "Workspace path",
                                      "path", ECORE_GETOPT_TYPE_STR),
          ECORE_GETOPT_VERSION('v', "version"),
          ECORE_GETOPT_COPYRIGHT('c', "copyright"),
          ECORE_GETOPT_LICENSE('l', "license"),
          ECORE_GETOPT_HELP('h', "help"),
          ECORE_GETOPT_SENTINEL
       }
   };

   Ecore_Getopt_Value values[] = {
      ECORE_GETOPT_VALUE_BOOL(*template),
      ECORE_GETOPT_VALUE_LIST(id),
      ECORE_GETOPT_VALUE_LIST(sd),
      ECORE_GETOPT_VALUE_LIST(fd),
      ECORE_GETOPT_VALUE_LIST(dd),
      ECORE_GETOPT_VALUE_LIST(wd),
      ECORE_GETOPT_VALUE_BOOL(quit),
      ECORE_GETOPT_VALUE_BOOL(quit),
      ECORE_GETOPT_VALUE_BOOL(quit),
      ECORE_GETOPT_VALUE_BOOL(help),
      ECORE_GETOPT_VALUE_NONE
   };

   //edc path
   int i = 0;
   for (; i < argc; i++)
     {
        if (strstr(argv[i], ".edc"))
          {
             snprintf(edc_path, path_size, "%s", argv[i]);
             *default_edc = EINA_FALSE;
          }
        else if (strstr(argv[i], ".edj"))
          {
             snprintf(edj_path, path_size, "%s", argv[i]);
          }
     }

   if ((ecore_getopt_parse(&optdesc, values, argc, argv) < 0) || quit)
     return EINA_FALSE;

   if (help)
     {
        fprintf(stdout, ENVENTOR_HELP_EXAMPLES);
        return EINA_FALSE;
     }

defaults:
   if (*default_edc)
     {
        Eina_Tmpstr *tmp_path;
        eina_file_mkstemp(DEFAULT_EDC_FORMAT, &tmp_path);
        snprintf(edc_path, path_size, "%s", (const char *)tmp_path);
        eina_tmpstr_del(tmp_path);
     }
   else
     {
        //Do not overwrite existing EDC file with template code.
        if (*template && ecore_file_exists(edc_path))
          {
             fprintf(stdout, "\"%s\" already exists! Please input another file "
                     "name with \"-t\" option.\n", edc_path);
             return EINA_FALSE;
          }
     }

     char *s = NULL;
     EINA_LIST_FREE(id, s)
       {
          *img_path = eina_list_append(*img_path, eina_stringshare_add(s));
          free(s);
       }
     EINA_LIST_FREE(sd, s)
       {
          *snd_path = eina_list_append(*snd_path, eina_stringshare_add(s));
          free(s);
       }
     EINA_LIST_FREE(fd, s)
       {
          *fnt_path = eina_list_append(*fnt_path, eina_stringshare_add(s));
          free(s);
       }
     EINA_LIST_FREE(dd, s)
       {
          *dat_path = eina_list_append(*dat_path, eina_stringshare_add(s));
          free(s);
       }
     if (wd)
       {
          sprintf(workspace_path, "%s", (char *)eina_list_data_get(wd));
          EINA_LIST_FREE(wd, s) free(s);
       }
     else
       {
          sprintf(workspace_path, ".");
       }

     return EINA_TRUE;
}

static Eina_Bool
config_data_set(int argc, char **argv, Eina_Bool *default_edc,
                Eina_Bool *template)
{
   char edc_path[PATH_MAX] = { 0, };
   char edj_path[PATH_MAX] = { 0, };
   char workspace_path[PATH_MAX] = { 0, };
   Eina_List *img_path = NULL;
   Eina_List *snd_path = NULL;
   Eina_List *fnt_path = NULL;
   Eina_List *dat_path = NULL;
   if (!args_dispatch(argc, argv, edc_path, edj_path, workspace_path,
                      &img_path, &snd_path, &fnt_path, &dat_path,
                      default_edc, template, PATH_MAX))
     return EINA_FALSE;
   if (!config_init(edc_path, edj_path, workspace_path,
                    img_path, snd_path, fnt_path, dat_path))
     return EINA_FALSE;
   config_update_cb_set(config_update_cb, NULL);

   return EINA_TRUE;
}

static void
elm_setup()
{
   elm_need_efreet();
   elm_config_profile_set("standard");

   /* Recover the scale since it will be reset by
      elm_config_profile_set() */
   char *scale = getenv("ELM_SCALE");
   if (scale) elm_config_scale_set(atof(scale));

   /* Recover the current engine since it will be reset by
      elm_config_profile_set() */
   char *engine = getenv("ELM_ACCEL");
   if (engine && !strncmp(engine, "hw", strlen("hw")))
     elm_config_accel_preference_set("hw");

   elm_config_tooltip_delay_set(0.5);
   elm_config_focus_highlight_clip_disabled_set(EINA_FALSE);
   elm_config_scroll_bounce_enabled_set(EINA_FALSE);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_compile_lib_dir_set(PACKAGE_LIB_DIR);
   elm_app_info_set(main, "enventor", "images/enventor_logo.png");

   snprintf(EDJE_PATH, sizeof(EDJE_PATH), "%s/themes/enventor.edj",
            elm_app_data_dir_get());
   elm_theme_extension_add(NULL, EDJE_PATH);
}

static void
enventor_cursor_line_changed_cb(void *data EINA_UNUSED,
                                Enventor_Object *obj EINA_UNUSED,
                                void *event_info)
{
   Enventor_Cursor_Line *cur_line = (Enventor_Cursor_Line *)event_info;
   stats_line_num_update(cur_line->cur_line, cur_line->max_line);
}

static void
enventor_cursor_group_changed_cb(void *data EINA_UNUSED, Enventor_Object *obj,
                                 void *event_info)
{
   const char *group_name = event_info;
   stats_edc_group_update(group_name);
   base_edc_navigator_group_update();

   //View Size
   int w, h;
   enventor_object_live_view_size_get(obj, &w, &h);

   //Set default view size if this view has no size.
   if ((w == 0) && (h == 0))
     {
        config_view_size_get(&w, &h);
        enventor_object_live_view_size_set(obj, w, h);
     }
   stats_view_size_update(w, h);

   //View Scale
   double scale = enventor_object_live_view_scale_get(obj);
   stats_view_scale_update(scale);
}

static void
enventor_compile_error_cb(void *data EINA_UNUSED,
                          Enventor_Object *obj EINA_UNUSED,
                          void *event_info)
{
   const char *msg = event_info;
   base_error_msg_set(msg);
}

static void
enventor_live_view_resized_cb(void *data EINA_UNUSED,
                              Enventor_Object *obj EINA_UNUSED,
                              void *event_info)
{
   Enventor_Live_View_Size *size = event_info;
   stats_view_size_update(size->w, size->h);
}

static void
enventor_live_view_loaded_cb(void *data EINA_UNUSED, Enventor_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   base_edc_navigator_group_update();
}

static void
enventor_live_view_cursor_moved_cb(void *data EINA_UNUSED,
                                   Enventor_Object *obj EINA_UNUSED,
                                   void *event_info)
{
   Enventor_Live_View_Cursor *cursor = event_info;
   stats_cursor_pos_update(cursor->x, cursor->y, cursor->relx, cursor->rely);
   base_edc_navigator_deselect();
}

static void
enventor_ctxpopup_activated_cb(void *data EINA_UNUSED,
                               Enventor_Object *obj EINA_UNUSED,
                               void *event_info)
{
   Enventor_Ctxpopup_Type type = (Enventor_Ctxpopup_Type) event_info;

   if (type == ENVENTOR_CTXPOPUP_TYPE_SLIDER)
     stats_info_msg_update("Mouse wheel: Change values elaborately.  "
                           "Backspace: Reset values.");
   else if (type == ENVENTOR_CTXPOPUP_TYPE_TOGGLE)
     stats_info_msg_update("Backspace: Reset values.");
}

static void
enventor_ctxpopup_changed_cb(void *data, Enventor_Object *obj EINA_UNUSED,
                             void *event_info)
{
   app_data *ad = data;
   Enventor_Item *it = event_info;

   if (!enventor_item_modified_get(it)) return;

   //FIXME: Probably, this lazy stuff is broken.
   //These data should be up to items.
   if (ad->on_saving)
     {
        ad->lazy_save = EINA_TRUE;
        return;
     }
   ad->on_saving = EINA_TRUE;

   enventor_item_file_save(it, NULL);
}

static void
enventor_live_view_updated_cb(void *data, Enventor_Object *obj EINA_UNUSED,
                              void *event_info)
{
   app_data *ad = data;

   Enventor_Item *it = event_info;

   //FIXME: Probably, this lazy stuff is broken.
   //These data should be up to items.
   if (ad->lazy_save && enventor_item_modified_get(it))
     {
        enventor_item_file_save(it, NULL);
        ad->lazy_save = EINA_FALSE;
     }
   else
     {
        ad->lazy_save = EINA_FALSE;
        ad->on_saving = EINA_FALSE;
     }
   base_edc_navigator_group_update();
}

static void
enventor_ctxpopup_dismissed_cb(void *data EINA_UNUSED, Enventor_Object *obj,
                               void *event_info EINA_UNUSED)
{
   if (menu_activated_get() > 0)
     efl_ui_focus_object_focus_set(obj, EINA_FALSE);
}

static void
enventor_file_open_requested_cb(void *data EINA_UNUSED, Enventor_Object *obj EINA_UNUSED,
                                void *event_info)
{
   const char *file_name = event_info;
   file_mgr_file_open(file_name);
}

static void
enventor_focused_cb(void *data EINA_UNUSED, Enventor_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   //FIXME: Get the all modified file list.
   if (file_mgr_edc_modified_get()) file_mgr_warning_open();
}

static void
enventor_mouse_down_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                       Enventor_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   base_edc_navigator_deselect();
}

static Enventor_Object *
enventor_setup(app_data *ad)
{
   Enventor_Object *enventor = enventor_object_add(base_layout_get());
   evas_object_smart_callback_add(enventor, "max_line,changed",
                                  enventor_cursor_line_changed_cb, ad);
   evas_object_smart_callback_add(enventor, "cursor,line,changed",
                                  enventor_cursor_line_changed_cb, ad);
   evas_object_smart_callback_add(enventor, "cursor,group,changed",
                                  enventor_cursor_group_changed_cb, ad);
   evas_object_smart_callback_add(enventor, "compile,error",
                                  enventor_compile_error_cb, ad);
   evas_object_smart_callback_add(enventor, "live_view,loaded",
                                  enventor_live_view_loaded_cb, ad);
   evas_object_smart_callback_add(enventor, "live_view,cursor,moved",
                                  enventor_live_view_cursor_moved_cb, ad);
   evas_object_smart_callback_add(enventor, "live_view,resized",
                                  enventor_live_view_resized_cb, ad);
   evas_object_smart_callback_add(enventor, "live_view,updated",
                                  enventor_live_view_updated_cb, ad);
   evas_object_smart_callback_add(enventor, "ctxpopup,activated",
                                  enventor_ctxpopup_activated_cb, ad);
   evas_object_smart_callback_add(enventor, "ctxpopup,changed",
                                  enventor_ctxpopup_changed_cb, ad);
   evas_object_smart_callback_add(enventor, "ctxpopup,dismissed",
                                  enventor_ctxpopup_dismissed_cb, ad);
   evas_object_smart_callback_add(enventor, "focused",
                                  enventor_focused_cb, ad);
   evas_object_smart_callback_add(enventor, "file,open,requested",
                                  enventor_file_open_requested_cb, ad);
   evas_object_event_callback_add(enventor, EVAS_CALLBACK_MOUSE_DOWN,
                                  enventor_mouse_down_cb, ad);

   evas_object_size_hint_expand_set(enventor, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(enventor, EVAS_HINT_FILL, EVAS_HINT_FILL);

   enventor_common_setup(enventor);

   base_enventor_set(enventor);

   return enventor;
}

static Eina_Bool
alt_func(Evas_Event_Key_Down *event)
{
   if (!evas_key_modifier_is_set(event->modifiers, "Alt"))
     return EINA_FALSE;

   //Full Edit View
   if (!strcmp(event->key, "Left"))
     {
        base_live_view_full_view();
        return EINA_TRUE;
     }
   //Full Live View
   if (!strcmp(event->key, "Right"))
     {
        base_enventor_full_view();
        return EINA_TRUE;
     }
   //Full Console View
   if (!strcmp(event->key, "Up"))
     {
        base_console_full_view();
        return EINA_TRUE;
     }
   //Full Editors View
   if (!strcmp(event->key, "Down"))
     {
        base_editors_full_view();
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

static Eina_Bool
ctrl_func(Evas_Event_Key_Down *event)
{
   if (!evas_key_modifier_is_set(event->modifiers, "Control"))
     return EINA_FALSE;

   //Save
   if (!strcmp(event->key, "s") || !strcmp(event->key, "S"))
     {
        file_mgr_edc_save();
        return EINA_TRUE;
     }
  //Delete Line
   if (!strcmp(event->key, "d") || !strcmp(event->key, "D"))
     {
        enventor_item_line_delete(file_mgr_focused_item_get());
        return EINA_TRUE;
     }
   //Find/Replace
   if (!strcmp(event->key, "f") || !strcmp(event->key, "F"))
     {
        live_edit_cancel(EINA_FALSE);
        search_open();
        return EINA_TRUE;
     }
   //Goto Line
   if (!strcmp(event->key, "l") || !strcmp(event->key, "L"))
     {
        live_edit_cancel(EINA_FALSE);
        goto_open();
        return EINA_TRUE;
     }
   //Part Highlight
   if (!strcmp(event->key, "h") || !strcmp(event->key, "H"))
     {
        tools_highlight_update(EINA_TRUE);
        return EINA_TRUE;
     }
   //Dummy Object
   if (!strcmp(event->key, "u") || !strcmp(event->key, "U"))
     {
        tools_dummy_update(EINA_TRUE);
        return EINA_TRUE;
     }
   //Wireframes
   if (!strcmp(event->key, "w") || !strcmp(event->key, "W"))
     {
        tools_wireframes_update(EINA_TRUE);
        return EINA_TRUE;
     }
   //Mirror Mode
   if (!strcmp(event->key, "m") || !strcmp(event->key, "M"))
     {
        tools_mirror_mode_update(EINA_TRUE);
        return EINA_TRUE;
     }
   //Template Code
   if (!strcmp(event->key, "t") || !strcmp(event->key, "T"))
     {
        tools_template_insert();
        return EINA_TRUE;
     }

   if (!strcmp(event->key, "space"))
     {
        enventor_object_auto_complete_list_show(base_enventor_get());
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

static void
keygrabber_key_down_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                       Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   //Main Menu
   if (!strcmp(ev->key, "Escape"))
     {
        if (stats_ctxpopup_dismiss()) return;

        if (live_edit_cancel(EINA_TRUE))
          {
             efl_ui_focus_object_focus_set(base_enventor_get(), EINA_TRUE);
             return;
          }
        if (file_mgr_warning_is_opened())
          {
             file_mgr_warning_close();
             return;
          }
        if (enventor_object_ctxpopup_visible_get(base_enventor_get()))
          {
             enventor_object_ctxpopup_dismiss(base_enventor_get());
             return;
          }

        menu_toggle();
        return;
     }

   if (menu_activated_get() > 0) return;
   if (file_mgr_warning_is_opened()) return;

   enventor_object_ctxpopup_dismiss(base_enventor_get());
   stats_ctxpopup_dismiss();

   if (ctrl_func(ev)) return;
   if (alt_func(ev)) return;

   //About
   if (!strcmp(ev->key, "F1"))
     {
        enventor_object_ctxpopup_dismiss(base_enventor_get());
        live_edit_cancel(EINA_FALSE);
        help_open();
        return;
     }
   //Set Main EDC
   if (!strcmp(ev->key, "F2"))
     {
        file_browser_selected_file_main_set();
        main_edc_update(EINA_FALSE);
        return;
     }
   //Revert Quick Jump
   if (!strcmp(ev->key, "F4"))
     {
        if (!file_mgr_file_backward())
          stats_info_msg_update("No files go back.");
        return;
     }
   //Keyword Reference
   if (!strcmp(ev->key, "F5"))
     {
        enventor_object_ctxpopup_dismiss(base_enventor_get());
        enventor_object_keyword_reference_show(base_enventor_get());
        return;
     }
   //Tools
   if (!strcmp(ev->key, "F7"))
     {
        enventor_object_ctxpopup_dismiss(base_enventor_get());
        base_tools_toggle(EINA_TRUE);
        return;
     }
   //Status Bar
   if (!strcmp(ev->key, "F8"))
     {
        enventor_object_ctxpopup_dismiss(base_enventor_get());
        base_statusbar_toggle(EINA_TRUE);
        return;
     }
   //File Browser
   if (!strcmp(ev->key, "F9"))
     {
        enventor_object_ctxpopup_dismiss(base_enventor_get());
        tools_file_browser_update(EINA_TRUE);
        return;
     }
   //EDC Navigator
   if (!strcmp(ev->key, "F10"))
     {
        enventor_object_ctxpopup_dismiss(base_enventor_get());
        tools_edc_navigator_update(EINA_TRUE);
        return;
     }
   //File Tab
   if (!strcmp(ev->key, "F11"))
     {
        enventor_object_ctxpopup_dismiss(base_enventor_get());
        tools_file_tab_update(EINA_TRUE);
        return;
     }
   //Setting
   if (!strcmp(ev->key, "F12"))
     {
        live_edit_cancel(EINA_FALSE);
        enventor_object_ctxpopup_dismiss(base_enventor_get());
        menu_setting();
        return;
     }
}

static void
statusbar_set()
{
   Evas_Object *obj = stats_init(base_layout_get());
   elm_object_part_content_set(base_layout_get(), "elm.swallow.statusbar", obj);
   base_statusbar_toggle(EINA_FALSE);
}

static void
keygrabber_init(app_data *ad)
{
   Evas *e = evas_object_evas_get(base_enventor_get());
   ad->keygrabber = base_enventor_get();
   evas_object_event_callback_add(ad->keygrabber, EVAS_CALLBACK_KEY_DOWN,
                                  keygrabber_key_down_cb, NULL);
#define GRAB_ADD(key, modifier) \
   if (!evas_object_key_grab(ad->keygrabber, (key), (modifier), 0, EINA_TRUE)) \
     EINA_LOG_ERR(_("Failed to grab key - %s"), (key))

   GRAB_ADD("Escape", 0);
   GRAB_ADD("F1", 0);
   GRAB_ADD("F2", 0);
   GRAB_ADD("F3", 0);
   GRAB_ADD("F4", 0);
   GRAB_ADD("F5", 0);
   GRAB_ADD("F6", 0);
   GRAB_ADD("F7", 0);
   GRAB_ADD("F8", 0);
   GRAB_ADD("F9", 0);
   GRAB_ADD("F10", 0);
   GRAB_ADD("F11", 0);
   GRAB_ADD("F12", 0);

   Evas_Modifier_Mask modifier;

   //Ctrl Modifier Mask
   modifier = evas_key_modifier_mask_get(e, "Control");
   GRAB_ADD("s", modifier);
   GRAB_ADD("S", modifier);
   GRAB_ADD("d", modifier);
   GRAB_ADD("D", modifier);
   GRAB_ADD("f", modifier);
   GRAB_ADD("F", modifier);
   GRAB_ADD("l", modifier);
   GRAB_ADD("L", modifier);
   GRAB_ADD("h", modifier);
   GRAB_ADD("H", modifier);
   GRAB_ADD("w", modifier);
   GRAB_ADD("W", modifier);
   GRAB_ADD("m", modifier);
   GRAB_ADD("M", modifier);
   GRAB_ADD("t", modifier);
   GRAB_ADD("T", modifier);
   GRAB_ADD("u", modifier);
   GRAB_ADD("U", modifier);
   GRAB_ADD("p", modifier);
   GRAB_ADD("P", modifier);
   GRAB_ADD("e", modifier);
   GRAB_ADD("E", modifier);
   GRAB_ADD("space", modifier);

   //Alt
   modifier = evas_key_modifier_mask_get(e, "Alt");
   GRAB_ADD("Left", modifier);
   GRAB_ADD("Right", modifier);
   GRAB_ADD("Up", modifier);
   GRAB_ADD("Down", modifier);
}

static Eina_Bool
enventor_lock_create(void)
{
   //Tempoary Folder
   const char *tmpdir = eina_environment_tmp_get();

   //Exception 1
   if (!tmpdir)
     {
        EINA_LOG_ERR("No temporary directory?!");
        return EINA_TRUE;
     }

   //Exception 2
   //Create temp directory if it doesn't exist.
   if (!ecore_file_is_dir(tmpdir))
     {
        Eina_Bool success = ecore_file_mkdir(tmpdir);
        if (!success)
          {
             EINA_LOG_ERR(_("Cannot create temporary diretory \"%s\""), tmpdir);
             return EINA_TRUE;
          }
     }

   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/"ENVENTOR_LOCK_FILE, tmpdir);

   //Is there any other Enventor instance?
   if (ecore_file_exists(buf))
     {
#ifdef _WIN32
        hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, ENVENTOR_NAME);
        if (hMutex)
          {
             fprintf(stdout, "Enventor program is already running!\n\n"
                     "If you are really stuck in launching enventor due to "
                     "this,\nTry removing a file \"%s\"\n", buf);
             return EINA_FALSE;
          }
#else
        fprintf(stdout, "Enventor program is already running!\n\n"
                "If you are really stuck in launching enventor due to "
                "this,\nTry removing a file \"%s\"\n", buf);
        return EINA_FALSE;
#endif
     }

   //Ok, this is an unique instance!
#ifdef _WIN32
   hMutex = CreateMutex(NULL, FALSE, ENVENTOR_NAME);

   HANDLE handle = CreateFile(buf, GENERIC_READ, 0, NULL, CREATE_NEW,
                              FILE_FLAG_DELETE_ON_CLOSE, 0);
   if (INVALID_HANDLE_VALUE == handle)
     {
        EINA_LOG_ERR("Failed to open file \"%s\"", buf);
        return EINA_TRUE;
     }
#else
   FILE *fp = fopen(buf, "w");
   if (!fp)
     {
        EINA_LOG_ERR("Failed to open file \"%s\"", buf);
        return EINA_TRUE;
     }
   fputs("x", fp);
   fclose(fp);
#endif

   own_lock = EINA_TRUE;

   return EINA_TRUE;
}

static void
enventor_lock_remove()
{
   //You are not the owner of the lock.
   if (!own_lock) return;

#ifdef _WIN32
   if (hMutex)
     {
        CloseHandle(hMutex);
        hMutex = NULL;
     }
#else
   //Tempoary Folder
   const char *tmpdir = eina_environment_tmp_get();
   if (!tmpdir)
     {
        EINA_LOG_ERR("No temporary directory?!");
        return;
     }

   if (!ecore_file_is_dir(tmpdir))
     {
        EINA_LOG_ERR("Cannot access to temporary directory?! = %s", tmpdir);
        return;
     }

   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/"ENVENTOR_LOCK_FILE, tmpdir);

   //Just in case.
   if (!ecore_file_exists(buf))
     {
        EINA_LOG_ERR("No enventor lock file?! = %s", buf);
        return;
     }

   ecore_file_remove(buf);
#endif
}

#ifndef _WIN32
static void
crash_handler(int x EINA_UNUSED, siginfo_t *info EINA_UNUSED,
              void *data EINA_UNUSED)
{
   EINA_LOG_ERR("Eeeek! Eventor is terminated abnormally!");
   enventor_lock_remove();
}
#endif

static void
sigaction_setup(void)
{
#ifndef _WIN32
   //Just in case, if you are debugging using gdb,
   //you can send signals like this. "handle SIGABRT pass"
   //Use this for remove the enventor lock.

   struct sigaction action;

   memset(&action, 0x0, sizeof(action));
   action.sa_handler = SIG_DFL;
   action.sa_sigaction = crash_handler;
   action.sa_flags = SA_NODEFER  | SA_RESETHAND | SA_SIGINFO;
   sigemptyset(&action.sa_mask);

   //Bus error (bad memory access)
   sigaction(SIGBUS, &action, NULL);

   //Floating pointer exception
   sigaction(SIGFPE, &action, NULL);

   //Illegal Instruction
   sigaction(SIGILL, &action, NULL);

   //Invalid memory reference
   sigaction(SIGSEGV, &action, NULL);

   //Kill
   sigaction(SIGKILL, &action, NULL);

   //Abort
   sigaction(SIGABRT, &action, NULL);

   //Stop
   sigaction(SIGSTOP, &action, NULL);

   //Interrupt from keyboard
   //sigaction(SIGINT, &action, NULL);
#else
   (void) 0;
#endif
}

static Eina_Bool
init(app_data *ad, int argc, char **argv)
{
#ifdef ENABLE_NLS
      setlocale(LC_ALL, "");
      bindtextdomain(PACKAGE, LOCALE_DIR);
      textdomain(PACKAGE);
#endif /* set locale */

   elm_setup();
   enventor_init(argc, argv);

   if (!enventor_lock_create()) return EINA_FALSE;

   sigaction_setup();

   Eina_Bool template = EINA_FALSE;
   Eina_Bool default_edc = EINA_TRUE;
   if (!config_data_set(argc, argv, &default_edc, &template))
     return EINA_FALSE;

   if (!template) newfile_default_set(default_edc);
   base_gui_init();
   statusbar_set();
   Enventor_Object *enventor = enventor_setup(ad);
   file_mgr_init();

   file_mgr_main_file_set(config_input_path_get());
   base_live_view_set(enventor_object_live_view_get(enventor));

   tools_set();
   live_edit_set();

   base_gui_show();

   //Guarantee Enventor editor has focus.
   efl_ui_focus_object_focus_set(enventor, EINA_TRUE);

   menu_init();

   if (template) menu_edc_new(EINA_TRUE);

   //Initialize syntax color.
   syntax_color_init(enventor);
   syntax_color_update(enventor);

   keygrabber_init(ad);

   ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL, main_mouse_wheel_cb, ad);

   return EINA_TRUE;
}

static void
term(void)
{
   menu_term();
   live_edit_term();
   stats_term();
   tools_term();
   base_gui_term();
   file_mgr_term();
   config_term();
   enventor_lock_remove();
   enventor_shutdown();
}

EAPI_MAIN
int elm_main(int argc, char **argv)
{
   app_data ad;
   memset(&ad, 0x00, sizeof(ad));

   if (!init(&ad, argc, argv))
     {
        term();
        return 0;
     }

   elm_run();

   term();

   return 0;
}
ELM_MAIN()
