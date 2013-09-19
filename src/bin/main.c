#include <Elementary.h>
#include <Eio.h>
#include "config.h"
#include "common.h"

int main(int argc, char **argv);

struct app_s
{
   edit_data *ed;
   edj_mgr *em;
   menu_data *md;
   stats_data *sd;
   config_data *cd;

   Evas_Object *layout;
   Evas_Object *panes;
   Evas_Object *win;
   Eio_Monitor *edc_monitor;

   Eina_Bool ctrl_pressed : 1;
   Eina_Bool shift_pressed : 1;
   Eina_Bool menu_opened : 1;
};

static const char *EDJE_CC_CMD;

static Eina_Bool
rebuild_edc()
{
   int ret = system(EDJE_CC_CMD);
   if (ret == -1) return EINA_FALSE;
   else return EINA_TRUE;
}

static Eina_Bool
edc_changed_cb(void *data, int type EINA_UNUSED, void *event)
{
   //FIXME: Why does this callback called multiple times?
   Eio_Monitor_Event *ev = event;
   app_data *ad = data;

   if (!edit_changed_get(ad->ed)) return ECORE_CALLBACK_RENEW;

   if (strcmp(ev->filename, config_edc_path_get(ad->cd)))
     return ECORE_CALLBACK_RENEW;

   rebuild_edc();
   edit_changed_set(ad->ed, EINA_FALSE);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
edje_cc_cmd_set(config_data *cd)
{
   Eina_Strbuf *buf = eina_strbuf_new();
   if (!buf) return EINA_FALSE;

   char default_path[PATH_MAX * 4];
   sprintf(default_path, "-id %s/images -sd %s/sounds -fd %s/fonts -dd %s/data",
           elm_app_data_dir_get(), elm_app_data_dir_get(),
           elm_app_data_dir_get(), elm_app_data_dir_get());

   eina_strbuf_append_printf(buf,
                             "edje_cc -fastcomp %s %s %s %s %s %s %s",
                             config_edc_path_get(cd),
                             config_edj_path_get(cd),
                             default_path,
                             config_edc_img_path_get(cd),
                             config_edc_snd_path_get(cd),
                             config_edc_fnt_path_get(cd),
                             config_edc_data_path_get(cd));
   eina_strbuf_append(buf, " > /dev/null");
   EDJE_CC_CMD = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);

   return EINA_TRUE;
}

static void
win_delete_request_cb(void *data, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
  app_data *ad = data;
  menu_exit(ad->md);
}

static Eina_Bool
base_gui_construct(app_data *ad)
{
   char buf[PATH_MAX];

   //Window
   Evas_Object *win = elm_win_util_standard_add(elm_app_name_get(),
                                                "Enventor");
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb,
                                  ad);
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

   ad->win = win;
   ad->layout = layout;
   ad->panes = panes;

   return EINA_TRUE;
}

static Eina_Bool
edc_proto_setup(config_data *cd)
{
   Eina_Bool success = EINA_TRUE;

   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/.proto/proto.edc",
            elm_app_data_dir_get());

   if (!ecore_file_exists(config_edc_path_get(cd)))
     {
        EINA_LOG_INFO("No working edc file exists. Copy a proto.edc");
        success = eina_file_copy(buf,
                                 config_edc_path_get(cd),
                                 EINA_FILE_COPY_DATA, NULL, NULL);
     }

   if (!success)
     {
        EINA_LOG_ERR("Cannot find file! \"%s\"", buf);
        return EINA_FALSE;
     }

   rebuild_edc();

   return EINA_TRUE;
}

static Eina_Bool
main_key_up_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   app_data *ad = data;

   if (!strcmp("Control_L", event->keyname))
     {
        edit_editable_set(ad->ed, EINA_TRUE);
        ad->ctrl_pressed = EINA_FALSE;
     }
   else if (!strcmp("Shift_L", event->keyname))
     ad->shift_pressed = EINA_FALSE;

   return ECORE_CALLBACK_PASS_ON;
}

static void
statusbar_toggle(app_data *ad)
{
   if (config_stats_bar_get(ad->cd))
     elm_object_signal_emit(ad->layout, "elm,state,statusbar,show", "");
   else
     elm_object_signal_emit(ad->layout, "elm,state,statusbar,hide", "");
}

