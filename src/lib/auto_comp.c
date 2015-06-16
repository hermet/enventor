#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include "enventor_private.h"

#define QUEUE_SIZE 20
#define COMPSET_PAIR_MINIMUM 1

typedef struct lexem_s
{
   Eina_List *nodes;
   char **txt;
   int txt_count;
   int cursor_offset;
   int line_back;
   char *name;
} lexem;

typedef struct autocomp_s
{
   char queue[QUEUE_SIZE];
   int queue_pos;
   const lexem *lexem_root;
   lexem *lexem_ptr;
   Eet_File *source_file;
   edit_data *ed;
   Evas_Object *anchor;
   Evas_Object *list;
   Ecore_Thread *init_thread;
   Eina_Bool anchor_visible : 1;
   Eina_Bool initialized : 1;
   Eina_Bool enabled : 1;
} autocomp_data;

static autocomp_data *g_ad = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/
static Eet_Data_Descriptor *lex_desc = NULL;

static void
eddc_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, lexem);
   lex_desc = eet_data_descriptor_file_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_LIST(lex_desc, lexem, "nodes", nodes, lex_desc);

   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY_STRING(lex_desc, lexem, "txt", txt);
   EET_DATA_DESCRIPTOR_ADD_BASIC(lex_desc, lexem, "cursor_offset", cursor_offset, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(lex_desc, lexem, "line_back", line_back, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(lex_desc, lexem, "name", name, EET_T_STRING);
}

static void
autocomp_load(autocomp_data *ad)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/autocomp/autocomp.eet",
            eina_prefix_data_get(PREFIX));

   if (ad->source_file)
     {
        if (lex_desc)
          eet_data_descriptor_free(lex_desc);
        eet_close(ad->source_file);

     }
   ad->source_file = eet_open(buf, EET_FILE_MODE_READ);

   ad->lexem_root = (lexem *)eet_data_read(ad->source_file, lex_desc, "node");
   ad->lexem_ptr = (lexem *)ad->lexem_root;
}

static void
init_thread_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   autocomp_data *ad = data;
   eddc_init();
   autocomp_load(ad);
}

static lexem *
context_lexem_get(autocomp_data *ad, Evas_Object *entry, int cur_pos)
{

   Eina_Bool find_flag = EINA_FALSE;
   Eina_List *l = NULL;
   Eina_List *nodes = ad->lexem_root->nodes;
   lexem *data = (lexem *)ad->lexem_root;
   if (cur_pos <= 1) return data;

   const char *text = elm_entry_entry_get(entry);
   if (!text) return data;
   int i = 0;

   char *utf8 = elm_entry_markup_to_utf8(text);
   if (!utf8) return data;

   char *cur = utf8;
   char *end = cur + cur_pos;
   char stack[20][40];
   int depth = 0;
   char *help_ptr = NULL;
   char *help_end_ptr = NULL;

   const char *quot = QUOT_UTF8;
   const int quot_len = QUOT_UTF8_LEN;
   int quot_cnt = 0;

   while (cur <= end)
     {
        if ((cur!=end) && (!strncmp(cur, quot, quot_len)))
          quot_cnt++;

        if (*cur == '{')
          {
             for (help_end_ptr = cur;
                  !isalnum(*help_end_ptr);
                  help_end_ptr--);
             for (help_ptr = help_end_ptr;
                  (((isalnum(*help_ptr )) || (*help_ptr == '_')));
                  help_ptr--);
             if (help_ptr != utf8)
               help_ptr++;

             memset(stack[depth], 0x0, 40);
             strncpy(stack[depth], help_ptr, help_end_ptr - help_ptr + 1);
             depth++;
          }
        if (*cur == '}')
          {
             memset(stack[depth], 0x0, 40);
             depth--;
          }
        cur++;
     }

   free(utf8);

   if (quot_cnt % 2) return NULL;

   for (i = 0; i < depth; i++)
     {
        find_flag = EINA_FALSE;
        EINA_LIST_FOREACH(nodes, l, data)
          {
             if (!strncmp(stack[i], data->name, strlen(data->name)))
               {
                  nodes = data->nodes;
                  l = NULL;
                  find_flag = EINA_TRUE;
                  break;
               }
          }
        if (!find_flag) return NULL;
     }
   return data;
}

static void
context_changed(autocomp_data *ad, Evas_Object *edit)
{
   if ((!ad->enabled) || (!ad->initialized)) return;

   int cursor_position = elm_entry_cursor_pos_get(edit);

   if (!cursor_position)
     {
        ad->lexem_ptr = (lexem *)ad->lexem_root;
        return;
     }

   ad->lexem_ptr = context_lexem_get(ad, edit, cursor_position);
}

