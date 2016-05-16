typedef struct preference_setting_s preference_setting_data;

Evas_Object *preference_setting_content_get(preference_setting_data *psd, Evas_Object *parent);
void preference_setting_config_set(preference_setting_data *psd);
void preference_setting_term(preference_setting_data *psd);
preference_setting_data *preference_setting_init(void);
void preference_setting_focus_set(preference_setting_data *psd);
void preference_setting_reset(preference_setting_data *psd);
