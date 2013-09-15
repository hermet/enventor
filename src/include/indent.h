#define TAB_SPACE 3

indent_data *indent_init(Eina_Strbuf *strbuf);
void indent_term(indent_data *id);
indent_data * syntax_indent_data_get(syntax_helper *sh);
int indent_space_get(indent_data *id, Evas_Object *entry);
void indent_insert_apply(indent_data *id, Evas_Object *entry, const char *insert, int cur_line);
void indent_delete_apply(indent_data *id, Evas_Object *entry, const char *insert, int cur_line);

