#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include "enventor_private.h"

/* This is a node for a list structure.
   Each node has its parent name list. */
typedef struct keyword_s
{
   char *name;
   char *desc;
   Eina_List *parent_name_list; //Parent keyword name list
} keyword_data;

typedef struct ref_s
{
   Eina_File *source_file;

   Eina_List *keyword_list; //keyword_data list

   char *keyword_name;
   char *keyword_desc;

   edit_data *ed;
   Evas_Object *event_rect;
   Evas_Object *layout;
} ref_data;

static ref_data *g_md = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void keyword_data_free(keyword_data **keyword);
static void keyword_list_free(Eina_List **keyword_list);
static void ref_event_rect_delete(void);
static void ref_layout_delete(void);

static void
keyword_data_free(keyword_data **keyword)
{
   if (!(*keyword)) return;

   if ((*keyword)->name)
     free((*keyword)->name);
   if ((*keyword)->desc)
     free((*keyword)->desc);

   char *parent_name;
   EINA_LIST_FREE((*keyword)->parent_name_list, parent_name)
     {
        free(parent_name);
     }
}

static void
keyword_list_free(Eina_List **keyword_list)
{
   if (!(*keyword_list)) return;

   keyword_data *keyword;
   EINA_LIST_FREE(*keyword_list, keyword)
     {
        keyword_data_free(&keyword);
     }
}

static char *
cursor_keyword_name_find(Evas_Object *entry)
{
   Evas_Object *tb = NULL;
   Evas_Textblock_Cursor *cur_orig = NULL;
   Evas_Textblock_Cursor *cur_begin = NULL;
   Evas_Textblock_Cursor *cur_end = NULL;
   int cur_orig_pos = 0;
   int cur_begin_pos = 0;
   char *cur_orig_ptr = NULL;
   char *cur_begin_ptr = NULL;
   char *cur_end_ptr = NULL;
   char *cur_text = NULL;
   char *keyword_name = NULL;

   tb = elm_entry_textblock_get(entry);
   cur_orig = evas_object_textblock_cursor_get(tb);
   if (!cur_orig) return NULL;

   //Show keyword reference only if cursor is located in a correct keyword.
   cur_text = evas_textblock_cursor_content_get(cur_orig);
   if (!cur_text || (!isalnum(*cur_text) && (*cur_text != '_'))) goto end;
   free(cur_text);
   cur_text = NULL;
   cur_orig_pos = evas_textblock_cursor_pos_get(cur_orig);

   //Find beginning cursor position of the keyword.
   cur_begin = evas_object_textblock_cursor_new(tb);
   if (!cur_begin) goto end;

   evas_textblock_cursor_pos_set(cur_begin, cur_orig_pos);
   if (!evas_textblock_cursor_word_start(cur_begin)) goto end;

   //Find ending cursor position of the keyword.
   cur_end = evas_object_textblock_cursor_new(tb);
   if (!cur_end) goto end;

   evas_textblock_cursor_pos_set(cur_end, cur_orig_pos);
   if (!evas_textblock_cursor_word_end(cur_end)) goto end;

   /* Move ending cursor by one character because cursor_range_text_get does
      not include character of ending cursor. */
   evas_textblock_cursor_char_next(cur_end);
   cur_text = evas_textblock_cursor_range_text_get(cur_begin, cur_end,
                                                   EVAS_TEXTBLOCK_TEXT_PLAIN);
   if (!cur_text) goto end;

   //Need to check if cursor text contains special character such as '.'.
   cur_begin_pos = evas_textblock_cursor_pos_get(cur_begin);
   cur_orig_ptr = cur_text + (cur_orig_pos - cur_begin_pos);

   //Find valid keyword beginning position from cursor text.
   char *cur_ptr = cur_orig_ptr;
   while (cur_ptr >= cur_text)
     {
        if (isalnum(*cur_ptr) || (*cur_ptr == '_'))
          cur_begin_ptr = cur_ptr;
        else
          break;

        cur_ptr--;
     }

   //Find valid keyword ending position from cursor text.
   cur_ptr = cur_orig_ptr;
   char *cur_text_end = cur_text + strlen(cur_text) - 1;
   while (cur_ptr <= cur_text_end)
     {
        if (isalnum(*cur_ptr) || (*cur_ptr == '_'))
          cur_end_ptr = cur_ptr;
        else
          break;

        cur_ptr++;
     }

   keyword_name = strndup(cur_begin_ptr, (cur_end_ptr - cur_begin_ptr + 1));

end:
   if (cur_begin) evas_textblock_cursor_free(cur_begin);
   if (cur_end) evas_textblock_cursor_free(cur_end);
   if (cur_text) free(cur_text);

   return keyword_name;
}

