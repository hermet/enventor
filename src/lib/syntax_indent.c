#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include "enventor_private.h"

struct indent_s
{
   Eina_Strbuf *strbuf;
   Evas_Object *entry;
   redoundo_data *rd;
};

typedef struct indent_line_s
{
   Eina_Stringshare *str;
   Eina_Bool indent_apply;
   int indent_depth;
} indent_line;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static int
indent_depth_get(indent_data *id EINA_UNUSED, char *src, int pos)
{
   const char *quot = "\"";
   const int quot_len = 1;

   if (!src || (pos < 1)) return 0;

   int depth = 0;
   char *cur = (char *) src;
   char *end = ((char *) src) + pos;

   while (cur && (cur < end))
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
indent_insert_br_case(indent_data *id)
{
   Evas_Object *entry = id->entry;
   Evas_Object *tb = elm_entry_textblock_get(entry);
   Evas_Textblock_Cursor *cur = evas_object_textblock_cursor_get(tb);
   const char *text = evas_textblock_cursor_paragraph_text_get(cur);
   char *utf8 = elm_entry_markup_to_utf8(text);
   Eina_Strbuf* diff = id->strbuf;
   eina_strbuf_reset(diff);
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
   redoundo_text_push(id->rd, eina_strbuf_string_get(diff), rd_cur_pos, 0,
                      EINA_FALSE);

   int space = indent_space_get(id);
   if (space <= 0) return;

   //Alloc Empty spaces
   char *p = alloca(space + 1);
   memset(p, ' ', space);
   p[space] = '\0';

   redoundo_text_push(id->rd, p, elm_entry_cursor_pos_get(entry), 0, EINA_TRUE);

   elm_entry_entry_insert(entry, p);
}

static void
indent_insert_bracket_case(indent_data *id, int cur_line)
{
   Evas_Object *entry = id->entry;
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

   int space = indent_space_get(id);
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

static Eina_List *
indent_code_line_list_create(indent_data *id EINA_UNUSED, const char *utf8)
{
   Eina_List *code_line_list = NULL;
   indent_line *code_line = NULL;

   char *utf8_ptr = NULL;
   char *utf8_end = NULL;
   char *utf8_lexem = NULL;
   char *utf8_appended = NULL;

   Eina_Bool keep_lexem = EINA_FALSE;

   //Single line comment begins in the beginning of the line.
   Eina_Bool single_comment_begin = EINA_FALSE;
   //Single line comment begins in the middle of the line.
   Eina_Bool single_comment_middle = EINA_FALSE;

   //Multi line comment begins in the beginning of the line.
   Eina_Bool multi_comment_begin = EINA_FALSE;
   //Multi line comment begins in the middle of the line.
   Eina_Bool multi_comment_middle = EINA_FALSE;
   //Multi line comment ends in the line.
   Eina_Bool multi_comment_end = EINA_FALSE;

   //Macro begins in the beginning of the line.
   Eina_Bool macro_begin = EINA_FALSE;
   //Macro begins in the middle of the line.
   Eina_Bool macro_middle = EINA_FALSE;

   int cur_indent_depth = 0;
   int new_indent_depth = 0;

   if (!utf8) return NULL;

   utf8_ptr = (char *)utf8;
   utf8_end = utf8_ptr + strlen(utf8);

   /* Create a list of code line strings from inserted string.
      Each code line string is generated based on lexeme.
      Here, lexeme starts with nonspace character and ends with newline. */
   for ( ; utf8_ptr < utf8_end; utf8_ptr++)
     {
        if (*utf8_ptr == '\n')
          {
             if (!keep_lexem)
               utf8_lexem = utf8_ptr;
             keep_lexem = EINA_FALSE;

             code_line = (indent_line *)calloc(1, sizeof(indent_line));

             char *append_begin = NULL;
             if (!utf8_appended) append_begin = (char *)utf8;

             if (single_comment_begin || multi_comment_begin ||
                 macro_begin || macro_middle)
               {
                  if (!append_begin)
                    append_begin = utf8_appended + 1;

                  code_line->indent_apply = EINA_FALSE;
               }
             else
               {
                  if (!append_begin)
                    append_begin = utf8_lexem;

                  //Newline only string does not need indentation.
                  if (utf8_ptr == append_begin)
                    code_line->indent_apply = EINA_FALSE;
                  else
                    code_line->indent_apply = EINA_TRUE;
               }
             code_line->str
                = eina_stringshare_add_length(append_begin,
                                              utf8_ptr - append_begin + 1);
             code_line->indent_depth = cur_indent_depth;
             cur_indent_depth = new_indent_depth;

             code_line_list = eina_list_append(code_line_list, code_line);
             utf8_appended = utf8_ptr;

             if (single_comment_begin)
               single_comment_begin = EINA_FALSE;
             if (single_comment_middle)
               single_comment_middle = EINA_FALSE;
             if (multi_comment_middle)
               {
                  multi_comment_middle = EINA_FALSE;
                  multi_comment_begin = EINA_TRUE;
               }
             if (multi_comment_end)
               {
                  multi_comment_begin = EINA_FALSE;
                  multi_comment_middle = EINA_FALSE;
                  multi_comment_end = EINA_FALSE;
               }

             //Check if macro ends.
             if (macro_begin || macro_middle)
               {
                  if (*(utf8_ptr - 1) != '\\')
                    {
                       macro_begin = EINA_FALSE;
                       macro_middle = EINA_FALSE;
                    }
               }
          }
        else if ((*utf8_ptr != ' ') && (*utf8_ptr != '\t') &&
                 (*utf8_ptr != '\r'))
          {
             if (!keep_lexem)
               {
                  utf8_lexem = utf8_ptr;
                  keep_lexem = EINA_TRUE;
               }

             //Check if current character is outside of line comment.
             if ((!single_comment_begin && !single_comment_middle) &&
                 ((!multi_comment_begin && !multi_comment_middle) ||
                  (multi_comment_end)))
               {
                  if ((*utf8_ptr == '/') && (utf8_ptr + 1 < utf8_end))
                    {
                       //Check if single line comment begins.
                       if (*(utf8_ptr + 1) == '/')
                         {
                            if (utf8_ptr == utf8_lexem)
                              single_comment_begin = EINA_TRUE;
                            else
                              single_comment_middle = EINA_TRUE;
                         }
                       //Check if multi line comment begins.
                       else if (*(utf8_ptr + 1) == '*')
                         {
                            if (utf8_ptr == utf8_lexem)
                              multi_comment_begin = EINA_TRUE;
                            else
                              multi_comment_middle = EINA_TRUE;
                         }
                    }
                  else if (*utf8_ptr == '#')
                    {
                       if (utf8_ptr == utf8_lexem)
                         macro_begin = EINA_TRUE;
                       else
                         macro_middle = EINA_TRUE;
                    }
                  else if (*utf8_ptr == '{')
                    new_indent_depth++;
                  else if (*utf8_ptr == '}')
                    {
                       new_indent_depth--;

                       /* Indentation depth decreases from the current code line
                          if string begins with '}'. */
                       if (utf8_ptr == utf8_lexem)
                         cur_indent_depth = new_indent_depth;
                    }
               }

             //Check if multi line comment ends.
             if (multi_comment_begin || multi_comment_middle)
               {
                  if ((*utf8_ptr == '*') && (utf8_ptr + 1 < utf8_end) &&
                      (*(utf8_ptr + 1) == '/'))
                    {
                       multi_comment_end = EINA_TRUE;
                    }
               }
          }
     }

   /* Append rest of the input string which does not end with newline. */
   if (!utf8_appended || (utf8_appended < utf8_end - 1))
     {
        char *append_begin = NULL;
        if (!utf8_appended) append_begin = (char *)utf8;

        code_line = (indent_line *)calloc(1, sizeof(indent_line));

        if (single_comment_begin || multi_comment_begin || macro_begin)
          {
             if (!append_begin)
               append_begin = utf8_appended + 1;

             code_line->indent_apply = EINA_FALSE;
          }
        else
          {
             if (!append_begin)
               {
                  //Only spaces are in the rest of the input string.
                  if (utf8_lexem <= utf8_appended)
                    {
                       append_begin = utf8_appended + 1;
                       code_line->indent_apply = EINA_FALSE;
                    }
                  //Non-space characters are in the rest of the input string.
                  else
                    {
                       append_begin = utf8_lexem;
                       code_line->indent_apply = EINA_TRUE;
                    }
               }
             else
               code_line->indent_apply = EINA_TRUE;
          }
        code_line->str
           = eina_stringshare_add_length(append_begin,
                                         utf8_end - append_begin);
        code_line->indent_depth = cur_indent_depth;

        code_line_list = eina_list_append(code_line_list, code_line);
     }

   return code_line_list;
}

static int
indent_text_auto_format(indent_data *id, const char *insert)
{
   int line_cnt = 0;
   //FIXME: To improve performance, change logic not to translate text.
   char *utf8 = evas_textblock_text_markup_to_utf8(NULL, insert);
   int utf8_size = strlen(utf8);

   Evas_Object *tb = elm_entry_textblock_get(id->entry);
   Evas_Textblock_Cursor *cur_start = evas_object_textblock_cursor_new(tb);
   Evas_Textblock_Cursor *cur_end = evas_object_textblock_cursor_get(tb);
   int tb_cur_pos = 0;

   Eina_List *code_line_list = indent_code_line_list_create(id, utf8);
   indent_line *code_line = NULL;
   free(utf8);
   if (!code_line_list) goto end;

   /* Check if indentation should be applied to the first code line.
      Indentation is applied if prior string has only spaces. */
   code_line= eina_list_data_get(code_line_list);
   if (code_line->indent_apply)
     {
        Evas_Textblock_Cursor *check_start
           = evas_object_textblock_cursor_new(tb);
        Evas_Textblock_Cursor *check_end
           = evas_object_textblock_cursor_new(tb);

        tb_cur_pos = evas_textblock_cursor_pos_get(cur_end);
        evas_textblock_cursor_pos_set(check_end, tb_cur_pos - utf8_size);
        evas_textblock_cursor_pos_set(check_start, tb_cur_pos - utf8_size);
        evas_textblock_cursor_line_char_first(check_start);

        char *check_range
           = evas_textblock_cursor_range_text_get(check_start, check_end,
                                                  EVAS_TEXTBLOCK_TEXT_PLAIN);

        evas_textblock_cursor_free(check_start);
        evas_textblock_cursor_free(check_end);

        Eina_Bool nonspace_found = EINA_FALSE;
        if (check_range)
          {
             int check_len = strlen(check_range);
             int index = 0;
             for ( ; index < check_len; index++)
               {
                  if (check_range[index] != ' ')
                    {
                       nonspace_found = EINA_TRUE;
                       break;
                    }
               }
             free(check_range);
          }
        if (nonspace_found)
          {
             code_line->indent_apply = EINA_FALSE;
          }
        else
          {
             int str_len = eina_stringshare_strlen(code_line->str);
             int index = 0;
             for ( ; index < str_len; index++)
               {
                  if ((code_line->str[index] != ' ') &&
                      (code_line->str[index] != '\t') &&
                      (code_line->str[index] != '\r'))
                    break;
               }
             if (index < str_len)
               {
                  char *new_str = strdup(&code_line->str[index]);
                  if (new_str)
                    {
                       eina_stringshare_del(code_line->str);
                       code_line->str = eina_stringshare_add(new_str);
                       free(new_str);
                    }
               }
          }
     }

   tb_cur_pos = evas_textblock_cursor_pos_get(cur_end);
   evas_textblock_cursor_pos_set(cur_start, tb_cur_pos - utf8_size);
   evas_textblock_cursor_range_delete(cur_start, cur_end);

   //Cancel last added diff, that was created when text pasted into entry.
   redoundo_n_diff_cancel(id->rd, 1);

   evas_textblock_cursor_line_char_first(cur_start);

   //Move cursor to the position where the inserted string will be prepended.
   code_line= eina_list_data_get(code_line_list);
   if (code_line->indent_apply)
     {
        evas_textblock_cursor_line_char_first(cur_start);
        int space_pos_start = evas_textblock_cursor_pos_get(cur_start);
        int space_pos_end = evas_textblock_cursor_pos_get(cur_end);

        if (space_pos_start < space_pos_end)
          {
             //Delete prior spaces.
             char *prior_space =
                evas_textblock_cursor_range_text_get(cur_start, cur_end,
                   EVAS_TEXTBLOCK_TEXT_MARKUP);

             if (prior_space)
               {
                  evas_textblock_cursor_range_delete(cur_start, cur_end);

                  tb_cur_pos = evas_textblock_cursor_pos_get(cur_end);
                  /* Add data about removal of prior spaces into the redoundo
                     queue. */
                  redoundo_text_push(id->rd, prior_space, tb_cur_pos, 0,
                                     EINA_FALSE);
                  free(prior_space);
               }
          }
     }
   else
     {
        tb_cur_pos = evas_textblock_cursor_pos_get(cur_end);
        evas_textblock_cursor_pos_set(cur_start, tb_cur_pos);
     }

   int space = indent_space_get(id);

   Eina_List *l = NULL;
   Eina_Strbuf *buf = id->strbuf;
   eina_strbuf_reset(buf);

   EINA_LIST_FOREACH(code_line_list, l, code_line)
     {
        Eina_Stringshare *line_str = code_line->str;

        if (code_line->indent_apply == EINA_FALSE)
          {
             eina_strbuf_append_printf(buf, "%s", line_str);
          }
        else
          {
             int cur_space = space + (code_line->indent_depth * TAB_SPACE);
             if (cur_space <= 0)
               {
                  eina_strbuf_append_printf(buf, "%s", line_str);
               }
             else
               {
                  char *p = alloca(cur_space + 1);
                  memset(p, ' ', cur_space);
                  p[cur_space] = '\0';
                  eina_strbuf_append_printf(buf, "%s%s", p, line_str);
                  memset(p, 0x0, cur_space);
               }
          }
        eina_stringshare_del(line_str);
        free(code_line);
     }
   eina_list_free(code_line_list);

   char *utf8_buf = eina_strbuf_string_steal(buf);
   char *newline_ptr = utf8_buf;
   line_cnt = 1;
   newline_ptr = strstr(newline_ptr, "\n");
   while (newline_ptr)
     {
        line_cnt++;
        newline_ptr = strstr(newline_ptr + 1, "\n");
     }

   //FIXME: To improve performance, change logic not to translate text.
   char *markup_buf = evas_textblock_text_utf8_to_markup(NULL, utf8_buf);
   free(utf8_buf);

   //Initialize cursor position to the beginning of the pasted string.
   tb_cur_pos = evas_textblock_cursor_pos_get(cur_start);
   evas_textblock_cursor_pos_set(cur_end, tb_cur_pos);

   evas_object_textblock_text_markup_prepend(cur_start, markup_buf);

   //Add data about formatted change into the redoundo queue.
   redoundo_text_push(id->rd, markup_buf, tb_cur_pos, 0, EINA_TRUE);
   free(markup_buf);

   //Update cursor position to the end of the pasted string.
   tb_cur_pos = evas_textblock_cursor_pos_get(cur_start);
   evas_textblock_cursor_pos_set(cur_end, tb_cur_pos);

end:
   evas_textblock_cursor_free(cur_start);
   return line_cnt;
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

indent_data *
indent_init(Eina_Strbuf *strbuf, edit_data *ed)
{
   indent_data *id = malloc(sizeof(indent_data));
   if (!id)
     {
        mem_fail_msg();
        return NULL;
     }
   id->strbuf = strbuf;
   id->entry = edit_entry_get(ed);
   id->rd = edit_redoundo_get(ed);

   if (!id->entry || !id->rd)
     EINA_LOG_ERR("Should be called after edit entry and redoundo is initialized!");

   return id;
}

void
indent_term(indent_data *id)
{
   free(id);
}

int
indent_space_get(indent_data *id)
{
   //Get the indentation depth
   int pos = elm_entry_cursor_pos_get(id->entry);
   char *src = elm_entry_markup_to_utf8(elm_entry_entry_get(id->entry));
   int space = indent_depth_get(id, src, pos);
   if (space < 0) space = 0;
   space *= TAB_SPACE;
   free(src);

   return space;
}

void
indent_delete_apply(indent_data *id, const char *del, int cur_line)
{
   if (del[0] != ' ') return;

   Evas_Object *tb = elm_entry_textblock_get(id->entry);
   Evas_Textblock_Cursor *cur = evas_object_textblock_cursor_new(tb);
   evas_textblock_cursor_line_set(cur, cur_line - 1);
   const char *text = evas_textblock_cursor_paragraph_text_get(cur);
   char *utf8 = elm_entry_markup_to_utf8(text);
   char *last_markup = NULL;
   Eina_Strbuf* diff = id->strbuf;
   eina_strbuf_reset(diff);

   int rd_cur_pos = evas_textblock_cursor_pos_get(cur);

   int len = 0;
   if (utf8) len = strlen(utf8);
   if (len <= 0) goto end;

   evas_textblock_cursor_paragraph_char_last(cur);
   last_markup = evas_textblock_cursor_content_get(cur);
   if (last_markup && !strncmp(last_markup, "<br/>", 5))
     evas_textblock_cursor_char_prev(cur);

   while (len > 0)
     {
        if ((utf8[(len - 1)] == ' '))
          {
             eina_strbuf_append(diff, evas_textblock_cursor_content_get(cur));
             evas_textblock_cursor_char_delete(cur);
             evas_textblock_cursor_char_prev(cur);
          }
        else break;
        len--;
     }
   redoundo_text_push(id->rd, eina_strbuf_string_get(diff), rd_cur_pos, 0,
                      EINA_FALSE);
   elm_entry_calc_force(id->entry);

end:
   evas_textblock_cursor_free(cur);
   if (utf8) free(utf8);
   if (last_markup) free(last_markup);
}


/* Check if indentation of input text is correct.
   Return EINA_TRUE if indentation is correct.
   Return EINA_FALSE if indentation is not correct. */
Eina_Bool
indent_text_check(indent_data *id EINA_UNUSED, const char *utf8)
{
   char *utf8_ptr = NULL;
   char *utf8_end = NULL;
   int utf8_size = 0;
   int depth = 0;
   int space = 0;
   Eina_Bool nonspace_found = EINA_FALSE;
   Eina_Bool comment_found = EINA_FALSE;
   Eina_Bool macro_found = EINA_FALSE;

   if (!utf8) return EINA_TRUE;

   utf8_ptr = (char *)utf8;
   utf8_size = strlen(utf8);
   utf8_end = (char *)utf8 + utf8_size;

   /* Check indentation spaces of each line.
      Indentation spaces are checked within line comment and macro. */
   while (utf8_ptr < utf8_end)
     {
        comment_found = EINA_FALSE;
        macro_found = EINA_FALSE;

        if (*utf8_ptr == '}')
          {
             depth--;
             if (depth < 0) depth = 0;
          }

        //Tab is not allowed.
        if (*utf8_ptr == '\t')
          return EINA_FALSE;
        else if (*utf8_ptr == ' ')
          {
             if (!nonspace_found)
               space++;
          }
        else if (*utf8_ptr == '\n')
          {
             //Do not allow space only line.
             if (!nonspace_found && (space > 0))
               return EINA_FALSE;

             //Do not allow newline at the end.
             if (utf8_ptr == utf8_end - 1)
               return EINA_FALSE;

             space = 0;
             nonspace_found = EINA_FALSE;
          }
        else if (*utf8_ptr != '\r')
          {
             char *comment_end = NULL;

             //Check line comment
             if ((*utf8_ptr == '/') && (utf8_ptr + 1 < utf8_end))
               {
                  //Start of single line comment.
                  if (*(utf8_ptr + 1) == '/')
                    {
                       comment_found = EINA_TRUE;

                       comment_end = strstr(utf8_ptr + 2, "\n");
                       if (comment_end) comment_end--;
                    }
                  //Start of multi line comment.
                  else if (*(utf8_ptr + 1) == '*')
                    {
                       comment_found = EINA_TRUE;

                       comment_end = strstr(utf8_ptr + 2, "*/");
                       if (comment_end) comment_end++;
                    }

                  if (comment_found)
                    {
                       if (comment_end)
                         utf8_ptr = comment_end;
                       else
                         return EINA_TRUE;
                    }
               }

             //Check macro
             else if (*utf8_ptr == '#')
               {
                  macro_found = EINA_TRUE;

                  char *macro_end = utf8_ptr;
                  while (macro_end)
                    {
                       char *backslash = strstr(macro_end + 1, "\\");
                       macro_end = strstr(macro_end + 1, "\n");
                       if (macro_end)
                         {
                            if (!backslash || (macro_end < backslash))
                              break;
                         }
                    }

                  if (macro_end)
                    {
                       macro_end--;
                       utf8_ptr = macro_end;
                    }
                  else
                    return EINA_TRUE;
               }

             if (comment_found || macro_found)
               {
                  if (!nonspace_found)
                    nonspace_found = EINA_TRUE;
               }
             //No line comment and No macro.
             else
               {
                  if (!nonspace_found)
                    {
                       if (space != depth * TAB_SPACE)
                         return EINA_FALSE;

                       nonspace_found = EINA_TRUE;
                    }

                  if (*utf8_ptr == '}')
                    {
                       if ((utf8_ptr + 1 < utf8_end) &&
                           (*(utf8_ptr + 1) != '\n'))
                         return EINA_FALSE;
                    }
                  else if (*utf8_ptr == ';')
                    {
                       if (utf8_ptr + 1 < utf8_end &&
                           *(utf8_ptr + 1) != '\n')
                         return EINA_FALSE;
                    }

                  if (*utf8_ptr == '{')
                    {
                       char *bracket_right_ptr = utf8_ptr + 1;
                       while (bracket_right_ptr < utf8_end)
                         {
                            if (*bracket_right_ptr != ' ' &&
                                *bracket_right_ptr != '\t')
                              break;
                            bracket_right_ptr++;
                         }
                       if (bracket_right_ptr < utf8_end &&
                           *bracket_right_ptr != '\n')
                         {
                            //Check block name after '{'.
                            Eina_Bool block_name_found = EINA_FALSE;

                            if (*bracket_right_ptr == '\"')
                              block_name_found = EINA_TRUE;
                            else if (bracket_right_ptr + 4 < utf8_end)
                              {
                                 if (!strncmp(bracket_right_ptr,
                                              "name:", 5))
                                   block_name_found = EINA_TRUE;
                                 else if (!strncmp(bracket_right_ptr,
                                                   "state:", 5))
                                   block_name_found = EINA_TRUE;
                              }

                            if (!block_name_found)
                              return EINA_FALSE;
                         }
                    }
               }
          }

        if (!comment_found && !macro_found && (*utf8_ptr == '{'))
          depth++;

        utf8_ptr++;
     }

   return EINA_TRUE;
}

/* Create indented markup text from input utf8 text.
   Count the number of lines of indented text.
   Return created indented markup text. */
char *
indent_text_create(indent_data *id, const char *utf8, int *indented_line_cnt)
{
   if (!utf8)
     {
        if (indented_line_cnt) *indented_line_cnt = 0;
        return NULL;
     }

   Eina_List *code_line_list = indent_code_line_list_create(id, utf8);
   if (!code_line_list)
     {
        if (indented_line_cnt) *indented_line_cnt = 0;
        return NULL;
     }

   indent_line *code_line = NULL;
   Eina_List *l = NULL;
   Eina_Strbuf *buf = id->strbuf;
   eina_strbuf_reset(buf);

   EINA_LIST_FOREACH(code_line_list, l, code_line)
     {
        Eina_Stringshare *line_str = code_line->str;

        if (!code_line->indent_apply)
          {
             eina_strbuf_append_printf(buf, "%s", line_str);
          }
        else
          {
             int space = code_line->indent_depth * TAB_SPACE;
             if (space <= 0)
               {
                  eina_strbuf_append_printf(buf, "%s", line_str);
               }
             else
               {
                  char *p = alloca(space + 1);
                  memset(p, ' ', space);
                  p[space] = '\0';
                  eina_strbuf_append_printf(buf, "%s%s", p, line_str);
                  memset(p, 0x0, space);
               }
          }
        eina_stringshare_del(line_str);
        free(code_line);
     }
   eina_list_free(code_line_list);

   char *utf8_buf = eina_strbuf_string_steal(buf);
   char *newline_ptr = utf8_buf;
   int line_cnt = 1;
   newline_ptr = strstr(newline_ptr, "\n");
   while (newline_ptr)
     {
        line_cnt++;
        newline_ptr = strstr(newline_ptr + 1, "\n");
     }

   //FIXME: This translation may cause low performance.
   char *indented_markup = evas_textblock_text_utf8_to_markup(NULL, utf8_buf);
   free(utf8_buf);

   if (indented_line_cnt) *indented_line_cnt = line_cnt;
   return indented_markup;
}

int
indent_insert_apply(indent_data *id, const char *insert, int cur_line)
{
   int len = strlen(insert);
   if (len == 0)
     {
        return 0;
     }
   else if (len == 1)
     {
        if (insert[0] == '}')
          indent_insert_bracket_case(id, cur_line);
        return 0;
     }
   else
     {
        if (!strcmp(insert, EOL))
          {
             indent_insert_br_case(id);
             return 1;
          }
        else if (!strcmp(insert, QUOT))
          return 0;
        else if (!strcmp(insert, LESS))
          return 0;
        else if (!strcmp(insert, GREATER))
          return 0;
        else if (!strcmp(insert, AMP))
          return 0;
        else
          {
             int increase = indent_text_auto_format(id, insert);
             if (increase > 0) increase--;
             return increase;
          }
     }
}
