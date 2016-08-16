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
part_type_get(Edje_Part_Type type)
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

char*
find_group_proc_internal(char *utf8, char *utf8_end, const char *group_name)
{
   char *p = utf8;
   char *result = NULL;

   int group_name_len = strlen(group_name);

   //Find group
   while (utf8_end > p)
     {
        //Skip " ~ " Section
        if (*p == '\"')
          {
             ESCAPE_RET_NULL();
          }

        if (!strncmp("group", p, strlen("group")))
          {
             p = strstr((p + 5), "\"");
             if (!p) return NULL;
             p++;
             if (!strncmp(group_name, p, group_name_len))
               {
                  //Compare Elaborately
                  char *next_quote = strstr(p, "\"");
                  if (group_name_len == (next_quote - p))
                    {
                       result = p;
                       break;
                    }
                  else
                    {
                       ESCAPE_RET_NULL();
                    }
               }
             else
               {
                  ESCAPE_RET_NULL();
               }
          }
        p++;
     }

   return result;
}

char*
find_part_proc_internal(char *utf8, char *utf8_end, const char* group_name,
                        const char *part_name, const char *part_type)
{
   char *p = find_group_proc_internal(utf8, utf8_end, group_name);

   //No found
   if (!p) return NULL;

   p = strstr(p, "\"");
   if (!p) return NULL;
   p++;

   char *result = NULL;

   //goto parts
   p = strstr(p, "parts");
   if (!p) return NULL;

   int part_name_len = strlen(part_name);

   //Find part
   while (utf8_end > p)
     {
        //Skip " ~ " Section
        if (*p == '\"')
          {
             ESCAPE_RET_NULL();
          }

        if (!strncmp(part_type, p, strlen(part_type)))
          {
             p = strstr((p + strlen(part_type)), "\"");
             if (!p) return NULL;
             p++;
             if (!strncmp(part_name, p, part_name_len))
               {
                  //Compare Elaborately
                  char *next_quote = strstr(p, "\"");
                  if (part_name_len == (next_quote - p))
                    {
                       result = p;
                       break;
                    }
                  else
                    {
                       ESCAPE_RET_NULL();
                    }
               }
             else
               {
                  ESCAPE_RET_NULL();
               }
          }

        //compatibility: "part"
        if (!strncmp("part", p, strlen("part")))
          {
             p = strstr((p + 4), "\"");
             if (!p) return NULL;
             p++;
             if (!strncmp(part_name, p, strlen(part_name)))
               {
                  //Compare Elaborately
                  char *next_quote = strstr(p, "\"");
                  if (part_name_len == (next_quote - p))
                    {
                       result = p;
                       break;
                    }
                  else
                    {
                       ESCAPE_RET_NULL();
                    }
               }
             else
               {
                  ESCAPE_RET_NULL();
               }
          }

        p++;
     }

   return result;
}
