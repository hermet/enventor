#include <Elementary.h>
#include "common.h"

typedef struct diff_s
{
   Eina_Stringshare *text;
   unsigned int length;
   unsigned int cursor_pos;
   Eina_Bool action : 1;   //EINA_TRUE: insert, EINA_FALSE, delete
   Eina_Bool relative : 1; //If this change relative to prevision or next step
} diff_data;

struct redoundo_s
{
   Evas_Object *entry;
   Evas_Object *textblock;
   Evas_Textblock_Cursor *cursor;
   Eina_List *queue;
   Eina_List *current_node;
   diff_data *last_diff;
   Eina_Bool internal_change : 1; //Entry change by redoundo
};

static void
untracked_diff_free(redoundo_data *rd)
{
   if (!rd->last_diff)
     {
        redoundo_clear(rd);
        return;
     }

   Eina_List *l;
   diff_data *diff;

   EINA_LIST_REVERSE_FOREACH(rd->queue, l, diff)
    {
       if (diff == rd->last_diff) break;
       eina_stringshare_del(diff->text);
       free(diff);
       rd->queue = eina_list_remove_list(rd->queue, l);
    }
}

static void
entry_changed_user_cb(void *data, Evas_Object *obj EINA_UNUSED,
                      const char *emission EINA_UNUSED,
                      const char *source EINA_UNUSED)
{
   redoundo_data *rd = data;
   Edje_Entry_Change_Info *info = edje_object_signal_callback_extra_data_get();

   if (rd->internal_change)
     {
         rd->internal_change = EINA_FALSE;
         return;
     }

   diff_data *diff = calloc(1, sizeof(diff_data));
   if (!diff)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }

   if (info->insert)
     {
        if (info->change.insert.plain_length == 0) goto nochange;
        diff->text = eina_stringshare_add(info->change.insert.content);
        char *utf8 = evas_textblock_text_markup_to_utf8(NULL, diff->text);
        diff->length = strlen(utf8);
        diff->cursor_pos = info->change.insert.pos;
        diff->action = EINA_TRUE;
        free(utf8);
     }
   else
     {
        int length = (info->change.del.end - info->change.del.start);
        if (length == 0) goto nochange;

        diff->text = eina_stringshare_add(info->change.del.content);
        if (length > 0) diff->cursor_pos = info->change.del.start;
        else diff->cursor_pos = info->change.del.end;
        diff->length = abs(length);
        diff->action = EINA_FALSE;
     }
   diff->relative = EINA_FALSE;

   untracked_diff_free(rd);
   rd->queue = eina_list_append(rd->queue, diff);
   rd->last_diff = diff;
   rd->current_node = eina_list_last(rd->queue);

   return;

nochange:
   free(diff);
}

int
redoundo_undo(redoundo_data *rd, Eina_Bool *changed)
{
   if (changed) *changed = EINA_FALSE;

   if (!rd->last_diff) return 0;

   elm_entry_cursor_pos_set(rd->entry, rd->last_diff->cursor_pos);
   rd->internal_change = EINA_TRUE;

   int lines;

   if (rd->last_diff->action)
     {
        //Last change was adding new symbol(s), that mean here need delete it
        //Undo one character
        if (rd->last_diff->length == 1)
          {
             evas_textblock_cursor_pos_set(rd->cursor,
                                           rd->last_diff->cursor_pos);
             evas_textblock_cursor_char_delete(rd->cursor);
          }
        //Undo String
        else
          {
             Evas_Textblock_Cursor *cursor =
                evas_object_textblock_cursor_new( rd->textblock);
             evas_textblock_cursor_pos_set(rd->cursor,
                                           rd->last_diff->cursor_pos);
             evas_textblock_cursor_pos_set(cursor,
                                           (rd->last_diff->cursor_pos +
                                            rd->last_diff->length));
             evas_textblock_cursor_range_delete(rd->cursor, cursor);
             evas_textblock_cursor_free(cursor);
          }
        lines = -parser_line_cnt_get(NULL, rd->last_diff->text);
     }
   else
     {
        evas_textblock_cursor_pos_set(rd->cursor,
                                      rd->last_diff->cursor_pos);
        evas_object_textblock_text_markup_prepend(rd->cursor,
                                                  rd->last_diff->text);

        lines = parser_line_cnt_get(NULL, rd->last_diff->text);
     }

   rd->internal_change = EINA_FALSE;
   rd->current_node =  eina_list_prev(rd->current_node);
   rd->last_diff =  eina_list_data_get(rd->current_node);

   if (rd->last_diff && rd->last_diff->relative)
     lines += redoundo_undo(rd, NULL);

   if (changed)
     {
        elm_entry_calc_force(rd->entry);
        *changed = EINA_TRUE;
     }

   return lines;
}