static void
init_thread_end_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   autocomp_data *ad = data;
   ad->initialized = EINA_TRUE;
   ad->init_thread = NULL;
}

static void
init_thread_cancel_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   autocomp_data *ad = data;
   ad->init_thread = NULL;
}

static void
entry_anchor_off(autocomp_data *ad)
{
   if (ad->anchor_visible)
     {
        elm_object_tooltip_hide(ad->anchor);
        /* Reset content_cb to have guarantee the callback call. If anchor is
           changed faster than tooltip hide, the callback won't be called
           since tooltip regards the content callback is same with before. */
        elm_object_tooltip_content_cb_set(ad->anchor, NULL, NULL, NULL);
     }
   ad->anchor_visible = EINA_FALSE;
}

static void
anchor_unfocused_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   autocomp_data *ad = g_ad;
   if (!g_ad) return;
   entry_anchor_off(ad);
}

static void
queue_reset(autocomp_data *ad)
{
   if (ad->queue_pos == 0) return;
   ad->queue_pos = 0;
   memset(ad->queue, 0x0, sizeof(ad->queue));
   entry_anchor_off(ad);
}

static void
push_char(autocomp_data *ad, char c)
{
   if (ad->queue_pos == QUEUE_SIZE)
     {
        memset(ad->queue, 0x0, sizeof(ad->queue));
        ad->queue_pos = 0;
     }
   ad->queue[ad->queue_pos] = c;

  ad->queue_pos++;
}

static void
pop_char(autocomp_data *ad, int cnt)
{
   if (ad->queue_pos == -1) return;

   int i;
   for (i = 0; i < cnt; i++)
     {
        ad->queue[ad->queue_pos] = 0x0;
        if (ad->queue_pos == 0) break;
        ad->queue_pos--;
     }
}

static void
insert_completed_text(autocomp_data *ad)
{
   if (!ad->lexem_ptr) return;
   Elm_Object_Item *it = elm_list_selected_item_get(ad->list);

   lexem *candidate =  elm_object_item_data_get(it);
   char **txt = candidate->txt;
   Evas_Object *entry = edit_entry_get(ad->ed);

   int space = edit_cur_indent_depth_get(ad->ed);
   int cursor_pos = elm_entry_cursor_pos_get(entry);

   //Insert the first line.
   elm_entry_entry_insert(entry,  txt[0] + (ad->queue_pos));

   //Insert last lines
   if (candidate->txt_count > 1)
     {
        //Alloc Empty spaces
        char *p = alloca(space + 1);
        memset(p, ' ', space);
        p[space] = '\0';

        int i;
        for (i = 1; i < (candidate->txt_count - 1); i++)
          {
             elm_entry_entry_insert(entry, p);
             elm_entry_entry_insert(entry, txt[i]);
          }
        elm_entry_entry_insert(entry, p);
        elm_entry_entry_insert(entry, txt[i]);
     }

   int cursor_pos2 = elm_entry_cursor_pos_get(entry);
   redoundo_data *rd = evas_object_data_get(entry, "redoundo");
   redoundo_entry_region_push(rd, cursor_pos, cursor_pos2);

   cursor_pos2 -= (candidate->cursor_offset + (candidate->line_back * space));
   elm_entry_cursor_pos_set(entry, cursor_pos2);
   edit_line_increase(ad->ed, (candidate->txt_count - 1));
}

static void
list_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;
   ad->list = NULL;
}

static Evas_Object *
entry_tooltip_content_cb(void *data, Evas_Object *obj EINA_UNUSED,
                         Evas_Object *tt EINA_UNUSED)
{
   autocomp_data *ad = data;
   Eina_Bool found = EINA_FALSE;

   if (!ad->lexem_ptr) return NULL;

   ad->list = elm_list_add(obj);
   elm_object_focus_allow_set(ad->list, EINA_FALSE);
   elm_list_mode_set(ad->list, ELM_LIST_EXPAND);

   Eina_List *l;
   lexem *lexem_data;
   Elm_Object_Item *it = NULL;
   EINA_LIST_FOREACH(ad->lexem_ptr->nodes, l, lexem_data)
     {
        if (!strncmp(lexem_data->name, ad->queue, ad->queue_pos))
          {
             it = elm_list_item_append(ad->list, lexem_data->name,
                                       NULL, NULL, NULL, lexem_data);
             found = EINA_TRUE;
          }
     }

   elm_list_item_selected_set(it, EINA_TRUE);
   evas_object_smart_callback_add(ad->list, "unfocused", anchor_unfocused_cb,
                                  ad);
   evas_object_event_callback_add(ad->list, EVAS_CALLBACK_DEL, list_del_cb, ad);
   if (!found)
     {
        entry_anchor_off(ad);
        return NULL;
     }
   elm_list_go(ad->list);
   evas_object_show(ad->list);

   return ad->list;
}

