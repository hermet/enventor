redoundo_data *redoundo_init(Evas_Object *entry);
void redoundo_term(redoundo_data *rd);
void redoundo_clear(redoundo_data *rd);
void redoundo_text_push(redoundo_data *rd, const char *text, int pos, int length,  Eina_Bool insert);
void redoundo_text_relative_push(redoundo_data *rd, const char *text);
void redoundo_entry_region_push(redoundo_data *rd, int cursor_pos, int cursor_pos2);
int redoundo_undo(redoundo_data *rd, Eina_Bool *changed);
int redoundo_redo(redoundo_data *rd, Eina_Bool *changed);
