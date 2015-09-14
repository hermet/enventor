#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include "enventor_private.h"

struct indent_s
{
   Eina_Strbuf *strbuf;
};

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

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

static void
indent_insert_br_case(indent_data *id, Evas_Object *entry)
{
   Evas_Object *tb = elm_entry_textblock_get(entry);
   Evas_Textblock_Cursor *cur = evas_object_textblock_cursor_get(tb);
   redoundo_data *rd = evas_object_data_get(entry, "redoundo");
   const char *text = evas_textblock_cursor_paragraph_text_get(cur);
   char *utf8 = elm_entry_markup_to_utf8(text);
   Eina_Strbuf* diff = eina_strbuf_new();
   int rd_cur_pos = evas_textblock_cursor_pos_get(cur);

   if (strlen(utf8) > 0)
     {
        evas_textblock_cursor_paragraph_char_first(cur);
        int i = 0;
        while (utf8[i] == ' ')
          {
             eina_strbuf_append(diff, evas_textblock_cursor_content_get(cur));
             evas_textblock_cursor_char_delete(cur);
             i++;
          }
     }
   free(utf8);
   redoundo_text_push(rd, eina_strbuf_string_get(diff), rd_cur_pos, 0,
                      EINA_FALSE);
   eina_strbuf_free(diff);

   int space = indent_space_get(id, entry);
   if (space <= 0) return;

   //Alloc Empty spaces
   char *p = alloca(space + 1);
   memset(p, ' ', space);
   p[space] = '\0';

   redoundo_text_push(rd, p, elm_entry_cursor_pos_get(entry), 0, EINA_TRUE);

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
   if (space == len)
     {
        free(utf8);
        return;
     }

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

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

indent_data *
indent_init(Eina_Strbuf *strbuf)
{
   indent_data *id = malloc(sizeof(indent_data));
   if (!id)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }
   id->strbuf = strbuf;
   return id;
}

void
indent_term(indent_data *id)
{
   free(id);
}

int
indent_space_get(indent_data *id, Evas_Object *entry)
{
   //Get the indentation depth
   int pos = elm_entry_cursor_pos_get(entry);
   char *src = elm_entry_markup_to_utf8(elm_entry_entry_get(entry));
   int space = indent_depth_get(id, src, pos);
   if (space < 0) space = 0;
   space *= TAB_SPACE;
   free(src);

   return space;
}