/* Check two string lists have same strings.
   Return EINA_TRUE if lists are same.
   Return EINA_FALSE if lists are not same. */
static Eina_Bool
str_list_same_check(Eina_List *str_list1, Eina_List *str_list2)
{
   if (!str_list1 && !str_list2)
     return EINA_TRUE;

   if ((!str_list1 && str_list2) || (str_list1 && !str_list2))
     return EINA_FALSE;

   if (eina_list_count(str_list1) != eina_list_count(str_list2))
     return EINA_FALSE;

   while (str_list1 && str_list2)
     {
        char *str1 = eina_list_data_get(str_list1);
        char *str2 = eina_list_data_get(str_list2);

        if (!str1 || !str2) return EINA_FALSE;
        if (strcmp(str1, str2)) return EINA_FALSE;

        str_list1 = eina_list_next(str_list1);
        str_list2 = eina_list_next(str_list2);
     }

   return EINA_TRUE;
}

static Eina_List *
keyword_parent_name_list_find(const char *text, const char *keyword_name)
{
   Eina_List *parent_name_list = NULL;

   if (!text) return NULL;
   if (!keyword_name) return NULL;

   //Check from the end of the text.
   char *ptr = (char *)(text + ((strlen(text) - 1) * sizeof(char)));
   int height = 0;
   int next_height = height + 1;

   const char *parent_begin = NULL;
   const char *parent_end = NULL;

   //Check if dot('.') grammar is valid to identify parent keyword.
   Eina_Bool dot_grammar_valid = EINA_TRUE;

   while (text <= ptr)
     {
        if ((*ptr == '{') || (dot_grammar_valid && (*ptr == '.')))
          {
             if (dot_grammar_valid)
               dot_grammar_valid = EINA_FALSE;

             height++;
             if (height == next_height)
               {
                  ptr--;
                  if (text > ptr) break;

                  parent_begin = NULL;
                  parent_end = NULL;
                  while (text <= ptr)
                    {
                       if (isspace(*ptr))
                         {
                            if (parent_end)
                              {
                                 parent_begin = ptr + 1;
                                 break;
                              }
                         }
                       else
                         {
                            if (!parent_end)
                              parent_end = ptr;
                         }
                       ptr--;
                    }
                  if (!parent_end)
                    continue;

                  if (parent_end && !parent_begin)
                    parent_begin = text;

                  char *parent_name = strndup(parent_begin,
                                              (parent_end - parent_begin + 1));
                  parent_name_list = eina_list_append(parent_name_list,
                                                      parent_name);
                  next_height++;
               }
          }
        else if (*ptr == '}')
          {
             if (dot_grammar_valid)
               dot_grammar_valid = EINA_FALSE;

             height--;
          }
        else if (dot_grammar_valid && !isalnum(*ptr))
          {
             dot_grammar_valid = EINA_FALSE;
          }
        ptr--;
     }

   return parent_name_list;
}

static keyword_data *
cursor_keyword_data_find(Evas_Object *entry, Eina_List *keyword_list,
                         const char *keyword_name)
{
   keyword_data *found_keyword = NULL;

   if (!entry) return NULL;
   if (!keyword_list) return NULL;
   if (!keyword_name) return NULL;

   //Get text before cursor position.
   Evas_Object *tb = elm_entry_textblock_get(entry);
   Evas_Object *cur_begin = evas_object_textblock_cursor_new(tb);
   Evas_Object *cur_end = evas_object_textblock_cursor_get(tb);
   char *utf8_text = NULL;
   Eina_List *parent_name_list = NULL;

   /* FIXME: Getting text from range with EVAS_TEXTBLOCK_TEXT_PLAIN generates
      garbage characters.
      So please use EVAS_TEXTBLOCK_TEXT_PLAIN after the bug is fixed. */
   char *markup_text =
      evas_textblock_cursor_range_text_get(cur_begin, cur_end,
                                           EVAS_TEXTBLOCK_TEXT_MARKUP);
   if (!markup_text) goto end;

   utf8_text = evas_textblock_text_markup_to_utf8(NULL, markup_text);
   free(markup_text);
   markup_text = NULL;

   //Find parent name list of the selected keyword in text.
   parent_name_list = keyword_parent_name_list_find((const char *)utf8_text,
                                                    keyword_name);
   Eina_List *l;
   keyword_data *keyword;
   EINA_LIST_FOREACH(keyword_list, l, keyword)
     {
        //Find a keyword which has the same name and the same parent names.
        if (!strcmp(keyword->name, keyword_name) &&
            str_list_same_check(keyword->parent_name_list, parent_name_list))
          {
             found_keyword = keyword;
             break;
          }
     }

end:
   if (cur_begin) evas_textblock_cursor_free(cur_begin);
   if (utf8_text) free(utf8_text);
   if (parent_name_list) eina_list_free(parent_name_list);

   return found_keyword;
}

