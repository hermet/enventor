#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include "enventor_private.h"

#define QUEUE_SIZE 20
#define COMPSET_PAIR_MINIMUM 1
#define MAX_CONTEXT_STACK 40
#define MAX_KEYWORD_LENGTH 40

typedef struct lexem_s
{
   Eina_List *nodes;
   char **txt;
   int txt_count;
   int cursor_offset;
   int line_back;
   char **name;
   int name_count;
   int dot;
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
   Evas_Object *event_rect;
   Ecore_Thread *init_thread;
   Eina_Bool anchor_visible : 1;
   Eina_Bool initialized : 1;
   Eina_Bool enabled : 1;
   Ecore_Thread *cntx_lexem_thread;
   Eina_Bool dot_candidate : 1;
   Eina_Bool on_keygrab : 1;
} autocomp_data;

typedef struct ctx_lexem_thread_data_s
{
   char *utf8;
   int cur_pos;
   lexem *result;
   autocomp_data *ad;
   Eina_Bool list_show;
} ctx_lexem_td;

static autocomp_data *g_ad = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/
static Eet_Data_Descriptor *lex_desc = NULL;
static void candidate_list_show(autocomp_data *ad);
static void queue_reset(autocomp_data *ad);
static void entry_anchor_off(autocomp_data *ad);
static void anchor_key_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);


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
   EET_DATA_DESCRIPTOR_ADD_BASIC(lex_desc, lexem, "dot", dot, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY_STRING(lex_desc, lexem, "name", name);
}

static void
autocomp_load(autocomp_data *ad)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/autocomp/autocomp.eet",
            eina_prefix_data_get(PREFIX));

   if (ad->source_file)
     eet_close(ad->source_file);
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

static void
lexem_tree_free(lexem **root)
{
   lexem *data = NULL;
   Eina_List *l = NULL;

   if (!(*root)) return;

   EINA_LIST_FOREACH((*root)->nodes, l, data)
    {
       if (data->nodes)
         lexem_tree_free(&data);
    }

   EINA_LIST_FREE((*root)->nodes, data)
     {
        free(data->txt);
        free(data->name);
        free(data);
     }
}