int
redoundo_redo(redoundo_data *rd, Eina_Bool *changed)
{
   if (changed) *changed = EINA_FALSE;

   if (!rd->queue) return 0;

   Eina_List *next;
   diff_data *diff;
   int lines;

   next = eina_list_next(rd->current_node);
   diff = eina_list_data_get(next);

   if ((!next) && (!rd->last_diff))
     {
        next = rd->queue;
        diff = eina_list_data_get(next);
     }

   if (!next || !diff)
     {
        rd->internal_change = EINA_FALSE;
        return 0;
     }

   rd->internal_change = EINA_TRUE;

   //Insert
   if (diff->action)
     {
        evas_textblock_cursor_pos_set(rd->cursor,  diff->cursor_pos);
        evas_object_textblock_text_markup_prepend(rd->cursor, diff->text);

        lines = parser_line_cnt_get(NULL, diff->text);
     }
   //Remove
   else
     {
        //One Character
        if (diff->length == 1)
          {
             evas_textblock_cursor_pos_set(rd->cursor, diff->cursor_pos);
             evas_textblock_cursor_char_delete(rd->cursor);
          }
        //String
        else
          {
             Evas_Textblock_Cursor *cursor =
                evas_object_textblock_cursor_new(rd->textblock);
             evas_textblock_cursor_pos_set(rd->cursor, diff->cursor_pos);
             evas_textblock_cursor_pos_set(cursor,
                                           (diff->cursor_pos + diff->length));
             evas_textblock_cursor_range_delete(rd->cursor, cursor);
             evas_textblock_cursor_free(cursor);
          }
        lines = -parser_line_cnt_get(NULL, diff->text);
     }

   rd->internal_change = EINA_FALSE;
   elm_entry_cursor_pos_set(rd->entry, (diff->cursor_pos + diff->length));

   rd->last_diff = diff;
   rd->current_node = next;

   if (diff->relative)
     lines += redoundo_redo(rd, NULL);

   if (changed)
     {
        elm_entry_calc_force(rd->entry);
        *changed = EINA_TRUE;
     }

   return EINA_TRUE;
}

void
redoundo_text_push(redoundo_data *rd, const char *text, int pos, int length,
                   Eina_Bool insert)
{
   if (!text) return;

   diff_data *diff = calloc(1, sizeof(diff_data));
   if (!diff)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }

   if (length) diff->length = length;
   else
     {
        char *utf8 = evas_textblock_text_markup_to_utf8(NULL, text);
        diff->length = strlen(utf8);
        free(utf8);
        if (!diff->length)
          {
             free(diff);
             return;
          }
     }

   diff->text = eina_stringshare_add(text);
   diff->cursor_pos = pos;
   diff->action = insert;
   diff->relative = EINA_FALSE;

   untracked_diff_free(rd);
   rd->queue = eina_list_append(rd->queue, diff);
   rd->last_diff = diff;
   rd->current_node = eina_list_last(rd->queue);
}

redoundo_data *
redoundo_init(Evas_Object *entry)
{
   if (!entry) return NULL;

   redoundo_data *rd = calloc(1, sizeof(redoundo_data));
   if (!rd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }

   rd->entry = entry;
   rd->textblock = elm_entry_textblock_get(entry);
   rd->cursor = evas_object_textblock_cursor_new(rd->textblock);

   //FIXME: Why signal callback? not smart callback?
   elm_object_signal_callback_add(entry, "entry,changed,user", "*",
                                  entry_changed_user_cb, rd);
   return rd;
}

void
redoundo_clear(redoundo_data *rd)
{
   diff_data *data;

   EINA_LIST_FREE(rd->queue, data)
     {
        eina_stringshare_del(data->text);
        free(data);
     }
   rd->internal_change = EINA_FALSE;
}

void
redoundo_term(redoundo_data *rd)
{
   redoundo_clear(rd);
   evas_textblock_cursor_free(rd->cursor);
   free(rd);
}

void
redoundo_entry_region_push(redoundo_data *rd, int cursor_pos, int cursor_pos2)
{
   elm_entry_select_region_set(rd->entry,  cursor_pos, cursor_pos2);
   redoundo_text_push(rd, elm_entry_selection_get(rd->entry), cursor_pos,
                      (cursor_pos2 - cursor_pos), EINA_TRUE);
   elm_entry_select_none(rd->entry);
}

void
redoundo_text_relative_push(redoundo_data *rd, const char *text)
{
   if (!text) return;

   diff_data *diff = malloc(sizeof(diff_data));
   if (!diff)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }

   diff->text = eina_stringshare_add(text);
   char *utf8 = evas_textblock_text_markup_to_utf8(NULL, diff->text);
   diff->length = strlen(utf8);
   diff->cursor_pos = elm_entry_cursor_pos_get(rd->entry);
   diff->action = EINA_TRUE;
   diff->relative = EINA_TRUE;

   untracked_diff_free(rd);

   rd->queue = eina_list_append(rd->queue, diff);
   rd->last_diff = diff;
   rd->current_node = eina_list_last(rd->queue);

   free(utf8);
}
