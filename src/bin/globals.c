#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

const char *DEFAULT_EDC_FORMAT = "enventor_XXXXXX.edc";
char EDJE_PATH[PATH_MAX];
const char *ENVENTOR_NAME = "enventor";

void mem_fail_msg(void)
{
   EINA_LOG_ERR("Failed to allocate Memory!");
}

Enventor_Item *facade_main_file_set(const char *path)
{
   Enventor_Item *it = enventor_object_main_file_set(base_enventor_get(), path);
   if (!it) return NULL;

   file_tab_clear();
   file_tab_it_add(it);
   file_tab_it_selected_set(it);

   base_text_editor_set(it);
   base_title_set(path);
   base_console_reset();

   return it;
}