static void
context_lexem_thread_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   const int quot_len = QUOT_UTF8_LEN;

   ctx_lexem_td *td = (ctx_lexem_td *)data;
   if (!td->utf8) return;

   int cur_pos = td->cur_pos;
   if (cur_pos <= 1) return;

   Eina_List *l = NULL;
   Eina_List *nodes = td->ad->lexem_root->nodes;
   td->result = (lexem *)td->ad->lexem_root;
   int i = 0;
   int k = 0;
   int depth = 0;
   int context_len = 0;
   char stack[MAX_CONTEXT_STACK][MAX_KEYWORD_LENGTH];
   char *utf8 = td->utf8;
   char *cur = utf8;
   char *end = cur + cur_pos;
   char *help_ptr = NULL;
   char *help_end_ptr = NULL;
   const char *quot = QUOT_UTF8;
   Eina_Bool inside_quot = EINA_FALSE;
   Eina_Bool find_flag = EINA_FALSE;
   Eina_Bool dot_lex = EINA_FALSE;

   //In case of sub items, it won't contain "collections".
   //We added it arbitrary.
   if (!edit_is_main_file(td->ad->ed))
     {
        strcpy(stack[depth], "collections");
        depth++;
     }

   while (cur && cur <= end)
     {
        //Check inside quote
        if ((cur != end) && (!strncmp(cur, quot, quot_len)))
          {
             /* TODO: add exception for case '\"' */
             inside_quot = !inside_quot;
          }
        if (inside_quot)
          {
             cur++;
             continue;
          }

        //Check inside comment
        if (*cur == '/')
          {
             if (cur[1] == '/')
               {
                  cur = strchr(cur, '\n');
                  continue;
               }
             else if (cur[1] == '*')
               {
                  cur = strstr(cur, "*/");
                  continue;
               }
          }

        //Case 1. Find a context and store it.
        if (*cur == '{')
          {
             //Skip non-alpha numberics.
             help_end_ptr = cur;
             while (!isalnum(*help_end_ptr))
               help_end_ptr--;

             help_ptr = help_end_ptr;

             //Figure out current context keyword
             while ((help_ptr >= utf8) &&
                    (isalnum(*help_ptr) || (*help_ptr == '_')))
               {
                  help_ptr--;
               }

             if (help_ptr != utf8)
               help_ptr++;

             //Exceptional case for size.
             context_len = help_end_ptr - help_ptr + 1;
             if (context_len >= MAX_KEYWORD_LENGTH)
               {
                  cur++;
                  continue;
               }

             //Store this context.
             strncpy(stack[depth], help_ptr, context_len);
             if ((++depth) == MAX_CONTEXT_STACK) break;
          }

        //Case 2. Find a context and store it.
        if (*cur == '.')
          {
             Eina_Bool alpha_present = EINA_FALSE;
             help_end_ptr = cur - 1;

             //Backtrace to the beginning of a certain keyword.
             for (help_ptr = help_end_ptr; help_ptr && isalnum(*help_ptr);
                  help_ptr--)
               {
                  if (isalpha(*help_ptr)) alpha_present = EINA_TRUE;
               }

             //Hanlding Exception cases.
             if ((!alpha_present) || (!strncmp(help_ptr, quot, quot_len)))
               {
                  cur++;
                  continue;
               }

             if (help_ptr != utf8) help_ptr++;

             context_len = help_end_ptr - help_ptr + 1;
             if (context_len >= MAX_KEYWORD_LENGTH)
               {
                  cur++;
                  continue;
               }

             //Store this context.
             strncpy(stack[depth], help_ptr, context_len);
             strncpy(stack[depth], help_ptr, context_len);
             if ((++depth) == MAX_CONTEXT_STACK) break;

             dot_lex = EINA_TRUE;
          }

        //End of a dot lex context.
        //Reset the previous context if its out of scope.
        if (dot_lex && (*cur == ';'))
          {
             dot_lex = EINA_FALSE;
             memset(stack[depth], 0x0, MAX_KEYWORD_LENGTH);
             if (depth > 0) depth--;
          }

        //End of a context. Reset the previous context if its out of scope.
        if (*cur == '}')
          {
             memset(stack[depth], 0x0, MAX_KEYWORD_LENGTH);
             if (depth > 0) depth--;
          }
        cur++;
     }

   if (inside_quot)
     {
        td->result = NULL;
        return;
     }

   // Find current context where a cursor is inside of among the stack.
   for (i = 0; i < depth; i++)
     {
        find_flag = EINA_FALSE;
        EINA_LIST_FOREACH(nodes, l, td->result)
          {
             for (k = 0; k < td->result->name_count; k++)
               {
                  //Ok, We go through this context.
                  //FIXME: Need to compare elaborately?
                  if (!strncmp(stack[i], td->result->name[k],
                               strlen(td->result->name[k])))
                    {
                       nodes = td->result->nodes;
                       find_flag = EINA_TRUE;
                       break;
                    }
               }
             if (find_flag) break;
          }
        if (!find_flag)
          {
             td->result = NULL;
             return;
          }
     }
}

static void
context_lexem_thread_end_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   ctx_lexem_td *td = (ctx_lexem_td *)data;

   td->ad->lexem_ptr = td->result;
   td->ad->cntx_lexem_thread = NULL;

   if (td->list_show  || (td->result && td->result->dot && td->ad->dot_candidate))
     candidate_list_show(td->ad);
   td->ad->dot_candidate = EINA_FALSE;
   free(td->utf8);
   free(td);
}

static void
context_lexem_thread_cancel_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   ctx_lexem_td *td = (ctx_lexem_td *)data;

   td->ad->lexem_ptr = td->result;
   if (td->list_show || (td->result && td->result->dot && td->ad->dot_candidate))
     candidate_list_show(td->ad);
   td->ad->cntx_lexem_thread = NULL;
   td->ad->dot_candidate = EINA_FALSE;
   free(td->utf8);
   free(td);
}

