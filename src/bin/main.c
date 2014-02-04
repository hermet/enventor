#include <Elementary.h>
#include <Eio.h>
#include "config.h"
#include "common.h"

struct app_s
{
   edit_data *ed;
   edj_mgr *em;
   stats_data *sd;

   Eio_Monitor *edc_monitor;

   Eina_Bool ctrl_pressed : 1;
   Eina_Bool shift_pressed : 1;
};

int main(int argc, char **argv);

static Eina_Bool
edc_changed_cb(void *data, int type EINA_UNUSED, void *event)
{
   //FIXME: Why does this callback called multiple times?
   Eio_Monitor_Event *ev = event;
   app_data *ad = data;

   if (!edit_changed_get(ad->ed)) return ECORE_CALLBACK_RENEW;

   if (strcmp(ev->filename, config_edc_path_get()))
     return ECORE_CALLBACK_RENEW;

   build_edc();
   edit_changed_set(ad->ed, EINA_FALSE);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
edc_proto_setup()
{
   Eina_Bool success = EINA_TRUE;

   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/.proto/proto.edc",
            elm_app_data_dir_get());

   if (!ecore_file_exists(config_edc_path_get()))
     {
        EINA_LOG_INFO("No working edc file exists. Copy a proto.edc");
        success = eina_file_copy(buf, config_edc_path_get(),
                                 EINA_FILE_COPY_DATA, NULL, NULL);
     }

   if (!success)
     {
        EINA_LOG_ERR("Cannot find file! \"%s\"", buf);
        return EINA_FALSE;
     }

   build_edc();

   return EINA_TRUE;
}

static Eina_Bool
main_key_up_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   app_data *ad = data;

   if (!strcmp("Control_L", event->key))
     {
        edit_editable_set(ad->ed, EINA_TRUE);
        ad->ctrl_pressed = EINA_FALSE;
     }
   else if (!strcmp("Shift_L", event->key))
     ad->shift_pressed = EINA_FALSE;

   return ECORE_CALLBACK_PASS_ON;
}

static void
part_highlight_toggle(app_data *ad, Eina_Bool msg)
{
   Eina_Bool highlight = config_part_highlight_get();
   if (highlight) edit_view_sync(ad->ed);
   else view_part_highlight_set(VIEW_DATA, NULL);

   if (!msg) return;

   if (highlight)
     stats_info_msg_update(ad->sd, "Part Highlighting Enabled.");
   else
     stats_info_msg_update(ad->sd, "Part Highlighting Disabled.");
}

static void
auto_indentation_toggle(app_data *ad)
{
   Eina_Bool toggle = !config_auto_indent_get();
   if (toggle) stats_info_msg_update(ad->sd, "Auto Indentation Enabled.");
   else stats_info_msg_update(ad->sd, "Auto Indentation Disabled.");
   config_auto_indent_set(toggle);
}

