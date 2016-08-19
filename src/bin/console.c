#include "common.h"

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
token_value_get(char *src, char *key_str, char end_key, int offset, char *dst)
{
   char *psrc = src;
   int count = 0;
   psrc += strlen(key_str) + offset;
   while (*psrc != end_key)
     dst[count++] = *psrc++;
   dst[count] = '\0';
}

static void
error_word_select(Evas_Object *console)
{
   //Convert console text including markup text to the plain text
   const char *markup_text = elm_entry_entry_get(console);
   if (!markup_text) return;
   char *console_text = elm_entry_markup_to_utf8(markup_text);
   if (!console_text) return;

   char error_word[1024];
   char error_line[1024];
   char *error_token, *edc_token;

   //Parse edc line
   if ((edc_token = strstr(console_text, "edc : ")))
     token_value_get(edc_token, "edc : ", ' ', 0, error_line);
   else
     goto end;

   //Parse error word
   if ((error_token = strstr(console_text, "keyword")))
     token_value_get(error_token, "keyword", ' ', 1, error_word);
   else if ((error_token = strstr(console_text, "name")))
     token_value_get(error_token, "name", ' ', 1, error_word);
   else
     goto end;

   //FIXME: Need to get the file that contains errors.
   Enventor_Item *it = file_mgr_focused_item_get();
   EINA_SAFETY_ON_NULL_RETURN(it);

   //Find error word position
   enventor_item_line_goto(it, atoi(error_line));
   int pos = enventor_item_cursor_pos_get(it);
   const char *entry_text = enventor_item_text_get(it);
   char *utf8 = elm_entry_markup_to_utf8(entry_text);
   if(!utf8) goto end;

   const char *search_line = utf8 + pos;
   const char *matched = strstr(search_line, error_word);
   if (!matched)
     {
        free(utf8);
        goto end;
     }

   int start, end;
   start = matched - utf8;
   end = start + strlen(error_word);

   //Select error word
   enventor_item_select_region_set(it, start, end);
   free(utf8);

end:
   free(console_text);
}

static void
set_console_error_msg(Evas_Object *console, const char *src)
{
   /* We cut error messages since it contains unnecessary information.
      Most of the time, first one line has a practical information. */
   const char *new_line  = "<br/>";
   const char *eol = strstr(src, new_line);
   if (!eol) return;

   char * single_error_msg = alloca((eol - src) + 1);
   if (!single_error_msg) return;

   strncpy(single_error_msg, src, eol - src);
   single_error_msg[eol - src] = '\0';

   char *color_msg = error_msg_syntax_color_set(single_error_msg);
   elm_entry_entry_set(console, color_msg);
   free(color_msg);
}

char*
error_msg_syntax_color_set(char *text)
{
   char *color_error_msg;
   const char color_end[] = "</color>";
   const char color_red[] = "<color=#FF4848>";
   const char color_green[] = "<color=#5CD1E5>";
   const char color_yellow[] = "<color=#FFBB00>";

   color_error_msg = (char *)calloc(1024, sizeof(char));
   char *token = strtok(text, " ");
   while (token != NULL)
     {
        if (strstr(token, "edje_cc:"))
          {
             strncat(color_error_msg, color_red, 15);
             strncat(color_error_msg, token, strlen(token));
             strncat(color_error_msg, color_end, 8);
          }
        else if (strstr(token, "Error"))
          {
             strncat(color_error_msg, color_red, 15);
             strncat(color_error_msg, token, strlen(token));
             strncat(color_error_msg, color_end, 8);
          }
        else if (strstr(token, ".edc"))
          {
             strncat(color_error_msg, color_yellow, 15);
             if (strstr(strstr(token, ".edc"), ":"))
               {
                  char *number = strstr(strstr(token, ".edc"), ":");
                  int len = strlen(token) - strlen(number);
                  strncat(color_error_msg, token, len);
                  strncat(color_error_msg, color_end, 8);
                  strncat(color_error_msg, " : ", 3);
                  strncat(color_error_msg, color_green, 15);
                  strncat(color_error_msg, number + 1, strlen(number) - 1);
                  strncat(color_error_msg, color_end, 8);
               }
             else
               {
                  strncat(color_error_msg, token, strlen(token));
                  strncat(color_error_msg, color_end, 8);
               }
          }
        else
          {
             strncat(color_error_msg, token, strlen(token));
          }
        strncat(color_error_msg, " ", 1);
        token = strtok(NULL, " ");
     }
   return color_error_msg;
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
console_text_set(Evas_Object *console, const char *text)
{
   set_console_error_msg(console, text);
   error_word_select(console);
}

Evas_Object *
console_create(Evas_Object *parent)
{
   Evas_Object *obj = elm_entry_add(parent);
   elm_entry_scrollable_set(obj, EINA_TRUE);
   elm_entry_editable_set(obj, EINA_FALSE);
   elm_entry_line_wrap_set(obj, ELM_WRAP_WORD);
   elm_object_focus_allow_set(obj, EINA_FALSE);
   evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);

   return obj;
}
