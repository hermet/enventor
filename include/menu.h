menu_data *menu_init(Evas_Object *win, edit_data *ed, option_data *od, view_data *vd, void (*close_cb)(void *data), void *data);
void menu_term(menu_data *md);
Eina_Bool menu_option_toggle();
void menu_ctxpopup_register(Evas_Object *ctxpopup);
Eina_Bool menu_edc_load(menu_data *md);
void menu_exit(menu_data *md);


