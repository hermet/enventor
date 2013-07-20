option_data *option_init(const char *edc_path, const char *edc_img_path,
                         const char *edc_snd_path);
void option_term(option_data *od);
const char *option_edc_path_get(option_data *od);
const char *option_edj_path_get(option_data *od);
const char *option_edc_img_path_get(option_data *od);
const char *option_edc_snd_path_get(option_data *od);
void option_edc_img_path_set(option_data *od, const char *edc_img_path);
void option_edc_snd_path_set(option_data *od, const char *edc_snd_path);
const Eina_List *option_edc_img_path_list_get(option_data *od);
const Eina_List *option_edc_snd_path_list_get(option_data *od);
void option_update_cb_set(option_data *od,
                          void (*cb)(void *data, option_data *od),
                          void *data);
void option_stats_bar_set(option_data *od, Eina_Bool enabled);
void option_linenumber_set(option_data *od, Eina_Bool enabled);
Eina_Bool option_stats_bar_get(option_data *od);
Eina_Bool option_linenumber_get(option_data *od);
void option_apply(option_data *od);
void option_edc_path_set(option_data *od, const char *edc_path);
void option_view_size_get(option_data *od, Evas_Coord *w, Evas_Coord *h);
void option_view_size_set(option_data *od, Evas_Coord w, Evas_Coord h);
Eina_Bool option_part_highlight_get(option_data *od);
void option_part_highlight_set(option_data *od, Eina_Bool highlight);
Eina_Bool option_dummy_swallow_get(option_data *od);
void option_dummy_swallow_set(option_data *od, Eina_Bool dummy_swallow);


