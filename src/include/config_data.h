#define MAX_FONT_SCALE 5.0
#define MIN_FONT_SCALE 0.5
#define MAX_VIEW_SCALE 5.0
#define MIN_VIEW_SCALE 0.5

void config_init(const char *edc_path, const char *edc_img_path, const char *edc_snd_path, const char *edc_fnt_path, const char *edc_dat_path);
void config_term(void);
const char *config_edc_path_get(void);
const char *config_edj_path_get(void);
const char *config_edc_img_path_get(void);
const char *config_edc_snd_path_get(void);
const char *config_edc_fnt_path_get(void);
const char *config_edc_dat_path_get(void);
void config_edc_img_path_set(const char *edc_img_path);
void config_edc_snd_path_set(const char *edc_snd_path);
void config_edc_fnt_path_set(const char *edc_fnt_path);
void config_edc_dat_path_set(const char *edc_fnt_path);
Eina_List *config_edc_img_path_list_get(void);
Eina_List *config_edc_snd_path_list_get(void);
Eina_List *config_edc_fnt_path_list_get(void);
Eina_List *config_edc_dat_path_list_get(void);
void config_syntax_color_set(Enventor_Syntax_Color_Type color_type, const char *val);
const char *config_syntax_color_get(Enventor_Syntax_Color_Type color_type);
void config_update_cb_set(void (*cb)(void *data), void *data);
void config_stats_bar_set(Eina_Bool enabled);
void config_linenumber_set(Eina_Bool enabled);
Eina_Bool config_stats_bar_get(void);
Eina_Bool config_linenumber_get(void);
void config_apply(void);
void config_edc_path_set(const char *edc_path);
void config_view_size_get(Evas_Coord *w, Evas_Coord *h);
void config_view_size_set(Evas_Coord w, Evas_Coord h);
Eina_Bool config_view_size_configurable_get(void);
void config_view_size_configurable_set(Eina_Bool configurable);
Eina_Bool config_part_highlight_get(void);
void config_part_highlight_set(Eina_Bool highlight);
Eina_Bool config_dummy_swallow_get(void);
void config_dummy_swallow_set(Eina_Bool dummy_swallow);
void config_auto_indent_set(Eina_Bool auto_indent);
Eina_Bool config_auto_indent_get(void);
void config_auto_complete_set(Eina_Bool auto_complete);
Eina_Bool config_auto_complete_get(void);
void config_font_scale_set(float font_scale);
float config_font_scale_get(void);
void config_view_scale_set(double view_scale);
double config_view_scale_get(void);
Eina_Bool config_tools_get(void);
void config_tools_set(Eina_Bool enabled);
Eina_Bool config_config_get(void);
void config_config_set(Eina_Bool enabled);
double config_console_size_get(void);
void config_console_size_set(double size);
void config_win_size_get(Evas_Coord *w, Evas_Coord *h);
void config_win_size_set(Evas_Coord w, Evas_Coord h);