static Eina_Bool
template_insert(app_data *ad, const char *key)
{
   Edje_Part_Type type;

   if (!strcmp(key, "a") || !strcmp(key, "A"))
     type = EDJE_PART_TYPE_TABLE;
   else if (!strcmp(key, "b") || !strcmp(key, "B"))
     type = EDJE_PART_TYPE_TEXTBLOCK;
   else if (!strcmp(key, "e") || !strcmp(key, "E"))
     type = EDJE_PART_TYPE_EXTERNAL;
   else if (!strcmp(key, "g") || !strcmp(key, "G"))
     type = EDJE_PART_TYPE_GRADIENT;
   else if (!strcmp(key, "i") || !strcmp(key, "I"))
     type = EDJE_PART_TYPE_IMAGE;
   else if (!strcmp(key, "o") || !strcmp(key, "O"))
     type = EDJE_PART_TYPE_GROUP;
   else if (!strcmp(key, "p") || !strcmp(key, "P"))
     type = EDJE_PART_TYPE_PROXY;
   else if (!strcmp(key, "r") || !strcmp(key, "R"))
     type = EDJE_PART_TYPE_RECTANGLE;
   else if (!strcmp(key, "t") || !strcmp(key, "T"))
     type = EDJE_PART_TYPE_TEXT;
   else if (!strcmp(key, "s") || !strcmp(key, "S"))
     type = EDJE_PART_TYPE_SPACER;
   else if (!strcmp(key, "w") || !strcmp(key, "W"))
     type = EDJE_PART_TYPE_SWALLOW;
   else if (!strcmp(key, "x") || !strcmp(key, "X"))
     type = EDJE_PART_TYPE_BOX;
   else
     type = EDJE_PART_TYPE_NONE;

   edit_template_part_insert(ad->ed, type);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
ctrl_func(app_data *ad, const char *key)
{
   //Save
   if (!strcmp(key, "s") || !strcmp(key, "S"))
     {
        edit_save(ad->ed);
        return ECORE_CALLBACK_DONE;
     }
   //Copy
   if (!strcmp(key, "c") || !strcmp(key, "C"))
     return ECORE_CALLBACK_PASS_ON;
   //Paste
   if (!strcmp(key, "v") || !strcmp(key, "V"))
     return ECORE_CALLBACK_PASS_ON;
   //Cut
   if (!strcmp(key, "x") || !strcmp(key, "X"))
     return ECORE_CALLBACK_PASS_ON;
   //Select All
   if (!strcmp(key, "a") || !strcmp(key, "A"))
     return ECORE_CALLBACK_PASS_ON;
   //Go to Begin/End
   if (!strcmp(key, "Home") || !strcmp(key, "End"))
     return ECORE_CALLBACK_PASS_ON;
   //Template Code
   if (!strcmp(key, "t") || !strcmp(key, "T"))
     {
        edit_template_insert(ad->ed);
        return ECORE_CALLBACK_DONE;
     }
   //Part Highlight
   if (!strcmp(key, "h") || !strcmp(key, "H"))
     {
        config_part_highlight_set(!config_part_highlight_get());
        part_highlight_toggle(ad, EINA_TRUE);
        return ECORE_CALLBACK_DONE;
     }
   //Swallow Dummy Object
   if (!strcmp(key, "w") || !strcmp(key, "W"))
     {
        config_dummy_swallow_set(!config_dummy_swallow_get());
        view_dummy_toggle(VIEW_DATA, EINA_TRUE);
        return ECORE_CALLBACK_DONE;
     }
   //Full Edit View
   if (!strcmp(key, "comma"))
     {
        base_full_view_left();
        return ECORE_CALLBACK_DONE;
     }
   //Full Edje View
   if (!strcmp(key, "period"))
     {
        base_full_view_right();
        return ECORE_CALLBACK_DONE;
     }
   //Auto Indentation
   if (!strcmp(key, "i") || !strcmp(key, "I"))
     {
        auto_indentation_toggle(ad);
        return ECORE_CALLBACK_DONE;
     }
   //Font Size Up
   if (!strcmp(key, "equal"))
     {
        config_font_size_set(config_font_size_get() + 0.1f);
        edit_font_size_update(ad->ed, EINA_TRUE);
        return ECORE_CALLBACK_DONE;
     }
   //Font Size Down
   if (!strcmp(key, "minus"))
     {
        config_font_size_set(config_font_size_get() - 0.1f);
        edit_font_size_update(ad->ed, EINA_TRUE);
        return ECORE_CALLBACK_DONE;
     }
   return ECORE_CALLBACK_DONE;
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
        if (ad->shift_pressed) return template_insert(ad, event->key);
        else return ctrl_func(ad, event->key);
     }

   //Main Menu
   if (!strcmp(event->key, "Escape"))
     {
        menu_toggle();
        if (menu_open_depth() == 0)
          edit_focus_set(ad->ed);
        return ECORE_CALLBACK_DONE;
     }

   if (menu_open_depth() > 0) return ECORE_CALLBACK_PASS_ON;

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
        menu_edc_new();
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
        edit_line_number_toggle(ad->ed);
        return ECORE_CALLBACK_DONE;
     }
   //Statusbar
   if (!strcmp(event->key, "F6"))
     {
        config_stats_bar_set(!config_stats_bar_get());
        base_statusbar_toggle();
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

static Eina_Bool
main_mouse_wheel_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Mouse_Wheel *event = ev;
   app_data *ad = data;

   if (!ad->ctrl_pressed) return ECORE_CALLBACK_PASS_ON;

   //Scale up/down layout
   view_data *vd = edj_mgr_view_get(ad->em, NULL);
   double scale = config_view_scale_get();

   if (event->z < 0) scale += 0.1;
   else scale -= 0.1;

   config_view_scale_set(scale);
   scale = config_view_scale_get();
   view_scale_set(vd, scale);

   char buf[256];
   snprintf(buf, sizeof(buf), "View Scale: %2.2fx", scale);
   stats_info_msg_update(ad->sd, buf);

   return ECORE_CALLBACK_PASS_ON;
}

