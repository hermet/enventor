#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include "enventor_private.h"

void mem_fail_msg(void)
{
   EINA_LOG_ERR("Failed to allocate Memory!");
}

const char*
part_type_str_convert(Edje_Part_Type type)
{
   switch (type)
     {
        case EDJE_PART_TYPE_RECTANGLE:
          return "rect";
        case EDJE_PART_TYPE_TEXT:
           return "text";
        case EDJE_PART_TYPE_IMAGE:
           return "image";
        case EDJE_PART_TYPE_SWALLOW:
           return "swallow";
        case EDJE_PART_TYPE_TEXTBLOCK:
           return "textblock";
        case EDJE_PART_TYPE_SPACER:
           return "spacer";
        default:
           return "part";
     }
}
