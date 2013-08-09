#include <Elementary.h>
#include <Eio.h>
#include "common.h"

const char *PROTO_EDC_PATH = "/tmp/.proto.edc";
char EDJE_PATH[PATH_MAX];

struct app_s
{
   edit_data *ed;
   view_data *vd;
   menu_data *md;
   stats_data *sd;
   option_data *od;

   Evas_Object *layout;
   Evas_Object *panes;
   Evas_Object *win;
   Eio_Monitor *edc_monitor;

   Eina_Bool ctrl_pressed : 1;
   Eina_Bool menu_opened : 1;
};

static const char *EDJE_CC_CMD;

static Eina_Bool
rebuild_edc()
{
   system(EDJE_CC_CMD);
   return EINA_TRUE;
}

static Eina_Bool
edc_changed_cb(void *data, int type EINA_UNUSED, void *event)
{
   //FIXME: Why does this callback called multiple times?
   Eio_Monitor_Event *ev = event;
   app_data *ad = data;

   if (!edit_changed_get(ad->ed)) return ECORE_CALLBACK_RENEW;

   if (strcmp(ev->filename, option_edc_path_get(ad->od)))
     return ECORE_CALLBACK_RENEW;

   rebuild_edc();
   edit_changed_set(ad->ed, EINA_FALSE);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
edje_cc_cmd_set(option_data *od)
{
   Eina_Strbuf *buf = eina_strbuf_new();
   if (!buf) return EINA_FALSE;
   eina_strbuf_append_printf(buf, "edje_cc -fastcomp %s %s %s %s",
                             option_edc_path_get(od),
                             option_edj_path_get(od),
                             option_edc_img_path_get(od),
                             option_edc_snd_path_get(od));
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
edc_proto_setup(option_data *od)
{
   Eina_Bool success = EINA_TRUE;

   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/.proto/proto.edc",
            elm_app_data_dir_get());

   if (!ecore_file_exists(option_edc_path_get(od)))
     {
        EINA_LOG_INFO("No working edc file exists. Copy a proto.edc");
        success = eina_file_copy(buf,
                                 option_edc_path_get(od),
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

   return ECORE_CALLBACK_PASS_ON;
}

static void
statusbar_toggle(app_data *ad)
{
   if (option_stats_bar_get(ad->od))
     elm_object_signal_emit(ad->layout, "elm,state,statusbar,show", "");
   else
     elm_object_signal_emit(ad->layout, "elm,state,statusbar,hide", "");
}

static void
part_highlight_toggle(app_data *ad)
{
   Eina_Bool highlight = option_part_highlight_get(ad->od);
   if (highlight) edit_cur_part_update(ad->ed);
   else view_part_highlight_set(ad->vd, NULL);

   if (highlight)
     stats_info_msg_update(ad->sd, "Part Highlighting Enabled");
   else
     stats_info_msg_update(ad->sd, "Part Highlighting Disabled");
}

static Eina_Bool
main_key_down_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   app_data *ad = data;

   if (ad->ctrl_pressed)
     {
        //Save
        if (!strcmp(event->keyname, "s") || !strcmp(event->keyname, "S"))
          {
             edit_save(ad->ed);
             return ECORE_CALLBACK_DONE;
          }
        //Load
        if (!strcmp(event->keyname, "l") || !strcmp(event->keyname, "L"))
          {
             ad->menu_opened = menu_edc_load(ad->md);
             return ECORE_CALLBACK_DONE;
          }
        //Copy
        if (!strcmp(event->keyname, "c") || !strcmp(event->keyname, "C"))
          return ECORE_CALLBACK_PASS_ON;
        //Paste
        if (!strcmp(event->keyname, "v") || !strcmp(event->keyname, "V"))
          return ECORE_CALLBACK_PASS_ON;
        //Select All
        if (!strcmp(event->keyname, "a") || !strcmp(event->keyname, "A"))
          return ECORE_CALLBACK_PASS_ON;
        //Part Highlight
        if (!strcmp(event->keyname, "h") || !strcmp(event->keyname, "H"))
          {
             option_part_highlight_set(ad->od,
                                       !option_part_highlight_get(ad->od));
             part_highlight_toggle(ad);
             return ECORE_CALLBACK_DONE;
          }
        //Part Highlight
        if (!strcmp(event->keyname, "w") || !strcmp(event->keyname, "W"))
          {
             option_dummy_swallow_set(ad->od,
                                     !option_dummy_swallow_get(ad->od));
             view_dummy_toggle(ad->vd);
             return ECORE_CALLBACK_DONE;
          }
        //Full Edit View
        if (!strcmp(event->keyname, "comma"))
          {
             panes_full_view_left(ad->panes);
             return ECORE_CALLBACK_DONE;
          }
        //Full Edje View
        if (!strcmp(event->keyname, "period"))
          {
             panes_full_view_right(ad->panes);
             return ECORE_CALLBACK_DONE;
          }
        if (!strcmp(event->keyname, "slash"))
          {
             panes_full_view_cancel(ad->panes);
             return ECORE_CALLBACK_DONE;
          }

        return ECORE_CALLBACK_DONE;
     }

   //Main Menu
   if (!strcmp(event->keyname, "Escape"))
     {
        ad->menu_opened = menu_option_toggle();
        if (!ad->menu_opened)
          edit_focus_set(ad->ed);
        return ECORE_CALLBACK_DONE;
     }

   if (ad->menu_opened) return ECORE_CALLBACK_PASS_ON;

   //Control Key
   if (!strcmp("Control_L", event->keyname))
     {
        ad->ctrl_pressed = EINA_TRUE;
     }
   //README
   else if (!strcmp(event->keyname, "F1"))
     {
        ad->menu_opened = menu_help(ad->md);
        return ECORE_CALLBACK_DONE;
     }
   //Line Number
   else if (!strcmp(event->keyname, "F5"))
     {
        option_linenumber_set(ad->od, !option_linenumber_get(ad->od));
        edit_line_number_toggle(ad->ed);
        return ECORE_CALLBACK_DONE;
     }
   //Statusbar
   else if (!strcmp(event->keyname, "F6"))
     {
        option_stats_bar_set(ad->od, !option_stats_bar_get(ad->od));
        statusbar_toggle(ad);
        return ECORE_CALLBACK_DONE;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
part_changed_cb(void *data, const char *part_name)
{
   app_data *ad = data;
   view_part_highlight_set(ad->vd, part_name);
}

static void
edc_edit_set(app_data *ad, stats_data *sd, option_data *od)
{
   edit_data *ed = edit_init(ad->panes, sd, od);
   edit_edc_read(ed, option_edc_path_get(od));
   elm_object_part_content_set(ad->panes, "right", edit_obj_get(ed));
   edit_part_changed_cb_set(ed, part_changed_cb, ad);
   ad->ed = ed;
}

static void
edc_view_set(app_data *ad, option_data *od, stats_data *sd)
{
   const char *group = edit_group_name_get(ad->ed);
   view_data *vd = view_init(ad->panes, group, sd, od);
   elm_object_part_content_set(ad->panes, "left", view_obj_get(vd));
   ad->vd = vd;
}

static void
statusbar_set(app_data *ad, option_data *od)
{
   stats_data *sd = stats_init(ad->layout, od);
   elm_object_part_content_set(ad->layout, "elm.swallow.statusbar",
                               stats_obj_get(sd));
   ad->sd = sd;
   option_stats_bar_set(ad->od, EINA_TRUE);
   statusbar_toggle(ad);
}

static void
option_update_cb(void *data, option_data *od)
{
   app_data *ad = data;
   edje_cc_cmd_set(od);
   edit_line_number_toggle(ad->ed);
   statusbar_toggle(ad);
   part_highlight_toggle(ad);
   view_dummy_toggle(ad->vd);

   //previous build was failed, Need to rebuild then reload the edj.
   if (view_reload_need_get(ad->vd))
     {
        rebuild_edc();
        edit_changed_set(ad->ed, EINA_FALSE);
        view_new(ad->vd, edit_group_name_get(ad->ed));
        part_changed_cb(ad, NULL);
        if (ad->edc_monitor) eio_monitor_del(ad->edc_monitor);
        ad->edc_monitor = eio_monitor_add(option_edc_path_get(ad->od));
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
              char *snd_path)
{
   Eina_Bool default_edc = EINA_TRUE;
   Eina_Bool default_img = EINA_TRUE;
   Eina_Bool default_snd = EINA_TRUE;

   //No arguments. set defaults
   if (argc == 1) goto defaults;

   //Help
   if ((argc >=2 ) && !strcmp(argv[1], "--help"))
     {
        fprintf(stdout, "Usage: enventor [input file] [-id image path] [-sd sound path]\n");
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
          }
        cur_arg += 2;
     }

defaults:
   if (default_edc) sprintf(edc_path, "%s", PROTO_EDC_PATH);
   if (default_img) sprintf(img_path, "%s/images", elm_app_data_dir_get());
   if (default_snd) sprintf(snd_path, "%s/sounds", elm_app_data_dir_get());
}

static void
config_data_set(app_data *ad, int argc, char **argv)
{
   char edc_path[PATH_MAX];
   char img_path[PATH_MAX];
   char snd_path[PATH_MAX];

   args_dispatch(argc, argv, edc_path, img_path, snd_path);
   option_data *od = option_init(edc_path, img_path, snd_path);
   option_update_cb_set(od, option_update_cb, ad);
   ad->od = od;
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
   elm_app_info_set("/usr/local/bin", "enventor",
                    "/usr/local/share/enventor");

   snprintf(EDJE_PATH, sizeof(EDJE_PATH), "%s/theme/enventor.edj",
            elm_app_data_dir_get());

   elm_theme_extension_add(NULL, EDJE_PATH);
}

static void
menu_close_cb(void *data)
{
   app_data *ad = data;
   ad->menu_opened = EINA_FALSE;
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

   if (!edje_cc_cmd_set(ad->od)) return EINA_FALSE;
   if (!edc_proto_setup(ad->od)) return EINA_FALSE;
   if (!base_gui_construct(ad)) return EINA_FALSE;

   statusbar_set(ad, ad->od);
   edc_edit_set(ad, ad->sd, ad->od);
   edc_view_set(ad, ad->od, ad->sd);
   ad->md = menu_init(ad->win, ad->ed, ad->od, ad->vd, menu_close_cb, ad);

   ad->edc_monitor = eio_monitor_add(option_edc_path_get(ad->od));
   ecore_event_handler_add(EIO_MONITOR_FILE_MODIFIED, edc_changed_cb, ad);

   return EINA_TRUE;
}

static void
term(app_data *ad)
{
   menu_term(ad->md);
   view_term(ad->vd);
   edit_term(ad->ed);
   stats_term(ad->sd);
   option_term(ad->od);

   elm_shutdown();
   ecore_event_shutdown();
}

int
main(int argc, char **argv)
{
   app_data ad = { 0 };

   if (!init(&ad, argc, argv))
     {
        term(&ad);
        return 0;
     }

   elm_run();

   term(&ad);

   return 0;
}