static void
anchor_list_update(autocomp_data *ad)
{
   lexem *data;
   Eina_List *l;
   Elm_Object_Item *it;
   Eina_Bool found = EINA_FALSE;

   elm_list_clear(ad->list);
   //Append new candidates
   EINA_LIST_FOREACH(ad->lexem_ptr->nodes, l, data)
   {
      if (!strncmp(data->name, ad->queue, ad->queue_pos))
        {
           elm_list_item_append(ad->list, data->name,
                                NULL, NULL, NULL, data);
           found = EINA_TRUE;
        }
   }
   if (!found)
     {
        entry_anchor_off(ad);
        return;
     }
   it = elm_list_first_item_get(ad->list);
   elm_list_item_selected_set(it, EINA_TRUE);
   elm_list_go(ad->list);
}

static void
candidate_list_show(autocomp_data *ad)
{
   if (!ad->lexem_ptr)
     {
        entry_anchor_off(ad);
        return;
     }

   Evas_Object *entry = edit_entry_get(ad->ed);

   //Update anchor position
   Evas_Coord x, y, h;
   Evas_Coord cx, cy, cw, ch;
   evas_object_geometry_get(entry, &x, &y, NULL, NULL);
   elm_entry_cursor_geometry_get(entry, &cx, &cy, &cw, &ch);
   evas_object_move(ad->anchor, cx + x, cy + y);
   evas_object_resize(ad->anchor, cw, ch);

   //Show the tooltip
   if (!ad->anchor_visible)
     {
        //Decide the Tooltip direction
        Elm_Tooltip_Orient tooltip_orient = ELM_TOOLTIP_ORIENT_BOTTOM;
        evas_object_geometry_get(edit_obj_get(ad->ed), NULL, NULL, NULL, &h);
        if ((cy + y) > (h / 2)) tooltip_orient = ELM_TOOLTIP_ORIENT_TOP;

        //Tooltip set
        elm_object_tooltip_content_cb_set(ad->anchor,
                                          entry_tooltip_content_cb, ad, NULL);
        elm_object_tooltip_orient_set(ad->anchor, tooltip_orient);
        elm_object_tooltip_show(ad->anchor);
        ad->anchor_visible = EINA_TRUE;
     }
   //Already tooltip is visible, just update the list item
   else anchor_list_update(ad);
}

static void
entry_changed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                 void *event_info)
{
   autocomp_data *ad = g_ad;
   if ((!g_ad) || (!ad->enabled)) return;

   Elm_Entry_Change_Info *info = event_info;

   if (info->insert)
     {
        if ((strlen(info->change.insert.content) > 1) ||
            (info->change.insert.content[0] == ' ') ||
            (info->change.insert.content[0] == '.'))
          {
             queue_reset(ad);
          }
        else
          {
             push_char(ad, info->change.insert.content[0]);
             candidate_list_show(ad);
          }
     }
   else
     {
        if (info->change.del.content[0] != ' ')
          {
             entry_anchor_off(ad);
             int cnt = abs(info->change.del.end - info->change.del.start);
             pop_char(ad, cnt);
          }
     }
}

static void
entry_cursor_changed_manual_cb(void *data EINA_UNUSED,
                               Evas_Object *obj EINA_UNUSED,
                               void *event_info EINA_UNUSED)
{
   autocomp_data *ad = g_ad;
   if (!g_ad) return;
   entry_anchor_off(ad);
}

static void
entry_cursor_changed_cb(void *data EINA_UNUSED, Evas_Object *obj,
                        void *event_info EINA_UNUSED)
{
   autocomp_data *ad = g_ad;
   if (!g_ad) return;

   //Update anchor position
   Evas_Coord x, y, cx, cy, cw, ch;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   elm_entry_cursor_geometry_get(obj, &cx, &cy, &cw, &ch);
   evas_object_move(ad->anchor, cx + x, cy + y);
   evas_object_resize(ad->anchor, cw, ch);
   context_changed(ad, edit_entry_get(ad->ed));
}

static void
entry_press_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   autocomp_data *ad = g_ad;
   if (!g_ad) return;
   entry_anchor_off(ad);
}

