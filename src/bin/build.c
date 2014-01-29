#include <Elementary.h>
#include "common.h"

static char *EDJE_CC_CMD = NULL;

Eina_Bool
build_cmd_set()
{
   Eina_Strbuf *buf = eina_strbuf_new();
   if (!buf) return EINA_FALSE;

   if (EDJE_CC_CMD) free(EDJE_CC_CMD);

   eina_strbuf_append_printf(buf,
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

   EDJE_CC_CMD = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);

   return EINA_TRUE;;
}

void
build_edc()
{
   char *bp = NULL;
   size_t size;
   FILE *stream = open_memstream(&bp, &size);
   //stderr = &(*stream);
   int ret = system(EDJE_CC_CMD);

  // if (bp)
  // printf("@@@@ buf = %s, size = %d\n", bp, size);
}

Eina_Bool
build_init()
{
   return build_cmd_set();
}

void
build_term()
{
   free(EDJE_CC_CMD);
}
