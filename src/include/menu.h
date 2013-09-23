#define VIEW_DATA edj_mgr_view_get(NULL, NULL)

menu_data *menu_init(Evas_Object *win, edit_data *ed, config_data *cd, void (*close_cb)(void *data), void *data);
void menu_term(menu_data *md);
Eina_Bool menu_toggle();
void menu_ctxpopup_register(Evas_Object *ctxpopup);
void menu_ctxpopup_unregister(Evas_Object *ctxpopup);
Eina_Bool menu_edc_new(menu_data *md);
Eina_Bool menu_edc_save(menu_data *md);
Eina_Bool menu_edc_load(menu_data *md);
void menu_exit(menu_data *md);
Eina_Bool menu_about(menu_data *md);
Eina_Bool menu_setting(menu_data *md);
void menu_theme_change(menu_data *md);