static void
context_lexem_get(autocomp_data *ad, Evas_Object *entry, Eina_Bool list_show)
{
   const char *text = elm_entry_entry_get(entry);
   if (!text)
     {
        ad->lexem_ptr = (lexem *)ad->lexem_root;
        return;
     }
   ecore_thread_cancel(ad->cntx_lexem_thread);

   ctx_lexem_td *td = (ctx_lexem_td *)calloc(1, sizeof(ctx_lexem_td));
   td->utf8 = elm_entry_markup_to_utf8(text);
   td->cur_pos = elm_entry_cursor_pos_get(entry);
   td->ad = ad;
   td->result = NULL;
   td->list_show = list_show;

   ad->cntx_lexem_thread =  ecore_thread_run(context_lexem_thread_cb,
                                             context_lexem_thread_end_cb,
                                             context_lexem_thread_cancel_cb,
                                             td);
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

   context_lexem_get(ad, edit, EINA_FALSE);
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
key_grab_add(Evas_Object *keygrabber, const char *key)
{
   if (!evas_object_key_grab(keygrabber, key, 0, 0, EINA_TRUE))
     EINA_LOG_ERR(_("Failed to grab key - %s"), key);
}

static void
key_grab_del(Evas_Object *keygrabber, const char *key)
{
   evas_object_key_ungrab(keygrabber, key, 0, 0);
}

static void
anchor_keygrab_set(autocomp_data *ad, Eina_Bool grab)
{
   Evas_Object *anchor = ad->anchor;

   if (grab)
     {
        if (ad->on_keygrab) return;
        key_grab_add(anchor, "BackSpace");
        key_grab_add(anchor, "Return");
        key_grab_add(anchor, "Tab");
        key_grab_add(anchor, "Up");
        key_grab_add(anchor, "Down");
        ad->on_keygrab = EINA_TRUE;
     }
   else
     {
        if (!ad->on_keygrab) return;
        key_grab_del(anchor, "BackSpace");
        key_grab_del(anchor, "Return");
        key_grab_del(anchor, "Tab");
        key_grab_del(anchor, "Up");
        key_grab_del(anchor, "Down");
        ad->on_keygrab = EINA_FALSE;
     }
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
   anchor_keygrab_set(ad, EINA_FALSE);
   ad->anchor_visible = EINA_FALSE;
}


static void
anchor_unfocused_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;
   if (!ad) return;
   entry_anchor_off(ad);
}