static void
edc_view_set(app_data *ad, stats_data *sd, Eina_Stringshare *group)
{
   view_data *vd = edj_mgr_view_get(ad->em, group);
   if (vd) edj_mgr_view_switch_to(ad->em, vd);
   else vd = edj_mgr_view_new(ad->em, group, sd);

   if (!vd) return;

   if (group) stats_edc_file_set(sd, group);
}

static void
view_sync_cb(void *data, Eina_Stringshare *part_name,
             Eina_Stringshare *group_name)
{
   app_data *ad = data;
   if (stats_group_name_get(ad->sd) != group_name)
     edc_view_set(ad, ad->sd, group_name);
   view_part_highlight_set(VIEW_DATA, part_name);
}

static void
edc_edit_set(app_data *ad, stats_data *sd)
{
   edit_data *ed = edit_init(base_layout_get(), sd);
   edit_edc_read(ed, config_edc_path_get());
   base_right_view_set(edit_obj_get(ed));
   edit_view_sync_cb_set(ed, view_sync_cb, ad);
   ad->ed = ed;
}

static void
statusbar_set(app_data *ad)
{
   stats_data *sd = stats_init(base_layout_get());
   elm_object_part_content_set(base_layout_get(), "elm.swallow.statusbar",
                               stats_obj_get(sd));
   ad->sd = sd;
   config_stats_bar_set(EINA_TRUE);
   base_statusbar_toggle();
}

static void
config_update_cb(void *data)
{
   app_data *ad = data;
   build_cmd_set();
   edit_line_number_toggle(ad->ed);
   edit_font_size_update(ad->ed, EINA_FALSE);

   base_statusbar_toggle();
   part_highlight_toggle(ad, EINA_FALSE);
   view_dummy_toggle(VIEW_DATA, EINA_FALSE);

   //previous build was failed, Need to rebuild then reload the edj.
   if (edj_mgr_reload_need_get(ad->em))
     {
        build_edc();
        edit_changed_set(ad->ed, EINA_FALSE);
        edj_mgr_clear(ad->em);
        edc_view_set(ad, ad->sd, stats_group_name_get(ad->sd));
        if (ad->edc_monitor) eio_monitor_del(ad->edc_monitor);
        ad->edc_monitor = eio_monitor_add(config_edc_path_get());
     }
   //If the edc is reloaded, then rebuild it!
   else if (edit_changed_get(ad->ed))
     {
        edit_changed_set(ad->ed, EINA_FALSE);
     }

   view_scale_set(edj_mgr_view_get(ad->em, NULL), config_view_scale_get());
}

static void
args_dispatch(int argc, char **argv, char *edc_path, char *img_path,
              char *snd_path, char *fnt_path, char *data_path)
{
   Eina_Bool default_edc = EINA_TRUE;
   Eina_Bool default_img = EINA_TRUE;
   Eina_Bool default_snd = EINA_TRUE;
   Eina_Bool default_fnt = EINA_TRUE;
   Eina_Bool default_data = EINA_TRUE;

   //No arguments. set defaults
   if (argc == 1) goto defaults;

   //Help
   if ((argc >=2 ) && !strcmp(argv[1], "--help"))
     {
        fprintf(stdout, "Usage: enventor [input file] [-id image path] [-sd sound path] [-fd font path] [-dd data path]\n");
        exit(0);
     }

   //edc path
   if ((argc >= 2) && ecore_file_can_read(argv[1]))
     {
        sprintf(edc_path, "%s", argv[1]);
        default_edc = EINA_FALSE;
     }
   else goto defaults;

   //edc image path
   int cur_arg = 2;

   while (cur_arg < argc)
     {
        if (argc > (cur_arg + 1))
          {
             if (!strcmp("-id", argv[cur_arg]))
               {
                  sprintf(img_path, "%s", argv[cur_arg + 1]);
                  default_img = EINA_FALSE;
               }
             else if (!strcmp("-sd", argv[cur_arg]))
               {
                  sprintf(snd_path, "%s", argv[cur_arg + 1]);
                  default_snd = EINA_FALSE;
               }
             else if (!strcmp("-fd", argv[cur_arg]))
               {
                  sprintf(fnt_path, "%s", argv[cur_arg + 1]);
                  default_fnt = EINA_FALSE;
               }
             else if (!strcmp("-dd", argv[cur_arg]))
               {
                  sprintf(data_path, "%s", argv[cur_arg + 1]);
                  default_data = EINA_FALSE;
               }
          }
        cur_arg += 2;
     }

defaults:
   if (default_edc) sprintf(edc_path, "%s", PROTO_EDC_PATH);
   if (default_img) sprintf(img_path, "%s/images", elm_app_data_dir_get());
   if (default_snd) sprintf(snd_path, "%s/sounds", elm_app_data_dir_get());
   if (default_fnt) sprintf(fnt_path, "%s/fonts", elm_app_data_dir_get());
   if (default_data) sprintf(data_path, "%s/data", elm_app_data_dir_get());
}

