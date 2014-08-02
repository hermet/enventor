#include <Elementary.h>
#include "common.h"


/**
 * @struct _Diff
 * The structure, that store one step of changes.
 */
struct _Diff
{
   Eina_Stringshare *diff; /**< The text, that was changed.*/
   unsigned int length;    /**< Length of changed text. */
   unsigned int position;  /**< Entry cursor position from that was changed text */
   Eina_Bool action;       /**< Type of action: EINA_TRUE - text insert,
                                EINA_FALSE - text delete */
   Eina_Bool relative;     /**< If this change relative to prevision or next step
                                this flag will EINA_TRUE*/
};
typedef struct _Diff Diff;


/**
 * @struct _redoundo_queue.
 * The main structure of Redo/Undo module. Here stored queue of changes and
 *  support fields to manage this queue.
 */
struct _redoundo_queue
{
   Evas_Object *entry;            /**< The elm_entry object, that will changed */
   Evas_Object *textblock;        /**< Textblock from entry, needed for apply changes */
   Evas_Textblock_Cursor *cursor; /**< Support cursor, that provide fast access
                                       to navigate in textblock. */
   Eina_List *queue;              /**< Queue of changes. Here stored
                                       @c _Diff structures.*/
   Eina_List *current_node;       /**< Support list pointer, that provide
                                       fast management with queue. Pointed to
                                       current change list node*/
   Diff *last_diff;               /**< @c _Diff pointer to current changes.
                                       Provide quick acces to changes data*/
   Eina_Bool internal_change;     /**< The flag, that indicates that change in
                                       entry was initiated by Redo/Undo module. */
};

static queue *g_queue = NULL;

static void
_free_untracked_changes(void)
{
   if (!g_queue) return;
   if (!g_queue->last_diff)
     {
        redoundo_clear();
        return;
     }

   Eina_List *l = NULL;
   Diff *data = NULL;

   EINA_LIST_REVERSE_FOREACH(g_queue->queue, l, data)
    {
       if (data == g_queue->last_diff) break;
       eina_stringshare_del(data->diff);
       free(data);
       g_queue->queue = eina_list_remove_list(g_queue->queue, l);
    }

   return;
}

static void
_changed_user_cb(void *data EINA_UNUSED,
                 Evas_Object *obj EINA_UNUSED,
                 const char *emission EINA_UNUSED,
                 const char *source EINA_UNUSED)
{
   Edje_Entry_Change_Info *info = (Edje_Entry_Change_Info *)
     edje_object_signal_callback_extra_data_get();

   if (!info) return;
   Diff *change = NULL;
   int length = 0;

   if (g_queue->internal_change)
     {
         g_queue->internal_change = EINA_FALSE;
         return;
     }

   change = (Diff *)calloc(1, sizeof(Diff));
   if (!change)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }

   if (info->insert)
     {
        if (!info->change.insert.plain_length) goto end;

        change->diff = eina_stringshare_add(info->change.insert.content);
        change->length = strlen(evas_textblock_text_markup_to_utf8(
                                            g_queue->textblock, change->diff));
        change->position = info->change.insert.pos;
        change->action = EINA_TRUE;
     }
   else
     {
        change->diff = eina_stringshare_add(info->change.del.content);
        length = info->change.del.end - info->change.del.start;
        if (!length) goto end;

        if (length > 0) change->position = info->change.del.start;
        else change->position = info->change.del.end;

        change->length = abs(length);
        change->action = EINA_FALSE;
     }
   change->relative = EINA_FALSE;

   _free_untracked_changes();
   g_queue->queue = eina_list_append(g_queue->queue, change);
   g_queue->last_diff = change;
   g_queue->current_node = eina_list_last(g_queue->queue);

   return;

end:
   free(change);
   return;
}

Eina_Bool
undo(edit_data *ed)
{
   if ((!g_queue) || (!g_queue->last_diff))
     return EINA_FALSE;

   int lines = 0;

   elm_entry_cursor_pos_set(g_queue->entry, g_queue->last_diff->position);
   g_queue->internal_change = EINA_TRUE;
   if (g_queue->last_diff->action)
     { /* Last change was adding new symbol(s), that mean here need delete it */
        stats_line_num_update(0, elm_entry_cursor_pos_get(g_queue->entry));
        if (g_queue->last_diff->length == 1)
          {
             evas_textblock_cursor_pos_set(g_queue->cursor,
                                           g_queue->last_diff->position);
             evas_textblock_cursor_char_delete(g_queue->cursor);
          }
        else
          {
             Evas_Textblock_Cursor *range = evas_object_textblock_cursor_new(
                                                g_queue->textblock);
             evas_textblock_cursor_pos_set(g_queue->cursor,
                                           g_queue->last_diff->position);
             evas_textblock_cursor_pos_set(range, g_queue->last_diff->position +
                                           g_queue->last_diff->length);
             evas_textblock_cursor_range_delete(g_queue->cursor, range);
             evas_textblock_cursor_free(range);
          }
        lines = parser_line_cnt_get(NULL, g_queue->last_diff->diff);
        edit_line_decrease(ed, lines);
     }
   else
     {
        evas_textblock_cursor_pos_set(g_queue->cursor,
                                      g_queue->last_diff->position);
        evas_object_textblock_text_markup_prepend(g_queue->cursor,
                                                  g_queue->last_diff->diff);

        lines = parser_line_cnt_get(NULL, g_queue->last_diff->diff);
        edit_line_increase(ed, lines);
     }
   g_queue->internal_change = EINA_FALSE;

   g_queue->current_node =  eina_list_prev(g_queue->current_node);
   g_queue->last_diff =  eina_list_data_get(g_queue->current_node);

   if ((g_queue->last_diff) && (g_queue->last_diff->relative)) undo(ed);
   edit_changed_set(ed, EINA_TRUE);
   edit_syntax_color_full_apply(ed, EINA_TRUE);
   return EINA_TRUE;
}