static void
keyword_list_load(Eina_List **keyword_list, Eina_List **parent_name_list,
                  char **ptr)
{
   if (!keyword_list) return;
   if (!parent_name_list) return;
   if (!*ptr) return;

   int len = strlen(*ptr);
   const char *text_end = *ptr + len;

   while (*ptr < text_end)
     {
        keyword_data *keyword = NULL;
        char *keyword_name = NULL;
        char *keyword_desc = NULL;
        char *block_begin = NULL;
        char *block_end = NULL;
        char *parent_name = NULL;

        //Find beginning position of keyword block to check pointer.
        block_begin = strstr(*ptr, "{");
        if (!block_begin) break;

        //Find ending position of keyword block to check pointer.
        block_end = strstr(*ptr, "}");
        if (!block_end) break;

        if (block_begin > block_end) break;

        //Find keyword name.
        char *keyword_name_begin = NULL;
        char *keyword_name_end = NULL;

        for ( ; *ptr < block_begin; (*ptr)++)
          {
             if (!isspace(**ptr))
               {
                  if (!keyword_name_begin)
                    {
                       keyword_name_begin = *ptr;
                       keyword_name_end = *ptr;
                    }
                  else
                    keyword_name_end = *ptr;
               }
          }
        if (!keyword_name_begin || !keyword_name_end) break;

        keyword_name = strndup(keyword_name_begin,
                               (keyword_name_end - keyword_name_begin + 1));
        if (!keyword_name) break;
        (*ptr)++; //Move pointer position after "{".

        //Find keyword description.
        char *keyword_desc_begin = NULL;
        char *keyword_desc_end = NULL;

        keyword_desc_begin = strstr(*ptr, "\"");
        if (!keyword_desc_begin)
          {
             free(keyword_name);
             break;
          }
        keyword_desc_begin++;

        keyword_desc_end = strstr(keyword_desc_begin, "\";");
        if (!keyword_desc_end)
          {
             free(keyword_name);
             break;
          }
        keyword_desc_end--;

        keyword_desc =
           strndup((const char *)keyword_desc_begin,
                   (keyword_desc_end - keyword_desc_begin + 1));

        if (!keyword_desc)
          {
             free(keyword_name);
             break;
          }
        *ptr = keyword_desc_end + 3; //Move pointer position after "\";".

        //Set parent name list.
        parent_name = keyword_name;
        *parent_name_list = eina_list_append(*parent_name_list,
                                             parent_name);

        //Set child keyword list recursively.
        keyword_list_load(keyword_list, parent_name_list, ptr);

        //Find ending position of keyword block to update pointer.
        block_end = strstr(*ptr, "}");
        if (!block_end)
          {
             free(keyword_name);
             free(keyword_desc);
             break;
          }
        *ptr = block_end + 1; //Move pointer position after "}".

        //Remove current keyword name from parent name list.
        *parent_name_list = eina_list_remove(*parent_name_list, parent_name);

        //Create a new keyword data and Append it to keyword list.
        keyword = calloc(1, sizeof(keyword_data));
        keyword->name = keyword_name;
        keyword->desc = keyword_desc;
        if (*parent_name_list)
          {
             Eina_List *new_parent_name_list = NULL;
             Eina_List *l = NULL;
             EINA_LIST_REVERSE_FOREACH(*parent_name_list, l, parent_name)
               {
                  new_parent_name_list = eina_list_append(new_parent_name_list,
                                                          strdup(parent_name));
               }
             keyword->parent_name_list = new_parent_name_list;
          }
        *keyword_list = eina_list_append(*keyword_list, keyword);
     }
}

