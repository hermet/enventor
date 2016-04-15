Evas_Object *text_setting_content_get(Evas_Object *parent);
void text_setting_syntax_color_reset(void);
void text_setting_syntax_color_save(void);
void text_setting_config_set(void);
void text_setting_font_set(const char *font_name, const char *font_style);
void text_setting_font_scale_set(double font_scale);
void text_setting_linenumber_set(Eina_Bool enabled);
void text_setting_auto_indent_set(Eina_Bool enabled);
void text_setting_auto_complete_set(Eina_Bool enabled);
void text_setting_smart_undo_redo_set(Eina_Bool enabled);
void text_setting_term(void);
void text_setting_init(void);
void text_setting_focus_set(void);
