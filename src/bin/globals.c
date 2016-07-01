#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

const char *DEFAULT_EDC_FORMAT = "enventor_XXXXXX.edc";
char EDJE_PATH[PATH_MAX];
const char *ENVENTOR_NAME = "enventor";
Enventor_Item *active_item = NULL;

void mem_fail_msg(void)
{
   EINA_LOG_ERR("Failed to allocate Memory!");
}

void facade_it_select(Enventor_Item *it)
{
   file_tab_it_select(it);
   enventor_item_focus_set(it);
   base_text_editor_set(it);
   base_title_set(enventor_item_file_get(it));
}

Enventor_Item *facade_sub_file_add(const char *path)
{
   Enventor_Item *it = enventor_object_sub_file_add(base_enventor_get(), path);
   if (!it) return NULL;

   file_tab_it_add(it);
   file_tab_it_select(it);

   facade_it_select(it);

   return it;
}

Enventor_Item *facade_main_file_set(const char *path)
{
   Enventor_Item *it = enventor_object_main_file_set(base_enventor_get(), path);
   if (!it) return NULL;

   file_tab_clear();
   file_tab_it_add(it);

   facade_it_select(it);

   base_console_reset();

   return it;
}