static void
config_data_set(app_data *ad, int argc, char **argv)
{
   char edc_path[PATH_MAX];
   char img_path[PATH_MAX];
   char snd_path[PATH_MAX];
   char fnt_path[PATH_MAX];
   char data_path[PATH_MAX];

   args_dispatch(argc, argv, edc_path, img_path, snd_path, fnt_path, data_path);
   config_init(edc_path, img_path, snd_path, fnt_path, data_path);
   config_update_cb_set(config_update_cb, ad);
}

static void
elm_setup()
{
   elm_config_profile_set("standard");

   /* Recover the scale & theme since it will be reset by
      elm_config_profile_set() */
   elm_theme_set(NULL, "default");
   char *scale = getenv("ELM_SCALE");
   if (scale) elm_config_scale_set(atof(scale));

   elm_config_scroll_bounce_enabled_set(EINA_FALSE);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_compile_lib_dir_set(PACKAGE_LIB_DIR);
   elm_app_info_set(main, "enventor",
                    "images/logo.png");

   snprintf(EDJE_PATH, sizeof(EDJE_PATH), "%s/themes/enventor.edj",
            elm_app_data_dir_get());
   elm_theme_extension_add(NULL, EDJE_PATH);
}

static void
edj_mgr_set(app_data *ad)
{
   ad->em = edj_mgr_init(base_layout_get());
   base_left_view_set(edj_mgr_obj_get(ad->em));
}

static void
hotkeys_set(edit_data *ed)
{
   Evas_Object *hotkeys = hotkeys_create(base_layout_get(), ed);
   base_hotkeys_set(hotkeys);
}

static Eina_Bool
init(app_data *ad, int argc, char **argv)
{
   /*To add a key event handler before evas, here initialize the ecore_event
     and add handlers. */
   ecore_event_init();
   ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, main_key_down_cb, ad);
   ecore_event_handler_add(ECORE_EVENT_KEY_UP, main_key_up_cb, ad);
   ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL, main_mouse_wheel_cb, ad);

   elm_init(argc, argv);
   elm_setup();
   config_data_set(ad, argc, argv);

   if (!build_init()) return EINA_FALSE;
   if (!edc_proto_setup()) return EINA_FALSE;
   if (!base_gui_init()) return EINA_FALSE;

   edj_mgr_set(ad);
   statusbar_set(ad);
   edc_edit_set(ad, ad->sd);
   edc_view_set(ad, ad->sd, stats_group_name_get(ad->sd));
   menu_init(ad->ed);
   hotkeys_set(ad->ed);

   ad->edc_monitor = eio_monitor_add(config_edc_path_get());
   ecore_event_handler_add(EIO_MONITOR_FILE_MODIFIED, edc_changed_cb, ad);

   return EINA_TRUE;
}

static void
term(app_data *ad)
{
   build_term();
   menu_term();
   edit_term(ad->ed);
   edj_mgr_term(ad->em);
   stats_term(ad->sd);
   base_gui_term();
   config_term();

   elm_shutdown();
   ecore_event_shutdown();
}

int
main(int argc, char **argv)
{
   app_data ad;
   memset(&ad, 0x00, sizeof(ad));

   if (!init(&ad, argc, argv))
     {
        term(&ad);
        return 0;
     }

   elm_run();

   term(&ad);

   return 0;
}
