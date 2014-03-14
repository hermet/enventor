#include <Elementary.h>
#include "common.h"


void
newfile_new(edit_data *ed, Eina_Bool init)
{
   Eina_Bool success = EINA_TRUE;
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/.proto/proto.edc",
            elm_app_data_dir_get());
   if (!init || !ecore_file_exists(config_edc_path_get()))
     {
        config_edc_path_set(PROTO_EDC_PATH);
        success = eina_file_copy(buf, config_edc_path_get(),
                                 EINA_FILE_COPY_DATA, NULL, NULL);
     }
   if (!success)
     {
        EINA_LOG_ERR("Cannot find file! \"%s\"", buf);
        return;
     }

   if (!init) edit_edc_reload(ed, PROTO_EDC_PATH);
}
