indent_data *indent_init(Eina_Strbuf *strbuf);
void indent_term(indent_data *id);
indent_data * syntax_indent_data_get(syntax_helper *sh);
int indent_depth_get(indent_data *id, const char *src, int pos);
