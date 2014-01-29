edj_mgr *edj_mgr_get();
edj_mgr *edj_mgr_init(Evas_Object *parent);
void edj_mgr_term(edj_mgr *em);
view_data * edj_mgr_view_new(edj_mgr *em, const char *group, stats_data *sd);
view_data *edj_mgr_view_get(edj_mgr *em, Eina_Stringshare *group);
Evas_Object * edj_mgr_obj_get(edj_mgr *em);
view_data *edj_mgr_view_switch_to(edj_mgr *em, view_data *vd);
void edj_mgr_view_del(edj_mgr *em, view_data *vd);

void edj_mgr_reload_need_set(edj_mgr *em, Eina_Bool reload);
Eina_Bool edj_mgr_reload_need_get(edj_mgr *em);
void edj_mgr_clear(edj_mgr *em);