Eina_Bool
indent_delete_apply(indent_data *id EINA_UNUSED, Evas_Object *entry,
                    const char *del, int cur_line)
{
   if (del[0] != ' ') return EINA_FALSE;

   Evas_Object *tb = elm_entry_textblock_get(entry);
   Evas_Textblock_Cursor *cur = evas_object_textblock_cursor_new(tb);
   evas_textblock_cursor_line_set(cur, cur_line - 1);
   const char *text = evas_textblock_cursor_paragraph_text_get(cur);
   char *utf8 = elm_entry_markup_to_utf8(text);
   Eina_Strbuf* diff = eina_strbuf_new();
   int rd_cur_pos = evas_textblock_cursor_pos_get(cur);
   redoundo_data *rd = evas_object_data_get(entry, "redoundo");

   int len = strlen(utf8);
   if (len < 0) return EINA_FALSE;

   evas_textblock_cursor_paragraph_char_last(cur);

   while (len > 0)
     {
        if ((utf8[(len - 1)] == ' '))
          {
             evas_textblock_cursor_char_prev(cur);
             eina_strbuf_append(diff, evas_textblock_cursor_content_get(cur));
             evas_textblock_cursor_char_delete(cur);
          }
        else break;
        len--;
     }

   if (len == 0)
     {
        eina_strbuf_append(diff, evas_textblock_cursor_content_get(cur));
        evas_textblock_cursor_char_delete(cur);
     }
   redoundo_text_push(rd, eina_strbuf_string_get(diff), rd_cur_pos, 0,
                      EINA_FALSE);
   elm_entry_calc_force(entry);
   evas_textblock_cursor_free(cur);
   free(utf8);
   eina_strbuf_free(diff);
   if (len == 0)
     {
        elm_entry_cursor_prev(entry);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

static int
indent_text_auto_format(indent_data *id EINA_UNUSED,
                        Evas_Object *entry, const char *insert)
{
   int line_cnt = 0;
   char *utf8 = evas_textblock_text_markup_to_utf8(NULL, insert);
   int utf8_size = strlen(utf8);

   Evas_Object *tb = elm_entry_textblock_get(entry);
   Evas_Textblock_Cursor *cur_start = evas_object_textblock_cursor_new(tb);
   Evas_Textblock_Cursor *cur_end = evas_object_textblock_cursor_get(tb);
   redoundo_data *rd = evas_object_data_get(entry, "redoundo");

   char *utf8_ptr = utf8;
   char *utf8_lexem = NULL;
   char *utf8_end = utf8 + utf8_size;
   Eina_List *code_lines = NULL;
   Eina_Strbuf *buf = eina_strbuf_new();

   int tb_cur_pos = 0;

   while (utf8_ptr <= utf8_end)
     {
        if (*utf8_ptr != ' ' && *utf8_ptr != '\t' &&  *utf8_ptr != '\n' )
          {
             utf8_lexem = utf8_ptr;
             while (utf8_ptr <= utf8_end)
               {
                  if (*utf8_ptr == '{' || *utf8_ptr == '}' || *utf8_ptr == ';')
                    {
                       if (utf8_ptr + 1 == utf8_end)
                         code_lines = eina_list_append(code_lines,
                                         eina_stringshare_add(utf8_lexem));
                       else
                         code_lines = eina_list_append(code_lines,
                                         eina_stringshare_add_length(utf8_lexem,
                                         utf8_ptr - utf8_lexem + 1));
                       break;
                    }
                 utf8_ptr++;
               }
          }
        utf8_ptr++;
     }
   free(utf8);

   if (!code_lines) return line_cnt;
   tb_cur_pos = evas_textblock_cursor_pos_get(cur_end);
   evas_textblock_cursor_pos_set(cur_start, tb_cur_pos - utf8_size);
   evas_textblock_cursor_range_delete(cur_start, cur_end);

   char *frmt_buf = NULL;
   Eina_List *l = NULL;
   Eina_Stringshare *line;
   evas_textblock_cursor_line_char_first(cur_start);
   evas_textblock_cursor_line_char_last(cur_end);
   int space = indent_space_get(id, entry);

   utf8 = evas_textblock_cursor_range_text_get(cur_start, cur_end,
                                               EVAS_TEXTBLOCK_TEXT_PLAIN);
   utf8_ptr = utf8 + strlen(utf8);
   while (utf8_ptr && (utf8_ptr >= utf8))
     {
        if (utf8_ptr[0] != ' ')
         {
            if ((utf8_ptr[0] == '}') && (space > 0))
              space -= TAB_SPACE;
            if (!evas_textblock_cursor_paragraph_next(cur_start))
             {
                code_lines = eina_list_prepend(code_lines,
                                               eina_stringshare_add("<br/>"));
                evas_textblock_cursor_line_char_last(cur_start);
             }
            break;
         }
        utf8_ptr--;
     }
   free(utf8);

   EINA_LIST_FOREACH(code_lines, l, line)
    {
       if (strstr(line, "}") && (space > 0))
         space -= TAB_SPACE;
       char *p = alloca(space + 1);
       memset(p, ' ', space);
       p[space] = '\0';
       if (strcmp(line, "<br/>"))
         eina_strbuf_append_printf(buf, "%s%s<br/>", p, line);
       else
         eina_strbuf_append_length(buf, "<br/>", 5);
       memset(p, 0x0, space);
       if (strstr(line, "{")) space += TAB_SPACE;
       eina_stringshare_del(line);
       line_cnt++;
    }

  frmt_buf = eina_strbuf_string_steal(buf);
  tb_cur_pos = evas_textblock_cursor_pos_get(cur_start);
  evas_textblock_cursor_pos_set(cur_end, tb_cur_pos);

  evas_object_textblock_text_markup_prepend(cur_start, frmt_buf);

  // Cancel last added diff, that was created when text pasted into entry.
  redoundo_n_diff_cancel(rd, 1);
  //Add data about formatted change into the redoundo queue.
  redoundo_text_push(rd, frmt_buf, tb_cur_pos, 0, EINA_TRUE);

  eina_strbuf_free(buf);
  free(frmt_buf);
  evas_textblock_cursor_free(cur_start);
  return line_cnt;
}

int
indent_insert_apply(indent_data *id, Evas_Object *entry, const char *insert,
                    int cur_line)
{
   int len = strlen(insert);
   if (len == 1)
     {
        if (insert[0] == '}')
          indent_insert_bracket_case(id, entry, cur_line);
        return 0;
     }
   else
     {
        if (!strcmp(insert, EOL))
          {
            indent_insert_br_case(id, entry);
            return 1;
          }
        else
          return indent_text_auto_format(id, entry, insert);
     }
}
