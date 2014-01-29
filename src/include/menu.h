#define VIEW_DATA edj_mgr_view_get(NULL, NULL)

menu_data *menu_init(Evas_Object *win, edit_data *ed, config_data *cd);
void menu_term();
void menu_toggle();
void menu_ctxpopup_register(Evas_Object *ctxpopup);
void menu_ctxpopup_unregister(Evas_Object *ctxpopup);
Eina_Bool menu_edc_new();
void menu_edc_save();
void menu_edc_load();
void menu_exit();
void menu_about();
void menu_setting();
int menu_open_depth();
