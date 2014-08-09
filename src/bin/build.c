#include <Elementary.h>
#include "common.h"

typedef struct builder_s
{
   Eina_Strbuf *strbuf;
   char *build_cmd;
   void (*noti_cb)(void *data, const char *msg);
   void *noti_data;
   Ecore_Event_Handler *event_data_handler;
   Ecore_Event_Handler *event_err_handler;

} build_data;

static build_data *g_bd = NULL;

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

Eina_Bool
build_cmd_set(void)
{
   build_data *bd = g_bd;

   free(bd->build_cmd);
   bd->build_cmd = NULL;

   Eina_Strbuf *strbuf = eina_strbuf_new();
   if (!strbuf)
     {
        EINA_LOG_ERR("Failed to new strbuf");
        return EINA_FALSE;
     }

   eina_strbuf_append_printf(strbuf,
                             "edje_cc -fastcomp %s %s -id %s/images -sd %s/sounds -fd %s/fonts -dd %s/data %s %s %s %s",
                             config_edc_path_get(),
                             config_edj_path_get(),
                             elm_app_data_dir_get(),
                             elm_app_data_dir_get(),
                             elm_app_data_dir_get(),
                             elm_app_data_dir_get(),
                             config_edc_img_path_get(),
                             config_edc_snd_path_get(),
                             config_edc_fnt_path_get(),
                             config_edc_data_path_get());

   bd->build_cmd = eina_strbuf_string_steal(strbuf);
   eina_strbuf_free(strbuf);

   return EINA_TRUE;
}

void
build_edc(void)
{
   build_data *bd = g_bd;
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

Eina_Bool
build_init(void)
{
   build_data *bd = g_bd;
   if (bd) return EINA_TRUE;

   bd = calloc(1, sizeof(build_data));
   if (!bd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return EINA_FALSE;
     }
   g_bd = bd;

   Eina_Bool ret = build_cmd_set();

   bd->event_data_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DATA,
                                                    exe_event_error_cb, bd);
   bd->event_err_handler = ecore_event_handler_add(ECORE_EXE_EVENT_ERROR,
                                                   exe_event_error_cb, bd);

   bd->strbuf = eina_strbuf_new();

   return ret;
}

void
build_term(void)
{
   build_data *bd = g_bd;
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