static void
ref_load(ref_data *md)
{
   if (!md) return;

   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/reference/reference.src",
            eina_prefix_data_get(PREFIX));

   //Open source file.
   md->source_file = eina_file_open((const char *)buf, EINA_FALSE);
   if (!md->source_file) return;

   //Load text from source file.
   char *text = (char *)eina_file_map_all(md->source_file, EINA_FILE_POPULATE);
   if (!text) goto end;

   if (md->keyword_list)
     {
        keyword_list_free(&(md->keyword_list));
        md->keyword_list = NULL;
     }

   //Load keyword data list from text.
   Eina_List *keyword_list = NULL;
   Eina_List *parent_name_list = NULL;
   char *ptr = text;
   keyword_list_load(&keyword_list, &parent_name_list, &ptr);
   md->keyword_list = keyword_list;

end:
   if (text) eina_file_map_free(md->source_file, text);

   eina_file_close(md->source_file);
   md->source_file = NULL;
}

static void
event_rect_mouse_down_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                         Evas_Object *obj EINA_UNUSED,
                         void *event_info EINA_UNUSED)
{
   ref_event_rect_delete();
   ref_layout_delete();
}

static void
entry_unfocused_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   ref_event_rect_delete();
   ref_layout_delete();
}

static void
entry_move_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   ref_event_rect_delete();
   ref_layout_delete();
}

static void
entry_changed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   ref_event_rect_delete();
   ref_layout_delete();
}

static void
entry_changed_user_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   ref_event_rect_delete();
   ref_layout_delete();
}

static void
entry_cursor_changed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                        void *event_info EINA_UNUSED)
{
   ref_event_rect_delete();
   ref_layout_delete();
}

static void
entry_cursor_changed_manual_cb(void *data EINA_UNUSED,
                               Evas_Object *obj EINA_UNUSED,
                               void *event_info EINA_UNUSED)
{
   ref_event_rect_delete();
   ref_layout_delete();
}

static void
ref_event_rect_delete(void)
{
   ref_data *md = g_md;
   if (!md) return;

   if (!md->event_rect) return;
   evas_object_del(md->event_rect);
   md->event_rect = NULL;
}

static void
ref_layout_delete(void)
{
   ref_data *md = g_md;
   if (!md) return;

   if (!md->layout) return;
   evas_object_del(md->layout);
   md->layout = NULL;

   if (!md->ed) return;
   //Delete entry callbacks to delete reference layout.
   Evas_Object *edit_entry = edit_entry_get(md->ed);
   evas_object_event_callback_del(edit_entry, EVAS_CALLBACK_MOVE,
                                  entry_move_cb);
   evas_object_smart_callback_del(edit_entry, "unfocused", entry_unfocused_cb);
   evas_object_smart_callback_del(edit_entry, "changed", entry_changed_cb);
   evas_object_smart_callback_del(edit_entry, "changed,user",
                                  entry_changed_user_cb);
   evas_object_smart_callback_del(edit_entry, "cursor,changed",
                                  entry_cursor_changed_cb);
   evas_object_smart_callback_del(edit_entry, "cursor,changed,manual",
                                  entry_cursor_changed_manual_cb);
}

static Evas_Object *
ref_event_rect_create(Evas_Object *edit_obj)
{
   if (!edit_obj) return NULL;

   //event_rect catches mouse down event, which delete reference layout.
   Evas_Object *win = elm_object_top_widget_get(edit_obj);
   Evas *e = evas_object_evas_get(win);
   Evas_Object *rect = evas_object_rectangle_add(e);
   evas_object_repeat_events_set(rect, EINA_TRUE);
   evas_object_color_set(rect, 0, 0, 0, 0);

   evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_DOWN,
                                  event_rect_mouse_down_cb, NULL);
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, rect);
   evas_object_show(rect);

   return rect;
}

