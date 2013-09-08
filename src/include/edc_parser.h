#define QUOT "\""
#define QUOT_C '\"'
#define QUOT_LEN 1

parser_data *parser_init();
void parser_term(parser_data *pd);
Eina_Stringshare *parser_group_name_get(parser_data *pd, Evas_Object *entry);
void parser_part_name_get(parser_data *pd, Evas_Object *entry, void (*cb)(void *data, Eina_Stringshare *part_name), void *data);
Eina_Bool parser_type_name_compare(parser_data *pd, const char *str);
const char *parser_markup_escape(parser_data *pd EINA_UNUSED, const char *str);
attr_value *parser_attribute_get(parser_data *pd, const char *text, const char *cur);
const char * parser_paragh_name_get(parser_data *pd, Evas_Object *entry);
char *parser_name_get(parser_data *pd, const char *cur);
