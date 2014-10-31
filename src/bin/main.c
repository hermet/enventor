#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define ENVENTOR_BETA_API_SUPPORT 1

#include <Eio.h>
#include <Enventor.h>
#include "common.h"

typedef struct app_s
{
   Evas_Object *enventor;

   Eina_Bool ctrl_pressed : 1;
   Eina_Bool shift_pressed : 1;
   Eina_Bool template_new : 1;
} app_data;

int main(int argc, char **argv);

static Eina_Bool
main_key_up_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   app_data *ad = data;

   if (!strcmp("Control_L", event->key))
     ad->ctrl_pressed = EINA_FALSE;
   else if (!strcmp("Shift_L", event->key))
     ad->shift_pressed = EINA_FALSE;

   return ECORE_CALLBACK_PASS_ON;
}

void
auto_comp_toggle(app_data *ad)
{
   Eina_Bool toggle = !config_auto_complete_get();
   enventor_object_auto_complete_set(ad->enventor, toggle);
   if (toggle) stats_info_msg_update("Auto Completion Enabled.");
   else stats_info_msg_update("Auto Completion Disabled.");
   config_auto_complete_set(toggle);
}

static void
auto_indent_toggle(app_data *ad)
{
   Eina_Bool toggle = !config_auto_indent_get();
   enventor_object_auto_indent_set(ad->enventor, toggle);
   if (toggle) stats_info_msg_update("Auto Indentation Enabled.");
   else stats_info_msg_update("Auto Indentation Disabled.");
   config_auto_indent_set(toggle);
}

static Eina_Bool
template_insert_patch(app_data *ad, const char *key)
{
   Edje_Part_Type part_type;
   if (config_live_edit_get())
     {
        stats_info_msg_update("Insertion of template code is disabled "
                              "while in Live Edit mode");
        return ECORE_CALLBACK_DONE;
     }

   if (!strcmp(key, "a") || !strcmp(key, "A"))
     part_type = EDJE_PART_TYPE_TABLE;
   else if (!strcmp(key, "b") || !strcmp(key, "B"))
     part_type = EDJE_PART_TYPE_TEXTBLOCK;
   else if (!strcmp(key, "e") || !strcmp(key, "E"))
     part_type = EDJE_PART_TYPE_EXTERNAL;
   else if (!strcmp(key, "g") || !strcmp(key, "G"))
     part_type = EDJE_PART_TYPE_GRADIENT;
   else if (!strcmp(key, "i") || !strcmp(key, "I"))
     part_type = EDJE_PART_TYPE_IMAGE;
   else if (!strcmp(key, "o") || !strcmp(key, "O"))
     part_type = EDJE_PART_TYPE_GROUP;
   else if (!strcmp(key, "p") || !strcmp(key, "P"))
     part_type = EDJE_PART_TYPE_PROXY;
   else if (!strcmp(key, "r") || !strcmp(key, "R"))
     part_type = EDJE_PART_TYPE_RECTANGLE;
   else if (!strcmp(key, "t") || !strcmp(key, "T"))
     part_type = EDJE_PART_TYPE_TEXT;
   else if (!strcmp(key, "s") || !strcmp(key, "S"))
     part_type = EDJE_PART_TYPE_SPACER;
   else if (!strcmp(key, "w") || !strcmp(key, "W"))
     part_type = EDJE_PART_TYPE_SWALLOW;
   else if (!strcmp(key, "x") || !strcmp(key, "X"))
     part_type = EDJE_PART_TYPE_BOX;
   else
     part_type = EDJE_PART_TYPE_NONE;

   char syntax[12];
   if (enventor_object_template_part_insert(ad->enventor, part_type, REL1_X,
                                            REL1_Y, REL2_X, REL2_Y, syntax,
                                            sizeof(syntax)))
     {
        char msg[64];
        snprintf(msg, sizeof(msg), "Template code inserted, (%s)", syntax);
        stats_info_msg_update(msg);
        enventor_object_save(ad->enventor, config_edc_path_get());
     }
   else
     {
        stats_info_msg_update("Can't insert template code here. Move the "
                              "cursor inside the \"Collections,Images,Parts,"
                              "Part,Programs\" scope.");
     }
   return ECORE_CALLBACK_DONE;
}

