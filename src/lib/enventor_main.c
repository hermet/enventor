#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#define ENVENTOR_BETA_API_SUPPORT 1

#include <Enventor.h>
#include <Eio.h>
#include "enventor_private.h"

char EDJE_PATH[PATH_MAX];
const char SIG_CURSOR_LINE_CHANGED[] = "cursor,line,changed";
const char SIG_CURSOR_GROUP_CHANGED[]= "cursor,group,changed";
const char SIG_LIVE_VIEW_LOADED[] = "live_view,loaded";
const char SIG_LIVE_VIEW_CURSOR_MOVED[] = "live_view,cursor,moved";
const char SIG_LIVE_VIEW_RESIZED[] = "live_view,resized";
const char SIG_MAX_LINE_CHANGED[] = "max_line,changed";
const char SIG_COMPILE_ERROR[] = "compile,error";
const char SIG_PROGRAM_RUN[] = "program,run";
const char SIG_CTXPOPUP_SELECTED[] = "ctxpopup,selected";
const char SIG_CTXPOPUP_DISMISSED[] = "ctxpopup,dismissed";
const char SIG_EDC_MODIFIED[] = "edc,modified";
const char SIG_FOCUSED[] = "focused";

static int _enventor_init_count = 0;
static int _enventor_log_dom = -1;
static Ecore_Event_Handler *_key_down_handler = NULL;

static Eina_Bool
key_down_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   if (autocomp_event_dispatch(event->key)) return ECORE_CALLBACK_DONE;
   return ECORE_CALLBACK_PASS_ON;
}

EAPI int
enventor_init(int argc, char **argv)
{
   _enventor_init_count++;

   if (_enventor_init_count > 1) return _enventor_init_count;

   if (!eina_init())
     {
        EINA_LOG_ERR("Failed to initialize Eina");
        return _enventor_init_count--;
     }

   if (!eet_init())
     {
        EINA_LOG_ERR("Failed to initialize Eet");
        return _enventor_init_count--;
     }

   if (!evas_init())
     {
        EINA_LOG_ERR("Failed to initialize Eet");
        return _enventor_init_count--;
     }

   if (!ecore_init())
     {
        EINA_LOG_ERR("Failed to initialize Ecore");
        return _enventor_init_count--;
     }

   if (!ecore_file_init())
     {
        EINA_LOG_ERR("Failed to initialize Ecore_File");
        return _enventor_init_count--;
     }

   if (!edje_init())
     {
        EINA_LOG_ERR("Failed to initialize Edje");
        return _enventor_init_count--;
     }

   if (!eio_init())
     {
        EINA_LOG_ERR("Failed to initialize Eio");
        return _enventor_init_count--;
     }

   if (!elm_init(argc, argv))
     {
        EINA_LOG_ERR("Failed to initialize Elementary");
        return _enventor_init_count--;
     }

   _enventor_log_dom = eina_log_domain_register("enventor", EINA_COLOR_CYAN);
   if (!_enventor_log_dom)
     {
        EINA_LOG_ERR("Could not register enventor log domain");
        _enventor_log_dom = EINA_LOG_DOMAIN_GLOBAL;
     }

   //FIXME: These should be moved to bin side.
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_compile_lib_dir_set(PACKAGE_LIB_DIR);
   elm_app_info_set(enventor_init, "enventor", "images/logo.png");

   snprintf(EDJE_PATH, sizeof(EDJE_PATH), "%s/themes/enventor.edj",
            elm_app_data_dir_get());

   srand(time(NULL));

   _key_down_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                               key_down_cb, NULL);
   return _enventor_init_count;
}

EAPI int
enventor_shutdown(void)
{
   if (_enventor_init_count <= 0)
     {
        EINA_LOG_ERR("Init count not greater than 0 in shutdown.");
        return 0;
     }

   ecore_event_handler_del(_key_down_handler);
   _key_down_handler = NULL;

   if ((_enventor_log_dom != -1) &&
       (_enventor_log_dom != EINA_LOG_DOMAIN_GLOBAL))
     {
        eina_log_domain_unregister(_enventor_log_dom);
        _enventor_log_dom = -1;
     }

   elm_shutdown();
   eio_shutdown();
   ecore_file_shutdown();
   ecore_shutdown();
   edje_shutdown();
   evas_shutdown();
   eet_shutdown();
   eina_shutdown();

   return _enventor_init_count;
}