static void
entry_move_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   autocomp_data *ad = g_ad;
   if (!g_ad) return;
   entry_anchor_off(ad);
}

static void
list_item_move(autocomp_data *ad, Eina_Bool up)
{
   Evas_Object *entry = edit_entry_get(ad->ed);
   evas_object_smart_callback_del(entry, "unfocused", anchor_unfocused_cb);

   Elm_Object_Item *it = elm_list_selected_item_get(ad->list);
   if (up) it = elm_list_item_prev(it);
   else it = elm_list_item_next(it);
   if (it) elm_list_item_selected_set(it, EINA_TRUE);

   evas_object_smart_callback_add(entry, "unfocused", anchor_unfocused_cb,
                                  NULL);
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
autocomp_target_set(edit_data *ed)
{
   autocomp_data *ad = g_ad;
   Evas_Object *entry;

   queue_reset(ad);

   //Unset previous target
   if (ad->ed)
     {
        entry = edit_entry_get(ad->ed);
        evas_object_smart_callback_del(entry, "changed,user", entry_changed_cb);
        evas_object_smart_callback_del(entry, "cursor,changed",
                                       entry_cursor_changed_cb);
        evas_object_smart_callback_del(entry, "cursor,changed,manual",
                                               entry_cursor_changed_manual_cb);
        evas_object_smart_callback_del(entry, "unfocused", anchor_unfocused_cb);
        evas_object_smart_callback_del(entry, "press", entry_press_cb);
        evas_object_event_callback_del(entry, EVAS_CALLBACK_MOVE,
                                       entry_move_cb);
        evas_object_del(ad->anchor);
        ad->anchor = NULL;
        ad->ed = NULL;
     }

   if (!ed) return;

   entry = edit_entry_get(ed);
   evas_object_smart_callback_add(entry, "changed,user", entry_changed_cb,
                                  NULL);
   evas_object_smart_callback_add(entry, "cursor,changed,manual",
                                          entry_cursor_changed_manual_cb, NULL);
   evas_object_smart_callback_add(entry, "cursor,changed",
                                  entry_cursor_changed_cb, NULL);
   evas_object_smart_callback_add(entry, "unfocused", anchor_unfocused_cb,
                                  NULL);
   evas_object_smart_callback_add(entry, "press", entry_press_cb, NULL);
   evas_object_event_callback_add(entry, EVAS_CALLBACK_MOVE,
                                  entry_move_cb, NULL);

   ad->anchor = elm_button_add(edit_obj_get(ed));
   ad->ed = ed;
}

Eina_Bool
autocomp_event_dispatch(const char *key)
{
   autocomp_data *ad = g_ad;
   if (!ad) return EINA_FALSE;

   //Reset queue.
   if (!ad->anchor_visible)
     {
        if (!strcmp(key, "Up") || !strcmp(key, "Down") || !strcmp(key, "Left") ||
            !strcmp(key, "Right"))
          queue_reset(ad);
        return EINA_FALSE;
     }

   //Cancel the auto complete.
   if (!strcmp(key, "BackSpace"))
     {
        queue_reset(ad);
        return EINA_TRUE;
     }
   if (!strcmp(key, "Return") || !strcmp(key, "Tab"))
     {
        insert_completed_text(ad);
        queue_reset(ad);
        edit_syntax_color_partial_apply(ad->ed, -1);
        return EINA_TRUE;
     }
   if (!strcmp(key, "Up"))
     {
        list_item_move(ad, EINA_TRUE);
        return EINA_TRUE;
     }
   if (!strcmp(key, "Down"))
     {
        list_item_move(ad, EINA_FALSE);
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

void
autocomp_init(void)
{
   autocomp_data *ad = calloc(1, sizeof(autocomp_data));
   if (!ad)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }

   ad->init_thread = ecore_thread_run(init_thread_cb, init_thread_end_cb,
                                      init_thread_cancel_cb, ad);
   ad->queue_pos = 0;
   g_ad = ad;
}

void
autocomp_term(void)
{
   autocomp_data *ad = g_ad;
   evas_object_del(ad->anchor);
   ecore_thread_cancel(ad->init_thread);

   eet_data_descriptor_free(lex_desc);
   eet_close(ad->source_file);

   free(ad);
   g_ad = NULL;
}

void
autocomp_enabled_set(Eina_Bool enabled)
{
   autocomp_data *ad = g_ad;
   enabled = !!enabled;
   ad->enabled = enabled;
}

Eina_Bool
autocomp_enabled_get(void)
{
   autocomp_data *ad = g_ad;
   return ad->enabled;
}