Eina_Bool
redo(edit_data *ed)
{
   if ((!g_queue) || (!g_queue->queue))
     return EINA_FALSE;

   Eina_List *next = NULL;
   Diff *change = NULL;
   int lines;

   next = eina_list_next(g_queue->current_node);
   change = eina_list_data_get(next);

   if ((!next) && (!g_queue->last_diff))
     {
        next = g_queue->queue;
        change = eina_list_data_get(next);
     }

   if ((!next) || (!change))
     {
        g_queue->internal_change = EINA_FALSE;
        return EINA_FALSE;
     }

   g_queue->internal_change = EINA_TRUE;
   if (change->action)
     {
        evas_textblock_cursor_pos_set(g_queue->cursor,  change->position);
        evas_object_textblock_text_markup_prepend(g_queue->cursor, change->diff);

        lines = parser_line_cnt_get(NULL, change->diff);
        edit_line_increase(ed, lines);
     }
   else
     {
        if (change->length == 1)
          {
             evas_textblock_cursor_pos_set(g_queue->cursor, change->position);
             evas_textblock_cursor_char_delete(g_queue->cursor);
          }
        else
          {
             Evas_Textblock_Cursor *range = evas_object_textblock_cursor_new(
                                               g_queue->textblock);
             evas_textblock_cursor_pos_set(g_queue->cursor, change->position);
             evas_textblock_cursor_pos_set(range, change->position + change->length);
             evas_textblock_cursor_range_delete(g_queue->cursor, range);
             evas_textblock_cursor_free(range);
          }
        lines = parser_line_cnt_get(NULL, change->diff);
        edit_line_decrease(ed, lines);
     }
   g_queue->internal_change = EINA_FALSE;
   elm_entry_cursor_pos_set(g_queue->entry, change->position + change->length);

   g_queue->last_diff = change;
   g_queue->current_node = next;

   if (change->relative) redo(ed);
   edit_changed_set(ed, EINA_TRUE);
   edit_syntax_color_full_apply(ed, EINA_TRUE);
   return EINA_TRUE;
}

Eina_Bool
redoundo_candidate_add(const char *content, int pos)
{
   if ((!g_queue) || (!content) || (!pos)) return EINA_FALSE;
   Diff *change = NULL;

   change = (Diff *)calloc(1, sizeof(Diff));
   if (!change)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return EINA_FALSE;
     }

   change->diff = eina_stringshare_add(content);
   change->length = strlen(evas_textblock_text_markup_to_utf8(
                                            g_queue->textblock, change->diff));
   change->position = pos;
   change->action = EINA_TRUE;
   change->relative = EINA_TRUE;

   _free_untracked_changes();
   g_queue->queue = eina_list_append(g_queue->queue, change);
   g_queue->last_diff = change;
   g_queue->current_node = eina_list_last(g_queue->queue);

   return EINA_TRUE;
}

Eina_Bool
redoundo_node_add(const char *content, int pos, int length, Eina_Bool insert)
{
   if ((!g_queue) || (!content)) return EINA_FALSE;
   Diff *change = NULL;

   change = (Diff *)calloc(1, sizeof(Diff));
   if (!change)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }

   change->diff = eina_stringshare_add(content);
   if (length) change->length = length;
   else change->length = strlen(evas_textblock_text_markup_to_utf8(
                                            g_queue->textblock, change->diff));
   if (!change->length)
     {
        eina_stringshare_del(change->diff);
        free(change);
        return;
     }
   change->position = pos;
   change->action = insert;
   change->relative = EINA_FALSE;

   _free_untracked_changes();
   g_queue->queue = eina_list_append(g_queue->queue, change);
   g_queue->last_diff = change;
   g_queue->current_node = eina_list_last(g_queue->queue);

   return EINA_TRUE;
}

queue *
redoundo_init(Evas_Object *entry)
{
   if (!entry) return NULL;

   g_queue = (queue *)calloc(1, sizeof(queue));
   if (!g_queue)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }

   g_queue->entry = entry;
   g_queue->textblock = elm_entry_textblock_get(entry);
   g_queue->cursor = evas_object_textblock_cursor_new(g_queue->textblock);
   g_queue->internal_change = EINA_FALSE;

   elm_object_signal_callback_add(entry, "entry,changed,user", "*",
     _changed_user_cb, NULL);

   return g_queue;
}

Eina_Bool
redoundo_clear(void)
{
   Diff *data = NULL;
   if (!g_queue) return EINA_FALSE;

   EINA_LIST_FREE(g_queue->queue, data)
     {
        eina_stringshare_del(data->diff);
        free(data);
     }
   g_queue->internal_change = EINA_FALSE;
   return EINA_TRUE;
}

Eina_Bool
redoundo_term(void)
{
   if (!g_queue) return EINA_FALSE;

   g_queue->entry = NULL;
   g_queue->textblock = NULL;
   evas_textblock_cursor_free(g_queue->cursor);

   redoundo_clear();
   free(g_queue);
   return EINA_TRUE;
}