static void
part_highlight_toggle(app_data *ad, Eina_Bool msg)
{
   Eina_Bool highlight = config_part_highlight_get(ad->cd);
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
   Eina_Bool toggle = !config_auto_indent_get(ad->cd);
   if (toggle) stats_info_msg_update(ad->sd, "Auto Indentation Enabled.");
   else stats_info_msg_update(ad->sd, "Auto Indentation Disabled.");
   config_auto_indent_set(ad->cd, toggle);
}

static Eina_Bool
template_insert(app_data *ad, const char *keyname)
{
   Edje_Part_Type type;

   if (!strcmp(keyname, "a") || !strcmp(keyname, "A"))
     type = EDJE_PART_TYPE_TABLE;
   else if (!strcmp(keyname, "b") || !strcmp(keyname, "B"))
     type = EDJE_PART_TYPE_TEXTBLOCK;
   else if (!strcmp(keyname, "e") || !strcmp(keyname, "E"))
     type = EDJE_PART_TYPE_EXTERNAL;
   else if (!strcmp(keyname, "g") || !strcmp(keyname, "G"))
     type = EDJE_PART_TYPE_GRADIENT;
   else if (!strcmp(keyname, "i") || !strcmp(keyname, "I"))
     type = EDJE_PART_TYPE_IMAGE;
   else if (!strcmp(keyname, "o") || !strcmp(keyname, "O"))
     type = EDJE_PART_TYPE_GROUP;
   else if (!strcmp(keyname, "p") || !strcmp(keyname, "P"))
     type = EDJE_PART_TYPE_PROXY;
   else if (!strcmp(keyname, "r") || !strcmp(keyname, "R"))
     type = EDJE_PART_TYPE_RECTANGLE;
   else if (!strcmp(keyname, "t") || !strcmp(keyname, "T"))
     type = EDJE_PART_TYPE_TEXT;
   else if (!strcmp(keyname, "s") || !strcmp(keyname, "S"))
     type = EDJE_PART_TYPE_SPACER;
   else if (!strcmp(keyname, "w") || !strcmp(keyname, "W"))
     type = EDJE_PART_TYPE_SWALLOW;
   else if (!strcmp(keyname, "x") || !strcmp(keyname, "X"))
     type = EDJE_PART_TYPE_BOX;
   else
     type = EDJE_PART_TYPE_NONE;

   edit_template_part_insert(ad->ed, type);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
ctrl_func(app_data *ad, const char *keyname)
{
   //Save
   if (!strcmp(keyname, "s") || !strcmp(keyname, "S"))
     {
        edit_save(ad->ed);
        return ECORE_CALLBACK_DONE;
     }
   //Copy
   if (!strcmp(keyname, "c") || !strcmp(keyname, "C"))
     return ECORE_CALLBACK_PASS_ON;
   //Paste
   if (!strcmp(keyname, "v") || !strcmp(keyname, "V"))
     return ECORE_CALLBACK_PASS_ON;
   //Cut
   if (!strcmp(keyname, "x") || !strcmp(keyname, "X"))
     return ECORE_CALLBACK_PASS_ON;
   //Select All
   if (!strcmp(keyname, "a") || !strcmp(keyname, "A"))
     return ECORE_CALLBACK_PASS_ON;
   //Go to Begin/End
   if (!strcmp(keyname, "Home") || !strcmp(keyname, "End"))
     return ECORE_CALLBACK_PASS_ON;
   //Template Code
   if (!strcmp(keyname, "t") || !strcmp(keyname, "T"))
     {
        edit_template_insert(ad->ed);
        return ECORE_CALLBACK_DONE;
     }
   //Part Highlight
   if (!strcmp(keyname, "h") || !strcmp(keyname, "H"))
     {
        config_part_highlight_set(ad->cd, !config_part_highlight_get(ad->cd));
        part_highlight_toggle(ad, EINA_TRUE);
        return ECORE_CALLBACK_DONE;
     }
   //Swallow Dummy Object
   if (!strcmp(keyname, "w") || !strcmp(keyname, "W"))
     {
        config_dummy_swallow_set(ad->cd, !config_dummy_swallow_get(ad->cd));
        view_dummy_toggle(VIEW_DATA, EINA_TRUE);
        return ECORE_CALLBACK_DONE;
     }
   //Full Edit View
   if (!strcmp(keyname, "comma"))
     {
        panes_full_view_left(ad->panes);
        return ECORE_CALLBACK_DONE;
     }
   //Full Edje View
   if (!strcmp(keyname, "period"))
     {
        panes_full_view_right(ad->panes);
        return ECORE_CALLBACK_DONE;
     }
   //Auto Indentation
   if (!strcmp(keyname, "i") || !strcmp(keyname, "I"))
     {
        auto_indentation_toggle(ad);
        return ECORE_CALLBACK_DONE;
     }
   //Font Size Up
   if (!strcmp(keyname, "equal"))
     {
        config_font_size_set(ad->cd, config_font_size_get(ad->cd) + 0.1f);
        edit_font_size_update(ad->ed, EINA_TRUE);
        return ECORE_CALLBACK_DONE;
     }
   //Font Size Down
   if (!strcmp(keyname, "minus"))
     {
        config_font_size_set(ad->cd, config_font_size_get(ad->cd) - 0.1f);
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
   if (!strcmp("Shift_L", event->keyname))
     {
        ad->shift_pressed = EINA_TRUE;
        return ECORE_CALLBACK_DONE;
     }

   if (ad->ctrl_pressed)
     {
        if (ad->shift_pressed) return template_insert(ad, event->keyname);
        else return ctrl_func(ad, event->keyname);
     }

   //Main Menu
   if (!strcmp(event->keyname, "Escape"))
     {
        ad->menu_opened = menu_toggle();
        if (!ad->menu_opened)
          edit_focus_set(ad->ed);
        return ECORE_CALLBACK_DONE;
     }

   if (ad->menu_opened) return ECORE_CALLBACK_PASS_ON;

   //Control Key
   if (!strcmp("Control_L", event->keyname))
     {
        ad->ctrl_pressed = EINA_TRUE;
        return ECORE_CALLBACK_PASS_ON;
     }
   //README
   if (!strcmp(event->keyname, "F1"))
     {
        ad->menu_opened = menu_about(ad->md);
        return ECORE_CALLBACK_DONE;
     }
   //New
   if (!strcmp(event->keyname, "F2"))
     {
        ad->menu_opened = menu_edc_new(ad->md);
        return ECORE_CALLBACK_DONE;
     }
   //Save
   if (!strcmp(event->keyname, "F3"))
     {
        ad->menu_opened = menu_edc_save(ad->md);
        return ECORE_CALLBACK_DONE;
     }
   //Load
   if (!strcmp(event->keyname, "F4"))
     {
        ad->menu_opened = menu_edc_load(ad->md);
        return ECORE_CALLBACK_DONE;
     }
   //Line Number
   if (!strcmp(event->keyname, "F5"))
     {
        config_linenumber_set(ad->cd, !config_linenumber_get(ad->cd));
        edit_line_number_toggle(ad->ed);
        return ECORE_CALLBACK_DONE;
     }
   //Statusbar
   if (!strcmp(event->keyname, "F6"))
     {
        config_stats_bar_set(ad->cd, !config_stats_bar_get(ad->cd));
        statusbar_toggle(ad);
        return ECORE_CALLBACK_DONE;
     }
   //Setting
   if (!strcmp(event->keyname, "F12"))
     {
        ad->menu_opened = menu_setting(ad->md);
        return ECORE_CALLBACK_DONE;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
edc_view_set(app_data *ad, config_data *cd, stats_data *sd,
             Eina_Stringshare *group)
{
   view_data *vd = edj_mgr_view_get(ad->em, group);
   if (vd) edj_mgr_view_switch_to(ad->em, vd);
   else vd = edj_mgr_view_new(ad->em, group, sd, cd);

   if (!vd) return;

   if (group) stats_edc_file_set(sd, group);
}

static void
view_sync_cb(void *data, Eina_Stringshare *part_name,
             Eina_Stringshare *group_name)
{
   app_data *ad = data;
   if (stats_group_name_get(ad->sd) != group_name)
     edc_view_set(ad, ad->cd, ad->sd, group_name);
   view_part_highlight_set(VIEW_DATA, part_name);
}

static void
edc_edit_set(app_data *ad, stats_data *sd, config_data *cd)
{
   edit_data *ed = edit_init(ad->panes, sd, cd);
   edit_edc_read(ed, config_edc_path_get(cd));
   elm_object_part_content_set(ad->panes, "right", edit_obj_get(ed));
   edit_view_sync_cb_set(ed, view_sync_cb, ad);
   ad->ed = ed;
}

static void
statusbar_set(app_data *ad, config_data *cd)
{
   stats_data *sd = stats_init(ad->layout, cd);
   elm_object_part_content_set(ad->layout, "elm.swallow.statusbar",
                               stats_obj_get(sd));
   ad->sd = sd;
   config_stats_bar_set(ad->cd, EINA_TRUE);
   statusbar_toggle(ad);
}

static void
config_update_cb(void *data, config_data *cd)
{
   app_data *ad = data;
   edje_cc_cmd_set(cd);
   edit_line_number_toggle(ad->ed);
   edit_font_size_update(ad->ed, EINA_FALSE);
   statusbar_toggle(ad);
   part_highlight_toggle(ad, EINA_FALSE);

   view_dummy_toggle(VIEW_DATA, EINA_FALSE);

   //previous build was failed, Need to rebuild then reload the edj.
   if (view_reload_need_get(VIEW_DATA))
     {
        rebuild_edc();
        edit_changed_set(ad->ed, EINA_FALSE);
        view_new(VIEW_DATA, stats_group_name_get(ad->sd));
        view_sync_cb(VIEW_DATA, NULL, NULL);
        if (ad->edc_monitor) eio_monitor_del(ad->edc_monitor);
        ad->edc_monitor = eio_monitor_add(config_edc_path_get(ad->cd));
     }
   //If the edc is reloaded, then rebuild it!
   else if (edit_changed_get(ad->ed))
     {
        rebuild_edc();
        edit_changed_set(ad->ed, EINA_FALSE);
     }
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
   config_data *cd = config_init(edc_path, img_path, snd_path, fnt_path,
                                 data_path);
   config_update_cb_set(cd, config_update_cb, ad);
   ad->cd = cd;
}

static void
elm_setup()
{
    elm_config_profile_set("standard");

    //Recover the scale since it will be reset by elm_config_profile_set()
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
menu_close_cb(void *data)
{
   app_data *ad = data;
   ad->menu_opened = EINA_FALSE;
}

static void
edj_mgr_set(app_data *ad)
{
   ad->em = edj_mgr_init(ad->panes);
   elm_object_part_content_set(ad->panes, "left", edj_mgr_obj_get(ad->em));
}

static Eina_Bool
init(app_data *ad, int argc, char **argv)
{
   /*To add a key event handler before evas, here initialize the ecore_event
     and add handlers. */
   ecore_event_init();
   ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, main_key_down_cb, ad);
   ecore_event_handler_add(ECORE_EVENT_KEY_UP, main_key_up_cb, ad);

   elm_init(argc, argv);
   elm_setup();
   config_data_set(ad, argc, argv);

   if (!edje_cc_cmd_set(ad->cd)) return EINA_FALSE;
   if (!edc_proto_setup(ad->cd)) return EINA_FALSE;
   if (!base_gui_construct(ad)) return EINA_FALSE;

   edj_mgr_set(ad);
   statusbar_set(ad, ad->cd);
   edc_edit_set(ad, ad->sd, ad->cd);
   edc_view_set(ad, ad->cd, ad->sd, stats_group_name_get(ad->sd));
   ad->md = menu_init(ad->win, ad->ed, ad->cd, menu_close_cb, ad);

   ad->edc_monitor = eio_monitor_add(config_edc_path_get(ad->cd));
   ecore_event_handler_add(EIO_MONITOR_FILE_MODIFIED, edc_changed_cb, ad);

   return EINA_TRUE;
}

static void
term(app_data *ad)
{
   menu_term(ad->md);
   edit_term(ad->ed);
   edj_mgr_term(ad->em);
   stats_term(ad->sd);
   config_term(ad->cd);

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
