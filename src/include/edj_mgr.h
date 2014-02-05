#define VIEW_DATA edj_mgr_view_get(NULL)

void edj_mgr_init(Evas_Object *parent);
void edj_mgr_term();
view_data * edj_mgr_view_new(const char *group);
view_data *edj_mgr_view_get(Eina_Stringshare *group);
Evas_Object * edj_mgr_obj_get();
view_data *edj_mgr_view_switch_to(view_data *vd);
void edj_mgr_view_del(view_data *vd);
void edj_mgr_reload_need_set(Eina_Bool reload);
Eina_Bool edj_mgr_reload_need_get();
void edj_mgr_clear();