static void
config_update_cb(void *data)
{
   app_data *ad = data;
   Evas_Object *enventor = ad->enventor;

   Eina_List *list = eina_list_append(NULL, config_edj_path_get());
   enventor_object_path_set(enventor, ENVENTOR_OUT_EDJ, list);
   eina_list_free(list);

   enventor_object_path_set(enventor, ENVENTOR_RES_IMAGE,
                            config_edc_img_path_list_get());
   enventor_object_path_set(enventor, ENVENTOR_RES_SOUND,
                            config_edc_snd_path_list_get());
   enventor_object_path_set(enventor, ENVENTOR_RES_FONT,
                            config_edc_fnt_path_list_get());
   enventor_object_path_set(enventor, ENVENTOR_RES_DATA,
                            config_edc_dat_path_list_get());
   enventor_object_font_scale_set(enventor, config_font_scale_get());
   enventor_object_linenumber_set(enventor, config_linenumber_get());
   enventor_object_dummy_swallow_set(enventor, config_dummy_swallow_get());
   enventor_object_part_highlight_set(enventor, config_part_highlight_get());
   enventor_object_live_view_scale_set(enventor, config_view_scale_get());

   base_tools_toggle(EINA_FALSE);
   base_statusbar_toggle(EINA_FALSE);

   //previous build was failed, Need to rebuild then reload the edj.
#if 0
   if (edj_mgr_reload_need_get())
     {
        build_edc();
        edj_mgr_clear();
        //edc_view_set(stats_group_name_get());
     }
#endif
}