static void
queue_reset(autocomp_data *ad)
{
   if ((ad->queue_pos == 0) && (!ad->anchor_visible)) return;
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
   const char *first_line = eina_stringshare_printf(txt[0], elm_object_item_part_text_get(it, NULL));
   elm_entry_entry_insert(entry,  first_line + (ad->queue_pos));
   eina_stringshare_del(first_line);

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
   redoundo_data *rd = edit_redoundo_get(ad->ed);
   redoundo_entry_region_push(rd, cursor_pos, cursor_pos2);

   entry_anchor_off(ad);

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


static int
list_item_compare(const void *data1, const void *data2)
{
   Elm_Object_Item *it1 = (Elm_Object_Item *) data1;
   Elm_Object_Item *it2 = (Elm_Object_Item *) data2;
   const char *name1 = NULL, *name2 = NULL;
   if (!it1) return -1;
   if (!it2) return 1;

   name1 = elm_object_item_part_text_get(it1, NULL);
   if (!name1) return -1;
   name2 = elm_object_item_part_text_get(it2, NULL);
   if (!name2) return 1;

   return strcmp(name1, name2);
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

   //compute list size to prevent over-sizing than enventor window.
   Evas_Coord y, y2, h;
   evas_object_geometry_get(edit_obj_get(ad->ed), NULL, &y, NULL, &h);
   evas_object_geometry_get(ad->anchor, NULL, &y2, NULL, NULL);
   Elm_Tooltip_Orient tooltip_orient =
      elm_object_tooltip_orient_get(ad->anchor);
   Evas_Coord mh;
   if (tooltip_orient == ELM_TOOLTIP_ORIENT_BOTTOM) mh = (h - y2);
   else mh = (y2 - y);
   evas_object_size_hint_max_set(ad->list, 999999, mh);

   //add keywords
   Eina_List *l;
   lexem *lexem_data;
   int k = 0;
   EINA_LIST_FOREACH(ad->lexem_ptr->nodes, l, lexem_data)
     {
       for (k = 0; k < lexem_data->name_count; k++)
         if (!strncmp(lexem_data->name[k], ad->queue, ad->queue_pos))
          {
             elm_list_item_sorted_insert(ad->list, lexem_data->name[k],
                                         NULL, NULL, NULL, lexem_data,
                                         list_item_compare);
             found = EINA_TRUE;
          }
     }

   //select first item in default.
   Elm_Object_Item *it = elm_list_first_item_get(ad->list);
   if (it) elm_list_item_selected_set(it, EINA_TRUE);

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
   int k = 0;
   EINA_LIST_FOREACH(ad->lexem_ptr->nodes, l, data)
   {
       for (k = 0; k < data->name_count; k++)
         if (!strncmp(data->name[k], ad->queue, ad->queue_pos))
           {
              elm_list_item_sorted_insert(ad->list, data->name[k],
                                   NULL, NULL, NULL, data, list_item_compare);
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
        anchor_keygrab_set(ad, EINA_TRUE);
        ad->anchor_visible = EINA_TRUE;
     }
   //Already tooltip is visible, just update the list item
   else anchor_list_update(ad);
}

static void
entry_changed_cb(void *data, Evas_Object *obj EINA_UNUSED,
                 void *event_info)
{
   autocomp_data *ad = data;
   if ((!ad) || (!ad->enabled)) return;

   Elm_Entry_Change_Info *info = event_info;

   if (info->insert)
     {
        if ((strlen(info->change.insert.content) > 1) ||
            (info->change.insert.content[0] == ' ') ||
            (info->change.insert.content[0] == '.'))
          {
             if (info->change.insert.content[0] == '.' && ad->queue_pos > 2)
               {
                  ad->dot_candidate = EINA_TRUE;
                  context_lexem_get(ad, obj, EINA_FALSE);
               }
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
entry_cursor_changed_manual_cb(void *data,
                               Evas_Object *obj EINA_UNUSED,
                               void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;
   if (!ad) return;
   if (ad->anchor_visible) entry_anchor_off(ad);
}

static void
entry_cursor_changed_cb(void *data, Evas_Object *obj,
                        void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;
   if (!ad) return;

   //Update anchor position
   Evas_Coord x, y, cx, cy, cw, ch;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   elm_entry_cursor_geometry_get(obj, &cx, &cy, &cw, &ch);
   evas_object_move(ad->anchor, cx + x, cy + y);
   evas_object_resize(ad->anchor, cw, ch);
   context_changed(ad, edit_entry_get(ad->ed));
}

static void
entry_press_cb(void *data, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;
   if (!ad) return;
   if (ad->anchor_visible) entry_anchor_off(ad);
}

static void
entry_move_cb(void *data, Evas *e EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;
   if (!ad) return;
   if (ad->anchor_visible) entry_anchor_off(ad);
}

static void
event_rect_mouse_down_cb(void *data, Evas *e EINA_UNUSED,
                         Evas_Object *obj EINA_UNUSED,
                         void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;
   if (!ad) return;
   if (ad->anchor_visible) entry_anchor_off(ad);
}

static void
list_item_move(autocomp_data *ad, Eina_Bool up)
{
   Evas_Object *entry = edit_entry_get(ad->ed);
   evas_object_smart_callback_del(entry, "unfocused", anchor_unfocused_cb);

   Elm_Object_Item *it = elm_list_selected_item_get(ad->list);
   if (up) it = elm_list_item_prev(it);
   else it = elm_list_item_next(it);
   if (it)
     {
        elm_list_item_selected_set(it, EINA_TRUE);
        elm_list_item_bring_in(it);
     }

   evas_object_smart_callback_add(entry, "unfocused", anchor_unfocused_cb,
                                  ad);
}

static void
anchor_key_down_cb(void *data, Evas *evas EINA_UNUSED,
                   Evas_Object *obj EINA_UNUSED, void *event_info)
{
   autocomp_data *ad = data;
   if (!ad->anchor_visible) return;

   Evas_Event_Key_Down *ev = event_info;

   //Cancel the auto complete.
   if (!strcmp(ev->key, "BackSpace"))
     {
        entry_anchor_off(ad);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return;
     }
   if (!strcmp(ev->key, "Return") || !strcmp(ev->key, "Tab"))
     {
        insert_completed_text(ad);
        queue_reset(ad);
        edit_syntax_color_partial_apply(ad->ed, -1);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return;
     }
   if (!strcmp(ev->key, "Up"))
     {
        list_item_move(ad, EINA_TRUE);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return;
     }
   if (!strcmp(ev->key, "Down"))
     {
        list_item_move(ad, EINA_FALSE);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return;
     }
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
autocomp_list_show(void)
{
   Evas_Object *entry;
   autocomp_data *ad = g_ad;
   if (!g_ad || !g_ad->enabled) return;

   entry = edit_entry_get(ad->ed);
   context_lexem_get(ad, entry, EINA_TRUE);
}

void
autocomp_reset(void)
{
   autocomp_data *ad = g_ad;
   if (!ad) return;
   queue_reset(ad);
}

void
autocomp_target_set(edit_data *ed)
{
   autocomp_data *ad = g_ad;
   if (!ad) return;

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
   evas_object_smart_callback_add(entry, "changed,user", entry_changed_cb, ad);
   evas_object_smart_callback_add(entry, "cursor,changed,manual",
                                          entry_cursor_changed_manual_cb, ad);
   evas_object_smart_callback_add(entry, "cursor,changed",
                                  entry_cursor_changed_cb, ad);
   evas_object_smart_callback_add(entry, "unfocused", anchor_unfocused_cb, ad);
   evas_object_smart_callback_add(entry, "press", entry_press_cb, ad);
   evas_object_event_callback_add(entry, EVAS_CALLBACK_MOVE, entry_move_cb, ad);

   ad->anchor = elm_button_add(edit_obj_get(ed));
   evas_object_event_callback_add(ad->anchor, EVAS_CALLBACK_KEY_DOWN,
                                  anchor_key_down_cb, ad);

   //event_rect catches mouse down event, which makes anchor off.
   if (!ad->event_rect)
     {
        Evas_Object *win = elm_object_top_widget_get(edit_obj_get(ed));
        Evas *e = evas_object_evas_get(win);
        Evas_Object *rect = evas_object_rectangle_add(e);
        evas_object_repeat_events_set(rect, EINA_TRUE);
        evas_object_color_set(rect, 0, 0, 0, 0);

        evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_DOWN,
                                       event_rect_mouse_down_cb, ad);
        evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND,
                                         EVAS_HINT_EXPAND);
        elm_win_resize_object_add(win, rect);
        evas_object_show(rect);

        ad->event_rect = rect;
     }

   ad->ed = ed;
}

Eina_Bool
autocomp_event_dispatch(const char *key)
{
   autocomp_data *ad = g_ad;
   if (!ad) return EINA_FALSE;
   if (ad->anchor_visible) return EINA_FALSE;

   //Reset queue.
   if (!strcmp(key, "Up") || !strcmp(key, "Down") ||
       !strcmp(key, "Left") || !strcmp(key, "Right"))
     queue_reset(ad);

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
   autocomp_target_set(NULL);

   evas_object_del(ad->event_rect);
   evas_object_del(ad->anchor);
   ecore_thread_cancel(ad->init_thread);

   lexem_tree_free((lexem **)&ad->lexem_root);

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

const char **
autocomp_current_context_get(int *name_count)
{
   autocomp_data *ad = g_ad;

   if (!ad->lexem_ptr || !ad->lexem_ptr->name)
     return NULL;

   *name_count = ad->lexem_ptr->name_count;
   return (const char **)ad->lexem_ptr->name;
}
