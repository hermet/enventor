config_data *config_init(const char *edc_path, const char *edc_img_path,
                         const char *edc_snd_path);
void config_term(config_data *cd);
const char *config_edc_path_get(config_data *cd);
const char *config_edj_path_get(config_data *cd);
const char *config_edc_img_path_get(config_data *cd);
const char *config_edc_snd_path_get(config_data *cd);
void config_edc_img_path_set(config_data *cd, const char *edc_img_path);
void config_edc_snd_path_set(config_data *cd, const char *edc_snd_path);
const Eina_List *config_edc_img_path_list_get(config_data *cd);
const Eina_List *config_edc_snd_path_list_get(config_data *cd);
void config_update_cb_set(config_data *cd,
                          void (*cb)(void *data, config_data *cd),
                          void *data);
void config_stats_bar_set(config_data *cd, Eina_Bool enabled);
void config_linenumber_set(config_data *cd, Eina_Bool enabled);
Eina_Bool config_stats_bar_get(config_data *cd);
Eina_Bool config_linenumber_get(config_data *cd);
void config_apply(config_data *cd);
void config_edc_path_set(config_data *cd, const char *edc_path);
void config_view_size_get(config_data *cd, Evas_Coord *w, Evas_Coord *h);
void config_view_size_set(config_data *cd, Evas_Coord w, Evas_Coord h);
Eina_Bool config_part_highlight_get(config_data *cd);
void config_part_highlight_set(config_data *cd, Eina_Bool highlight);
Eina_Bool config_dummy_swallow_get(config_data *cd);
void config_dummy_swallow_set(config_data *cd, Eina_Bool dummy_swallow);
void config_auto_indent_set(config_data *cd, Eina_Bool auto_indent);
Eina_Bool config_auto_indent_get(config_data *cd);
void config_font_size_set(config_data *cd, float font_size);
float config_font_size_get(config_data *cd);
