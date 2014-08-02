#ifndef __REDOUNDO_H__
#define __REDOUNDO_H__

/* FIXME: Change comments! */

/**
 * This function undo the last change, that will happen in entry. If there are
 * wasn't changes will returned EINA_FALSE;
 *
 * @param ed The pointer to the editor_s instatns;
 *
 * @return EINA_TRUE on success or EINA_FALSE in otherwise.
 */
Eina_Bool
undo(edit_data *ed);

/**
 * This function redo the previos change, that will happen in entry. If there are
 * wasn't changes will returned EINA_FALSE;
 *
 * @param ed The pointer to the editor_s instatns;
 *
 * @return EINA_TRUE on success or EINA_FALSE in otherwise.
 */
Eina_Bool
redo(edit_data *ed);

/**
 * This function provide add new node into undo/redo stack, that shoud take
 * undo or redo action with next(for redo) or prevision(for undo) node from stack.
 *
 * @param content The pointer to text, which need mark as changed.
 * @param pos The position, where text shoud begin in editable area.
 *
 * @return EINA_TRUE on success or EINA_FALSE in otherwise.
 *
 */
Eina_Bool
redoundo_candidate_add(const char *content, int pos);

/**
 * This function provide add new node into undo/redo stack.
 *
 * @param content The pointer to text, which need mark as changed.
 * @param pos The position, where text shoud begin in editable area.
 * @param length The length of text. If this value equal 0, then length will
 *   calculate with using eina_stringshare_strlen, after sharing.
 * @param insert If EINA_TRUE change will mark as new text append action,
 *   else change will known as remove action.
 *
 * @return EINA_TRUE on success or EINA_FALSE in otherwise.
 *
 * @note This function need to resolve situations, where text is inserted by API,
 * and this event doesn't emit signal "entry,changed,user"
 */
Eina_Bool
redoundo_node_add(const char *diff, int pos, int length,  Eina_Bool insert);

/**
 * Initialize undo/redo module.
 *
 * @param entry The Evas_Object pointer to elm_entry, that contain chengable text.
 * @return @queue structure if success, or NULL in otherwise.
 */
queue *
redoundo_init(Evas_Object *entry);

/**
 * Terminating undo/redo submodule;
 *
 * @return EINA_TRUE on success or EINA_FALSE in otherwise.
 */
Eina_Bool
redoundo_term(void);

/**
 * Clear all history of changes;
 *
 * @return EINA_TRUE on success or EINA_FALSE in otherwise.
 */
Eina_Bool
redoundo_clear(void);

#endif /* __REDOUNDO_H__ */
