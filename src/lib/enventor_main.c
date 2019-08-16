#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include <Eio.h>
#include "enventor_private.h"

char EDJE_PATH[PATH_MAX];
Eina_Prefix *PREFIX = NULL;
const char SIG_CURSOR_LINE_CHANGED[] = "cursor,line,changed";
const char SIG_CURSOR_GROUP_CHANGED[]= "cursor,group,changed";
const char SIG_LIVE_VIEW_LOADED[] = "live_view,loaded";
const char SIG_LIVE_VIEW_UPDATED[] = "live_view,updated";
const char SIG_LIVE_VIEW_CURSOR_MOVED[] = "live_view,cursor,moved";
const char SIG_LIVE_VIEW_RESIZED[] = "live_view,resized";
const char SIG_MAX_LINE_CHANGED[] = "max_line,changed";
const char SIG_COMPILE_ERROR[] = "compile,error";
const char SIG_CTXPOPUP_CHANGED[] = "ctxpopup,changed";
const char SIG_CTXPOPUP_DISMISSED[] = "ctxpopup,dismissed";
const char SIG_CTXPOPUP_ACTIVATED[] = "ctxpopup,activated";
const char SIG_EDC_MODIFIED[] = "edc,modified";
const char SIG_FOCUSED[] = "focused";
const char SIG_FILE_OPEN_REQUESTED[] = "file,open,requested";

static int _enventor_init_count = 0;
static int _enventor_log_dom = -1;

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

   PREFIX = eina_prefix_new(NULL, enventor_init, "ENVENTOR", "enventor", NULL,
                         PACKAGE_BIN_DIR, PACKAGE_LIB_DIR,
                         PACKAGE_DATA_DIR, PACKAGE_DATA_DIR);

   snprintf(EDJE_PATH, sizeof(EDJE_PATH), "%s/themes/enventor.edj",
            eina_prefix_data_get(PREFIX));
   srand(time(NULL));

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

   if ((--_enventor_init_count) > 0) return _enventor_init_count;

   if ((_enventor_log_dom != -1) &&
       (_enventor_log_dom != EINA_LOG_DOMAIN_GLOBAL))
     {
        eina_log_domain_unregister(_enventor_log_dom);
        _enventor_log_dom = -1;
     }
   eina_prefix_free(PREFIX);

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
