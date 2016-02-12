#include "common.h"

void
console_text_set(Evas_Object *console, const char *text)
{
   elm_entry_entry_set(console, text);
}

static void
token_value_get(char *src, char *key_str, char end_key, int offset, char *dst)
{
   char *psrc = src;
   int count = 0;
   psrc += (strlen(key_str) + offset ) * sizeof(char);
   while (*psrc != end_key)
     dst[count++] = *psrc++;
   dst[count] = '\0';
}

static Eina_Bool
error_word_selection_anim_cb(void *data)
{
   Evas_Object *entry = data;
   const char *console_text = elm_entry_entry_get(entry);

   char error_word[1024];
   char error_line[1024];
   char *error_token, *edc_token;

   //parse edc line
   if (edc_token = strstr(console_text, "edc:"))
     token_value_get(edc_token, "edc:", ' ', 0, error_line);
   else return ECORE_CALLBACK_CANCEL;

   //parse error word
   if (error_token = strstr(console_text, "keyword"))
     token_value_get(error_token, "keyword", '<', 1, error_word);
   else if (error_token = strstr(console_text, "name"))
     token_value_get(error_token, "name", '<', 1, error_word);
   else return ECORE_CALLBACK_CANCEL;

    //find error word position
    const char *entry_text = enventor_object_text_get(base_enventor_get());
    const char *utf8 = elm_entry_markup_to_utf8(entry_text);

    enventor_object_line_goto(base_enventor_get(), atoi(error_line));
    int pos = enventor_object_cursor_pos_get(base_enventor_get());

    const char *search_line = utf8 + pos * sizeof(char);
    const char *matched = strstr(search_line, error_word);

    if (matched == NULL)
      return ECORE_CALLBACK_CANCEL;

    int start, end;
    start = matched - utf8;
    end = start + strlen(error_word);

    //select error word
    //here, we insert the syntax lock, unlock function
    //before and after selection region function
    //to prevent releasing the selection region.
    enventor_object_syntax_color_full_apply(base_enventor_get(), EINA_FALSE);
    enventor_object_select_region_set(base_enventor_get(), start, end);
    enventor_object_syntax_color_partial_apply(base_enventor_get(), 10);

   return ECORE_CALLBACK_CANCEL;
}

static void
console_mouse_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                      void *evnent_info)
{
   if (((Evas_Event_Mouse_Down*)evnent_info)->flags &
                 EVAS_BUTTON_DOUBLE_CLICK)
     ecore_animator_add(error_word_selection_anim_cb, obj);
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

   evas_object_event_callback_add(obj,
                                 EVAS_CALLBACK_MOUSE_DOWN,
                                 console_mouse_down_cb,
                                 NULL);

   return obj;
}
