#define DEFAULT_EDC_NAVIGATOR_SIZE 0.3

Evas_Object *edc_navigator_init(Evas_Object *parent);
void edc_navigator_term(void);
void edc_navigator_group_update(const char *cur_group);
void edc_navigator_deselect(void);
void edc_navigator_tools_set(void);
void edc_navigator_tools_visible_set(Eina_Bool visible);
void edc_navigator_show();
void edc_navigator_hide();
