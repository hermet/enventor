#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

const char *DEFAULT_EDC_FORMAT = "enventor_XXXXXX.edc";
char EDJE_PATH[PATH_MAX];
const char *ENVENTOR_NAME = "enventor";
Enventor_Item *active_item = NULL;

void
mem_fail_msg(void)
{
   EINA_LOG_ERR("Failed to allocate Memory!");
}
