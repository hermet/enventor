#define QUOT "\""
#define QUOT_C '\"'
#define QUOT_LEN 1
#define EOL "<br/>"
#define EOL_LEN 5
#define TAB "<tab/>"
#define TAB_LEN 6

parser_data *parser_init(void);
void parser_term(parser_data *pd);
Eina_Stringshare *parser_first_group_name_get(parser_data *pd, Evas_Object *entry);
void parser_cur_name_get(parser_data *pd, Evas_Object *entry, void (*cb)(void *data, Eina_Stringshare *part_name, Eina_Stringshare *group_name), void *data);
void parser_cur_group_name_get(parser_data *pd, Evas_Object *entry, void (*cb)(void *data, Eina_Stringshare *part_name, Eina_Stringshare *group_name), void *data);
Eina_Stringshare *parser_cur_name_fast_get(Evas_Object *entry, const char *scope);
Eina_Bool parser_type_name_compare(parser_data *pd, const char *str);
attr_value *parser_attribute_get(parser_data *pd, const char *text, const char *cur);
Eina_Stringshare *parser_paragh_name_get(parser_data *pd, Evas_Object *entry);
char *parser_name_get(parser_data *pd, const char *cur);
void parser_cancel(parser_data *pd);
int parser_line_cnt_get(parser_data *pd EINA_UNUSED, const char *src);
Eina_List *parser_states_filtered_name_get(Eina_List *states);
int parser_end_of_parts_block_pos_get(const Evas_Object *entry, const char *group_name);
Eina_Bool parser_images_pos_get(const Evas_Object *entry, int *ret);
Eina_Bool parser_styles_pos_get(const Evas_Object *entry, int *ret);
const char * parser_colon_pos_get(parser_data *pd EINA_UNUSED, const char *cur);