static Eina_Bool
main_mouse_wheel_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Mouse_Wheel *event = ev;
   app_data *ad = data;
   Evas_Coord x, y, w, h;

   if (!ad->ctrl_pressed) return ECORE_CALLBACK_PASS_ON;

   //View Scale
   Evas_Object *view = enventor_object_live_view_get(ad->enventor);
   evas_object_geometry_get(view, &x, &y, &w, &h);

   if ((event->x >= x) && (event->x <= (x + w)) &&
       (event->y >= y) && (event->y <= (y + h)))
     {
        double scale = config_view_scale_get();

        if (event->z < 0) scale += 0.1;
        else scale -= 0.1;

        config_view_scale_set(scale);
        scale = config_view_scale_get();
        enventor_object_live_view_scale_set(ad->enventor, scale);

        char buf[256];
        snprintf(buf, sizeof(buf), "View Scale: %2.2fx", scale);
        stats_info_msg_update(buf);

        return ECORE_CALLBACK_PASS_ON;
     }

   //Font Size
   evas_object_geometry_get(ad->enventor, &x, &y, &w, &h);

   if ((event->x >= x) && (event->x <= (x + w)) &&
       (event->y >= y) && (event->y <= (y + h)))
     {
        if (event->z < 0)
          {
             config_font_scale_set(config_font_scale_get() + 0.1f);
             enventor_object_font_scale_set(ad->enventor,
                                            config_font_scale_get());
          }
        else
          {
             config_font_scale_set(config_font_scale_get() - 0.1f);
             enventor_object_font_scale_set(ad->enventor,
                                            config_font_scale_get());
          }

        char buf[128];
        snprintf(buf, sizeof(buf), "Font Size: %1.1fx",
                 config_font_scale_get());
        stats_info_msg_update(buf);

        return ECORE_CALLBACK_PASS_ON;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
tools_set(Evas_Object *enventor)
{
   Evas_Object *tools = tools_create(base_layout_get(), enventor);
   base_tools_set(tools);
}

static void
args_dispatch(int argc, char **argv, char *edc_path, char *img_path,
              char *snd_path, char *fnt_path, char *dat_path,
              Eina_Bool *template_new)
{
   Eina_Bool default_edc = EINA_TRUE;
   *template_new = EINA_FALSE;

   //No arguments. set defaults
   if (argc == 1) goto defaults;

   //Help
   if ((argc >=2 ) && !strcmp(argv[1], "--help"))
     {
        fprintf(stdout, "Usage: enventor [input file] [-to] [-id image path]"
                "[-sd sound path] [-fd font path] [-dd data path]\n"
                "\n"
                "-input file = EDC file to open. If input file is skipped, "
                "Enventor will open a default template code with a temporary "
                "file.\n"
                "-to = Open template menu when you launch Enventor\n"
                "-id = image resources, that edc includes, path\n"
                "-sd = sound resources, that edc includes, path\n"
                "-fd = font resources, that edc includes, path\n"
                "-dd = data resources, that edc includes, path\n"
                "\n"
                "Examples of Enventor command line usage:\n"
                "$ enventor\n"
                "$ enventor -to\n"
                "$ enventor newfile.edc -to\n"
                "$ enventor sample.edc -id ./images -sd ./sounds\n"
                "\n");
        exit(0);
     }

   //edc path
   if ((argc >= 2) && strstr(argv[1], ".edc"))
     {
        sprintf(edc_path, "%s", argv[1]);
        default_edc = EINA_FALSE;
     }
   else if ((argc >= 2) && !strcmp("-to", argv[1]))
     {
        *template_new = EINA_TRUE;
     }
   else goto defaults;

   //edc image path
   int cur_arg = 2;

   while (cur_arg < argc)
     {
        if (!strcmp("-to", argv[cur_arg]))
          {
             *template_new = EINA_TRUE;
             cur_arg++;
          }
        else if (!strcmp("-id", argv[cur_arg]))
          {
             sprintf(img_path, "%s", argv[cur_arg + 1]);
             cur_arg += 2;
          }
        else if (!strcmp("-sd", argv[cur_arg]))
          {
             sprintf(snd_path, "%s", argv[cur_arg + 1]);
             cur_arg += 2;
          }
        else if (!strcmp("-fd", argv[cur_arg]))
          {
             sprintf(fnt_path, "%s", argv[cur_arg + 1]);
             cur_arg += 2;
          }
        else if (!strcmp("-dd", argv[cur_arg]))
          {
             sprintf(dat_path, "%s", argv[cur_arg + 1]);
             cur_arg += 2;
          }
     }

defaults:
   if (default_edc) sprintf(edc_path, DEFAULT_EDC_PATH_FORMAT, getpid());
}

static void
config_data_set(app_data *ad, int argc, char **argv)
{
   char edc_path[PATH_MAX] = { 0, };
   char img_path[PATH_MAX] = { 0, };
   char snd_path[PATH_MAX] = { 0, };
   char fnt_path[PATH_MAX] = { 0, };
   char dat_path[PATH_MAX] = { 0, };
   Eina_Bool template_new = EINA_FALSE;

   args_dispatch(argc, argv, edc_path, img_path, snd_path, fnt_path, dat_path,
                 &template_new);
   config_init(edc_path, img_path, snd_path, fnt_path, dat_path);
   config_update_cb_set(config_update_cb, ad);
   ad->template_new = template_new;
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
      elm_config_profiel_set() */
   char *engine = getenv("ELM_ENGINE");
   if (engine && !strncmp(engine, "gl", strlen("gl")))
     elm_config_preferred_engine_set("opengl_x11");

   elm_config_focus_highlight_clip_disabled_set(EINA_FALSE);
   elm_config_scroll_bounce_enabled_set(EINA_FALSE);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_compile_lib_dir_set(PACKAGE_LIB_DIR);
   elm_app_info_set(main, "enventor", "images/logo.png");

   snprintf(EDJE_PATH, sizeof(EDJE_PATH), "%s/themes/enventor.edj",
            elm_app_data_dir_get());
   elm_theme_extension_add(NULL, EDJE_PATH);
}

static void
enventor_cursor_line_changed_cb(void *data EINA_UNUSED, Evas_Object *obj,
                                void *event_info)
{
   int linenumber = (int) event_info;
   stats_line_num_update(linenumber, enventor_object_max_line_get(obj));
}

static void
enventor_cursor_group_changed_cb(void *data EINA_UNUSED,
                                 Evas_Object *obj EINA_UNUSED,
                                 void *event_info)
{
   const char *group_name = event_info;
   stats_edc_group_update(group_name);
}

static void
enventor_compile_error_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                          void *event_info)
{
   const char *msg = event_info;
   base_error_msg_set(msg);
}

static void
enventor_live_view_resized_cb(void *data EINA_UNUSED,
                              Evas_Object *obj EINA_UNUSED,
                              void *event_info)
{
   if (!config_stats_bar_get()) return;
   Enventor_Live_View_Size *size = event_info;
   config_view_size_set(size->w, size->h);
   stats_view_size_update(size->w, size->h);
}

