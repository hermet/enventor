typedef struct text_setting_s text_setting_data;

Evas_Object *text_setting_content_get(text_setting_data *tsd, Evas_Object *parent);
void text_setting_config_set(text_setting_data *tsd);
void text_setting_term(text_setting_data *tsd);
text_setting_data *text_setting_init(void);
void text_setting_focus_set(text_setting_data *tsd);
void text_setting_reset(text_setting_data *tsd);
