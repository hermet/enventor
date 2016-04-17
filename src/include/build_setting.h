typedef struct build_setting_s build_setting_data;

Evas_Object *build_setting_content_get(build_setting_data *tsd, Evas_Object *parent);
void build_setting_config_set(build_setting_data *tsd);
void build_setting_term(build_setting_data *tsd);
build_setting_data *build_setting_init(void);
void build_setting_focus_set(build_setting_data *tsd);