static void
enventor_live_view_cursor_moved_cb(void *data EINA_UNUSED,
                                   Evas_Object *obj EINA_UNUSED,
                                   void *event_info)
{
   if (!config_stats_bar_get()) return;
   Enventor_Live_View_Cursor *cursor = event_info;
   stats_cursor_pos_update(cursor->x, cursor->y, cursor->relx, cursor->rely);
}

static void
enventor_live_view_resize_cb(void *data EINA_UNUSED,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info)
{
   if (!config_stats_bar_get()) return;
   Enventor_Live_View_Cursor *cursor = event_info;
   stats_cursor_pos_update(cursor->x, cursor->y, cursor->relx, cursor->rely);
}

static void
enventor_program_run_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                        void *event_info)
{
   if (!config_stats_bar_get()) return;
   const char *program = event_info;
   char buf[256];
   snprintf(buf, sizeof(buf), "Program Run: \"%s\"", program);
   stats_info_msg_update(buf);
}

static void
enventor_ctxpopup_selected_cb(void *data EINA_UNUSED, Evas_Object *obj,
                              void *event_info EINA_UNUSED)
{
   EINA_LOG_ERR("!!");
   enventor_object_save(obj, config_edc_path_get());
}

static void
enventor_setup(app_data *ad)
{
   Evas_Object *enventor = enventor_object_add(base_layout_get());
   evas_object_smart_callback_add(enventor, "cursor,line,changed",
                                  enventor_cursor_line_changed_cb, ad);
   evas_object_smart_callback_add(enventor, "cursor,group,changed",
                                  enventor_cursor_group_changed_cb, ad);
   evas_object_smart_callback_add(enventor, "compile,error",
                                  enventor_compile_error_cb, ad);
   evas_object_smart_callback_add(enventor, "live_view,cursor,moved",
                                  enventor_live_view_cursor_moved_cb, ad);
   evas_object_smart_callback_add(enventor, "live_view,resized",
                                  enventor_live_view_resized_cb, ad);
   evas_object_smart_callback_add(enventor, "program,run",
                                  enventor_program_run_cb, ad);
   evas_object_smart_callback_add(enventor, "ctxpopup,selected",
                                  enventor_ctxpopup_selected_cb, ad);

   enventor_object_font_scale_set(enventor, config_font_scale_get());
   enventor_object_live_view_scale_set(enventor, config_view_scale_get());
   enventor_object_linenumber_set(enventor, config_linenumber_get());
   enventor_object_part_highlight_set(enventor, config_part_highlight_get());
   enventor_object_auto_indent_set(enventor, config_auto_indent_get());
   enventor_object_auto_complete_set(enventor, config_auto_complete_get());
   enventor_object_dummy_swallow_set(enventor, config_dummy_swallow_get());

   Eina_List *list = eina_list_append(NULL, config_edj_path_get());
   enventor_object_path_set(enventor, ENVENTOR_OUT_EDJ, list);
   eina_list_free(list);

   enventor_object_path_set(enventor, ENVENTOR_RES_IMAGE,
                            config_edc_img_path_list_get());
   enventor_object_path_set(enventor, ENVENTOR_RES_SOUND,
                            config_edc_snd_path_list_get());
   enventor_object_path_set(enventor, ENVENTOR_RES_FONT,
                            config_edc_fnt_path_list_get());
   enventor_object_path_set(enventor, ENVENTOR_RES_DATA,
                            config_edc_dat_path_list_get());
   enventor_object_file_set(enventor, config_edc_path_get());

   evas_object_size_hint_expand_set(enventor, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(enventor, EVAS_HINT_FILL, EVAS_HINT_FILL);

   base_enventor_set(enventor);
   base_title_set(config_edc_path_get());

   base_live_view_set(enventor_object_live_view_get(enventor));

   ad->enventor = enventor;
}

static void
edc_save(app_data *ad)
{
   char buf[PATH_MAX];

   if (enventor_object_save(ad->enventor, config_edc_path_get()))
     {
        if (config_stats_bar_get())
          {
             snprintf(buf, sizeof(buf), "File saved. \"%s\"",
                      config_edc_path_get());
          }
     }
   else
     {
        if (config_stats_bar_get())
          {
             snprintf(buf, sizeof(buf), "Already saved. \"%s\"",
                      config_edc_path_get());
          }

     }
   stats_info_msg_update(buf);
}

static void
part_highlight_toggle(app_data *ad)
{
   config_part_highlight_set(!config_part_highlight_get());
   enventor_object_part_highlight_set(ad->enventor,
                                      config_part_highlight_get());
   if (config_part_highlight_get())
     stats_info_msg_update("Part Highlighting Enabled.");
   else
     stats_info_msg_update("Part Highlighting Disabled.");
}

static void
dummy_swallow_toggle(app_data *ad)
{
   config_dummy_swallow_set(!config_dummy_swallow_get());
   enventor_object_dummy_swallow_set(ad->enventor, config_dummy_swallow_get());
   if (config_dummy_swallow_get())
     stats_info_msg_update("Dummy Swallow Enabled.");
   else
     stats_info_msg_update("Dummy Swallow Disabled.");
}

static void
default_template_insert(app_data *ad)
{
   if (config_live_edit_get())
     {
        stats_info_msg_update("Insertion of template code is disabled "
                              "while in Live Edit mode");
        return;
     }

   char syntax[12];
   if (enventor_object_template_insert(ad->enventor, syntax, sizeof(syntax)))
     {
        char msg[64];
        snprintf(msg, sizeof(msg), "Template code inserted, (%s)", syntax);
        stats_info_msg_update(msg);
        enventor_object_save(ad->enventor, config_edc_path_get());
     }
   else
     {
        stats_info_msg_update("Can't insert template code here. Move the "
                              "cursor inside the \"Collections,Images,Parts,"
                              "Part,Programs\" scope.");
     }
}

static Eina_Bool
ctrl_func(app_data *ad, const char *key)
{
   //Save
   if (!strcmp(key, "s") || !strcmp(key, "S"))
     {
        edc_save(ad);
        return ECORE_CALLBACK_DONE;
     }
  //Delete Line
   if (!strcmp(key, "d") || !strcmp(key, "D"))
     {
        enventor_object_line_delete(ad->enventor);
        return ECORE_CALLBACK_DONE;
     }
   //Find/Replace
   if (!strcmp(key, "f") || !strcmp(key, "F"))
     {
        search_open(ad->enventor);
        return ECORE_CALLBACK_DONE;
     }
   //Goto Line
   if (!strcmp(key, "l") || !strcmp(key, "L"))
     {
        goto_open(ad->enventor);
        return ECORE_CALLBACK_DONE;
     }
   //Part Highlight
   if (!strcmp(key, "h") || !strcmp(key, "H"))
     {
        part_highlight_toggle(ad);
        return ECORE_CALLBACK_DONE;
     }
   //Swallow Dummy Object
   if (!strcmp(key, "w") || !strcmp(key, "W"))
     {
        dummy_swallow_toggle(ad);
        return ECORE_CALLBACK_DONE;
     }
   //Template Code
   if (!strcmp(key, "t") || !strcmp(key, "T"))
     {
        default_template_insert(ad);
        return ECORE_CALLBACK_DONE;
     }
   //Full Edit View
   if (!strcmp(key, "Left"))
     {
        base_live_view_full_view();
        return ECORE_CALLBACK_DONE;
     }
   //Full Live View
   if (!strcmp(key, "Right"))
     {
        base_enventor_full_view();
        return ECORE_CALLBACK_DONE;
     }
   //Full Console View
   if (!strcmp(key, "Up"))
     {
        base_console_full_view();
        return ECORE_CALLBACK_DONE;
     }
   //Full Editors View
   if (!strcmp(key, "Down"))
     {
        base_editors_full_view();
        return ECORE_CALLBACK_DONE;
     }
   //Auto Indentation
   if (!strcmp(key, "i") || !strcmp(key, "I"))
     {
        auto_indent_toggle(ad);
        return ECORE_CALLBACK_DONE;
     }
   //Auto Completion
   if (!strcmp(key, "o") || !strcmp(key, "O"))
     {
        auto_comp_toggle(ad);
        return ECORE_CALLBACK_DONE;
     }
   //Live Edit
   if (!strcmp(key, "e") || !strcmp(key, "E"))
     {
        live_edit_toggle();
        return ECORE_CALLBACK_DONE;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
main_key_down_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   app_data *ad = data;

   //Shift Key
   if (!strcmp("Shift_L", event->key))
     {
        ad->shift_pressed = EINA_TRUE;
        return ECORE_CALLBACK_DONE;
     }

   if (ad->ctrl_pressed)
     {
        if (ad->shift_pressed) return template_insert_patch(ad, event->key);
        else return ctrl_func(ad, event->key);
     }

   //Main Menu
   if (!strcmp(event->key, "Escape"))
     {
        if (search_is_opened() || goto_is_opened())
          {
             goto_close();
             search_close();
             enventor_object_focus_set(ad->enventor, EINA_TRUE);
             return ECORE_CALLBACK_DONE;
          }
        if (file_mgr_warning_is_opened())
          {
             file_mgr_warning_close();
             return ECORE_CALLBACK_DONE;
          }

        menu_toggle();
        return ECORE_CALLBACK_DONE;
     }

   if (menu_activated_get() > 0) return ECORE_CALLBACK_PASS_ON;

   //Control Key
   if (!strcmp("Control_L", event->key))
     {
        ad->ctrl_pressed = EINA_TRUE;
        return ECORE_CALLBACK_PASS_ON;
     }

   //README
   if (!strcmp(event->key, "F1"))
     {
        menu_about();
        return ECORE_CALLBACK_DONE;
     }

   //New
   if (!strcmp(event->key, "F2"))
     {
        menu_edc_new(EINA_FALSE);
        return ECORE_CALLBACK_DONE;
     }
   //Save
   if (!strcmp(event->key, "F3"))
     {
        menu_edc_save();
        return ECORE_CALLBACK_DONE;
     }
   //Load
   if (!strcmp(event->key, "F4"))
     {
        menu_edc_load();
        return ECORE_CALLBACK_DONE;
     }
   //Line Number
   if (!strcmp(event->key, "F5"))
     {
        config_linenumber_set(!config_linenumber_get());
        enventor_object_linenumber_set(ad->enventor, config_linenumber_get());
        return ECORE_CALLBACK_DONE;
     }

   //Tools
   if (!strcmp(event->key, "F9"))
     {
        base_tools_toggle(EINA_TRUE);
        return ECORE_CALLBACK_DONE;
     }

   //Console
   if (!strcmp(event->key, "F10"))
     {
        base_console_toggle();
        return ECORE_CALLBACK_DONE;
     }
   //Statusbar
   if (!strcmp(event->key, "F11"))
     {
        base_statusbar_toggle(EINA_TRUE);
        return ECORE_CALLBACK_DONE;
     }
   //Setting
   if (!strcmp(event->key, "F12"))
     {
        menu_setting();
        return ECORE_CALLBACK_DONE;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
statusbar_set()
{
   Evas_Object *obj = stats_init(base_layout_get());
   elm_object_part_content_set(base_layout_get(), "elm.swallow.statusbar", obj);
   base_statusbar_toggle(EINA_FALSE);
}

static void
template_show(app_data *ad)
{
   if (ad->template_new)
     menu_edc_new(EINA_TRUE);
}

static Eina_Bool
init(app_data *ad, int argc, char **argv)
{
   enventor_init(argc, argv);

   ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, main_key_down_cb, ad);
   ecore_event_handler_add(ECORE_EVENT_KEY_UP, main_key_up_cb, ad);
   ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL, main_mouse_wheel_cb, ad);

   elm_setup();

   config_data_set(ad, argc, argv);
   newfile_default_set();
   base_gui_init();
   statusbar_set();
   enventor_setup(ad);
   file_mgr_init(ad->enventor);
   tools_set(ad->enventor);

   base_gui_show();

   //Guarantee Enventor editor has focus.
   enventor_object_focus_set(ad->enventor, EINA_TRUE);

   menu_init(ad->enventor);

   template_show(ad);

   return EINA_TRUE;
}

static void
term(app_data *ad EINA_UNUSED)
{
   menu_term();
#if 0
   live_edit_term();
#endif
   stats_term();
   base_gui_term();
   file_mgr_term();
   config_term();
   enventor_shutdown();
}

EAPI_MAIN
int elm_main(int argc, char **argv)
{
   app_data ad = {0, };

   if (!init(&ad, argc, argv))
     {
        term(&ad);
        return 0;
     }

   elm_run();

   term(&ad);

   elm_shutdown();

   return 0;
}
ELM_MAIN()
