color_data *color_init(Eina_Strbuf *strbuf);
void color_term(color_data *cd);
const char *color_cancel(color_data *cd, const char *str, int length);
const char *color_apply(color_data *cd, const char *str, int length);

