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

static void
indent_insert_br_case(indent_data *id, Evas_Object *entry)
{
   Evas_Object *tb = elm_entry_textblock_get(entry);
   Evas_Textblock_Cursor *cur = evas_object_textblock_cursor_get(tb);
   const char *text = evas_textblock_cursor_paragraph_text_get(cur);
   char *utf8 = elm_entry_markup_to_utf8(text);
   if (strlen(utf8) > 0)
     {
        evas_textblock_cursor_paragraph_char_first(cur);
        int i = 0;
        while (utf8[i] == ' ')
          {
             evas_textblock_cursor_char_delete(cur);
             i++;
          }
     }

   int space = indent_space_get(id, entry);
   //Alloc Empty spaces
   char *p = alloca(space + 1);
   memset(p, ' ', space);
   p[space] = '\0';

   elm_entry_entry_insert(entry, p);
}

static void
indent_insert_bracket_case(indent_data *id, Evas_Object *entry, int cur_line)
{
   Evas_Object *tb = elm_entry_textblock_get(entry);
   Evas_Textblock_Cursor *cur = evas_object_textblock_cursor_new(tb);
   evas_textblock_cursor_line_set(cur, cur_line - 1);
   const char *text = evas_textblock_cursor_paragraph_text_get(cur);
   char *utf8 = elm_entry_markup_to_utf8(text);

   int len = strlen(utf8) - 1;
   if (len < 0) return;

   while (len)
     {
        if (utf8[len] == '}') break;
        len--;
     }

   int space = indent_space_get(id, entry);
   if (space == len) return;

   int i = 0;
   if (len > space)
     {
        evas_textblock_cursor_paragraph_char_last(cur);
        evas_textblock_cursor_char_prev(cur);

        while (i < (len - space))
          {
             if ((utf8[(len - 1) - i] == ' ') &&
                 (utf8[(len - 2) - i] == ' '))
               {
                  evas_textblock_cursor_char_prev(cur);
                  evas_textblock_cursor_char_delete(cur);
               }
             else break;
             i++;
          }
        //leftover
        if (space == 0 && utf8[0] ==  ' ')
          {
             evas_textblock_cursor_char_prev(cur);
             evas_textblock_cursor_char_delete(cur);
          }
     }
   else
     {
        //Alloc Empty spaces
        space = (space - len);
        char *p = alloca(space + 1);
        memset(p, ' ', space);
        p[space] = '\0';
        evas_textblock_cursor_text_prepend(cur, p);
     }

   elm_entry_calc_force(entry);
   evas_textblock_cursor_free(cur);
   free(utf8);
}

void
indent_delete_apply(indent_data *id, Evas_Object *entry, const char *del,
                    int cur_line)
{
   if (del[0] != ' ') return;

   Evas_Object *tb = elm_entry_textblock_get(entry);
   Evas_Textblock_Cursor *cur = evas_object_textblock_cursor_new(tb);
   evas_textblock_cursor_line_set(cur, cur_line - 1);
   const char *text = evas_textblock_cursor_paragraph_text_get(cur);
   char *utf8 = elm_entry_markup_to_utf8(text);

   int len = strlen(utf8);
   if (len < 0) return;

   evas_textblock_cursor_paragraph_char_last(cur);

   while (len > 0)
     {
        if ((utf8[(len - 1)] == ' '))
          {
             evas_textblock_cursor_char_prev(cur);
             evas_textblock_cursor_char_delete(cur);
          }
        else break;
        len--;
     }

   if (len == 0) evas_textblock_cursor_char_delete(cur);
   elm_entry_calc_force(entry);
   evas_textblock_cursor_free(cur);
   free(utf8);
   if (len == 0) elm_entry_cursor_prev(entry);
}

void
indent_insert_apply(indent_data *id, Evas_Object *entry, const char *insert,
                    int cur_line)
{
   if (!strcmp(insert, "<br/>"))
     indent_insert_br_case(id, entry);
   else if (insert[0] == '}')
     indent_insert_bracket_case(id, entry, cur_line);
}