static Evas_Object *
ref_layout_create(Evas_Object *edit_entry, const char *desc)
{
   if (!edit_entry) return NULL;
   if (!desc) return NULL;

   Evas_Object *layout = elm_layout_add(edit_entry);
   if (!layout)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }
   elm_layout_file_set(layout, EDJE_PATH, "reference_layout");

   //Do not apply edit entry's font scale to reference layout.
   elm_object_scale_set(layout, 1.0);

   Evas_Object *entry = elm_entry_add(layout);
   elm_object_style_set(entry, "enventor");
   elm_entry_editable_set(entry, EINA_FALSE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_entry_entry_set(entry, desc);

   elm_object_part_content_set(layout, "elm.swallow.content", entry);
   evas_object_show(layout);

   //Add entry callbacks to delete reference layout.
   evas_object_event_callback_add(edit_entry, EVAS_CALLBACK_MOVE, entry_move_cb,
                                  NULL);
   evas_object_smart_callback_add(edit_entry, "unfocused", entry_unfocused_cb,
                                  NULL);
   evas_object_smart_callback_add(edit_entry, "changed", entry_changed_cb,
                                  NULL);
   evas_object_smart_callback_add(edit_entry, "changed,user",
                                  entry_changed_user_cb, NULL);
   evas_object_smart_callback_add(edit_entry, "cursor,changed",
                                  entry_cursor_changed_cb, NULL);
   evas_object_smart_callback_add(edit_entry, "cursor,changed,manual",
                                  entry_cursor_changed_manual_cb, NULL);

   return layout;
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
ref_show(edit_data *ed)
{
   ref_data *md = g_md;
   Evas_Object *edit_obj = NULL;
   Evas_Object *edit_entry = NULL;

   if (!md) return;
   if (!ed) return;
   md->ed = ed;

   //Return if reference layout is already shown.
   if (md->event_rect || md->layout) return;

   edit_obj = edit_obj_get(ed);
   edit_entry = edit_entry_get(ed);

   //Set keyword name.
   if (md->keyword_name) free(md->keyword_name);
   md->keyword_name = cursor_keyword_name_find(edit_entry);
   if (!md->keyword_name) return;

   //Set keyword desc.
   if (md->keyword_desc)
     {
        free(md->keyword_desc);
        md->keyword_desc = NULL;
     }
   keyword_data *keyword = cursor_keyword_data_find(edit_entry,
                                                    md->keyword_list,
                                                    md->keyword_name);
   if (keyword)
     md->keyword_desc = strdup(keyword->desc);
   if (!md->keyword_desc) return;

   //Create event rect which catches mouse down event.
   md->event_rect = ref_event_rect_create(edit_obj);

   //Create reference layout.
   md->layout = ref_layout_create(edit_entry, md->keyword_desc);

   //Calculate reference layout position.
   Evas_Coord obj_x, obj_y, obj_w, obj_h;
   evas_object_geometry_get(edit_obj, &obj_x, &obj_y, &obj_w, &obj_h);

   Evas_Coord entry_x;
   evas_object_geometry_get(edit_entry, &entry_x, NULL, NULL, NULL);

   Evas_Coord cur_x, cur_y, cur_h;
   elm_entry_cursor_geometry_get(edit_entry, &cur_x, &cur_y, NULL, &cur_h);

   char *layout_width =
      (char *)edje_object_data_get(elm_layout_edje_get(md->layout), "width");
   char *layout_height =
      (char *)edje_object_data_get(elm_layout_edje_get(md->layout), "height");
   if (!layout_width || !layout_height) return;

   const Evas_Coord ref_w = ELM_SCALE_SIZE(atoi(layout_width));
   const Evas_Coord ref_h = ELM_SCALE_SIZE(atoi(layout_height));
   Evas_Coord ref_x = 0;
   Evas_Coord ref_y = 0;

   //The center of reference layout is the entry cursor position.
   if (cur_x < (ref_w / 2))
     ref_x = obj_x;
   else if ((obj_w - (entry_x - obj_x + cur_x)) < (ref_w / 2))
     ref_x = obj_x + obj_w - ref_w;
   else
     ref_x = entry_x + cur_x - (ref_w / 2);

   if ((obj_h - cur_y - cur_h) < ref_h)
     ref_y = obj_y + cur_y - ref_h;
   else
     ref_y = obj_y + cur_y + cur_h;

   evas_object_move(md->layout, ref_x, ref_y);
}

void
ref_init(void)
{
   ref_data *md = calloc(1, sizeof(ref_data));
   if (!md)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }

   ref_load(md);

   g_md = md;
}

void
ref_term(void)
{
   ref_data *md = g_md;

   keyword_list_free(&(md->keyword_list));

   if (md->keyword_name) free(md->keyword_name);
   if (md->keyword_desc) free(md->keyword_desc);

   ref_event_rect_delete();
   ref_layout_delete();

   free(md);
   g_md = NULL;
}
