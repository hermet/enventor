#include <Elementary.h>
#include "common.h"

struct search_s
{
   int order;
};

search_data *
search_word(search_data *sd, Evas_Object *entry, const char *word,
            Eina_Bool *found)
{
   *found = EINA_FALSE;

   if (!word) return NULL;

   const char *text = elm_entry_entry_get(entry);
   const char *utf8 = elm_entry_markup_to_utf8(text);

   if (!sd) sd = calloc(1, sizeof(search_data));

   //There is no word in the text
   char *s = strstr(utf8, word);
   if (!s)
     {
        free(sd);
        return sd;
     }

   int order = sd->order;

   //No more next word found
   if ((order > 0) && (strlen(s) <= 1)) return sd;

   while (order > 0)
     {
        s++;
        s = strstr(s, word);
        if (!s) return sd;
        order--;
     }

   //Got you!
   int len = strlen(word);
   elm_entry_select_region_set(entry, (s - utf8), (s - utf8) + len);
   sd->order++;
   *found = EINA_TRUE;

   return sd;
}

void
search_stop(search_data *sd)
{
   if (sd) free(sd);
}
