#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#define ENVENTOR_BETA_API_SUPPORT 1

#include <Enventor.h>
#include "enventor_private.h"

typedef struct builder_s
{
   Eina_Strbuf *strbuf;
   char *build_cmd;
   void (*noti_cb)(void *data, const char *msg);
   void *noti_data;
   Eina_Stringshare *edc_path;
   Eina_List *pathes_list[5];
   Ecore_Event_Handler *event_data_handler;
   Ecore_Event_Handler *event_err_handler;

   Eina_Bool build_cmd_changed : 1;

} build_data;

static build_data *g_bd = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static Eina_Bool
exe_event_error_cb(void *data, int type EINA_UNUSED, void *event_info)
{
   build_data *bd = data;
   Ecore_Exe_Event_Data *ev = event_info;
   Ecore_Exe_Event_Data_Line *el;

   eina_strbuf_reset(bd->strbuf);

   for (el = ev->lines; el && el->line; el++)
     {
        eina_strbuf_append(bd->strbuf, el->line);
        eina_strbuf_append_char(bd->strbuf, '\n');
     }

   bd->noti_cb(bd->noti_data, eina_strbuf_string_get(bd->strbuf));

   return ECORE_CALLBACK_RENEW;
}

static Eina_Strbuf *
strbuf_path_get(build_data *bd, Enventor_Path_Type type, const char *syntax)
{
   Eina_Strbuf *strbuf = eina_strbuf_new();
   if (!strbuf)
     {
        EINA_LOG_ERR("Failed to new strbuf");
        return NULL;
     }

   Eina_List *l;
   Eina_Stringshare *path;
   EINA_LIST_FOREACH(bd->pathes_list[type], l, path)
     {
        eina_strbuf_append(strbuf, syntax);
        eina_strbuf_append(strbuf, path);
     }
   return strbuf;
}

static void
build_cmd_set(build_data *bd)
{
   if (!bd->build_cmd_changed) return;
   free(bd->build_cmd);
   bd->build_cmd = NULL;

   Eina_Strbuf *strbuf_img = NULL;
   Eina_Strbuf *strbuf_snd = NULL;
   Eina_Strbuf *strbuf_fnt = NULL;
   Eina_Strbuf *strbuf_dat = NULL;

   //Image
   strbuf_img = strbuf_path_get(bd, ENVENTOR_RES_IMAGE, " -id ");
   if (!strbuf_img) goto err;

   strbuf_snd = strbuf_path_get(bd, ENVENTOR_RES_SOUND, " -sd ");
   if (!strbuf_snd) goto err;

   strbuf_fnt = strbuf_path_get(bd, ENVENTOR_RES_FONT, " -fd ");
   if (!strbuf_fnt) goto err;

   strbuf_dat = strbuf_path_get(bd, ENVENTOR_RES_DATA, " -dd ");
   if (!strbuf_dat) goto err;

   Eina_Strbuf *strbuf = eina_strbuf_new();
   if (!strbuf)
     {
        EINA_LOG_ERR("Failed to new strbuf");
        goto err;
     }

   eina_strbuf_append_printf(strbuf,
      "edje_cc -fastcomp %s %s -id %s/images -sd %s/sounds -fd %s/fonts -dd %s/data %s %s %s %s",
      bd->edc_path,
      (char *) eina_list_data_get(bd->pathes_list[ENVENTOR_OUT_EDJ]),
      elm_app_data_dir_get(),
      elm_app_data_dir_get(),
      elm_app_data_dir_get(),
      elm_app_data_dir_get(),
      eina_strbuf_string_get(strbuf_img),
      eina_strbuf_string_get(strbuf_snd),
      eina_strbuf_string_get(strbuf_fnt),
      eina_strbuf_string_get(strbuf_dat));
   bd->build_cmd = eina_strbuf_string_steal(strbuf);
   bd->build_cmd_changed = EINA_FALSE;

err:
   eina_strbuf_free(strbuf);
   eina_strbuf_free(strbuf_img);
   eina_strbuf_free(strbuf_snd);
   eina_strbuf_free(strbuf_fnt);
   eina_strbuf_free(strbuf_dat);
}

void
build_edc(void)
{
   build_data *bd = g_bd;

   build_cmd_set(bd);

   if (!bd->build_cmd)
     {
        EINA_LOG_ERR("Build Command is not set!");
        return;
     }
   Ecore_Exe_Flags flags =
      (ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
       ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR);
   ecore_exe_pipe_run(bd->build_cmd, flags, NULL);
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
build_init(void)
{
   build_data *bd = g_bd;
   if (bd) return;

   bd = calloc(1, sizeof(build_data));
   if (!bd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }
   g_bd = bd;

   bd->event_data_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DATA,
                                                    exe_event_error_cb, bd);
   bd->event_err_handler = ecore_event_handler_add(ECORE_EXE_EVENT_ERROR,
                                                   exe_event_error_cb, bd);
   bd->strbuf = eina_strbuf_new();
}

void
build_term(void)
{
   build_data *bd = g_bd;
   eina_stringshare_del(bd->edc_path);

   int i;
   for (i = 0; i < (sizeof(bd->pathes_list) / sizeof(Eina_List *)); i++)
     {
        Eina_Stringshare *path;
        EINA_LIST_FREE(bd->pathes_list[i], path)
          eina_stringshare_del(path);
     }

   ecore_event_handler_del(bd->event_data_handler);
   ecore_event_handler_del(bd->event_err_handler);
   eina_strbuf_free(bd->strbuf);
   free(bd->build_cmd);
   free(bd);
   g_bd = NULL;
}

void
build_err_noti_cb_set(void (*cb)(void *data, const char *msg), void *data)
{
   build_data *bd = g_bd;
   bd->noti_cb = cb;
   bd->noti_data = data;
}

Eina_List *
build_path_get(Enventor_Path_Type type)
{
   build_data *bd = g_bd;
   return bd->pathes_list[type];
}

Eina_Bool
build_path_set(Enventor_Path_Type type, const Eina_List *pathes)
{
   if ((type < ENVENTOR_OUT_EDJ) || (type >= ENVENTOR_PATH_TYPE_LAST))
     return EINA_FALSE;

   build_data *bd = g_bd;
   Eina_Stringshare *path;
   Eina_List *l;

   //don't allow null edj path
   if (!pathes && (type == ENVENTOR_OUT_EDJ)) return EINA_FALSE;

   EINA_LIST_FREE(bd->pathes_list[type], path)
     eina_stringshare_del(path);

   EINA_LIST_FOREACH((Eina_List *)pathes, l, path)
     bd->pathes_list[type] = eina_list_append(bd->pathes_list[type],
                                              eina_stringshare_add(path));
   bd->build_cmd_changed = EINA_TRUE;

   return EINA_TRUE;
}

const char *
build_edj_path_get(void)
{
   build_data *bd = g_bd;
   return eina_list_data_get(bd->pathes_list[ENVENTOR_OUT_EDJ]);
}

const char *
build_edc_path_get(void)
{
   build_data *bd = g_bd;
   return bd->edc_path;
}

void
build_edc_path_set(const char *edc_path)
{
   build_data *bd = g_bd;
   if (bd->edc_path == edc_path) return;
   eina_stringshare_del(bd->edc_path);
   bd->edc_path = eina_stringshare_add(edc_path);
}
