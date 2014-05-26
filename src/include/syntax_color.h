color_data *color_init(Eina_Strbuf *strbuf);
void color_term(color_data *cd);
const char *color_cancel(color_data *cd, const char *str, int length, int from_pos, int to_pos, char **from, char **to);
const char *color_apply(color_data *cd, const char *str, int length, char *from, char *to);
Eina_Bool color_ready(color_data *cd);

