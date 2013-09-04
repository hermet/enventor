#include <Elementary.h>
#include "common.h"


struct indent_s
{
   Eina_Strbuf *strbuf;
};

indent_data *
indent_init(Eina_Strbuf *strbuf)
{
   indent_data *id = malloc(sizeof(indent_data));
   return id;
}

void
indent_term(indent_data *id)
{
   free(id);
}

int
indent_depth_get(indent_data *id, char *src, int pos)
{
   if (!src || (pos < 1)) return 0;

   int depth = 0;
   const char *quot = QUOT;
   int quot_len = QUOT_LEN;
   char *cur = (char *) src;
   char *end = ((char *) src) + pos;

   while (cur && (cur <= end))
     {
        //Skip "" range
        if (*cur == *quot)
          {
             cur += quot_len;
             cur = strstr(cur, quot);
             if (!cur) return depth;
             cur += quot_len;
          }

        if (*cur == '{') depth++;
        else if (*cur == '}') depth--;
        cur++;
     }

   return depth;
}
