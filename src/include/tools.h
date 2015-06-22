Evas_Object *tools_init(Evas_Object *parent, Evas_Object *enventor);
void tools_term(void);
Evas_Object *tools_live_edit_get(Evas_Object *tools);
void tools_highlight_update(Evas_Object *enventor, Eina_Bool toggle);
void tools_lines_update(Evas_Object *enventor, Eina_Bool toggle);
void tools_swallow_update(Evas_Object *enventor, Eina_Bool toggle);
void tools_status_update(Evas_Object *enventor, Eina_Bool toggle);
void tools_goto_update(Evas_Object *enventor, Eina_Bool toggle);
void tools_search_update(Evas_Object *enventor, Eina_Bool toggle);
void tools_live_update(Eina_Bool on);
void tools_console_update(Eina_Bool on);
void tools_menu_update(Eina_Bool on);
