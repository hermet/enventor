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
   id->strbuf = strbuf;
   return id;
}

void
indent_term(indent_data *id)
{
   free(id);
}

static int
indent_depth_get(indent_data *id EINA_UNUSED, char *src, int pos)
{
   if (!src || (pos < 1)) return 0;

   int depth = 0;
   const char *quot = "\"";
   int quot_len = 1; // strlen("&quot;");
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

int
indent_space_get(indent_data *id, Evas_Object *entry)
{
   //Get the indentation depth
   int pos = elm_entry_cursor_pos_get(entry);
   char *src = elm_entry_markup_to_utf8(elm_entry_entry_get(entry));
   int space = indent_depth_get(id, src, pos);
   space *= TAB_SPACE;
   free(src);

   return space;
}

void
indent_insert_apply(indent_data *id, Evas_Object *entry, const char *insert, int cur_line)
{
   if (!strcmp(insert, "<br/>"))
     {
        int space = indent_space_get(id, entry);
        //Alloc Empty spaces
        char *p = alloca(space + 1);
        memset(p, ' ', space);
        p[space] = '\0';

        elm_entry_entry_insert(entry, p);
     }
   else if (insert[0] == '}')
     {
        Evas_Object *tb = elm_entry_textblock_get(entry);
        Evas_Textblock_Cursor *cur = evas_object_textblock_cursor_new(tb);
        evas_textblock_cursor_line_set(cur, cur_line - 1);
        const char *p = evas_textblock_cursor_paragraph_text_get(cur);
        char *utf8 = elm_entry_markup_to_utf8(p);

        int len = strlen(utf8) - 2; //2: eol + '}'
        if (len < 1) return;
        int i = 0;

        evas_textblock_cursor_paragraph_char_last(cur);
        evas_textblock_cursor_char_prev(cur);

        while (i < TAB_SPACE)
          {
             if (utf8[len - i] == ' ')
               {
                  evas_textblock_cursor_char_prev(cur);
                  evas_textblock_cursor_char_delete(cur);
               }
             else break;
             i++;
          }
        elm_entry_calc_force(entry);
        evas_textblock_cursor_free(cur);
        free(utf8);
     }
}


