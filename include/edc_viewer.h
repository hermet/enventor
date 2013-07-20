view_data * view_init(Evas_Object *parent, const char *group, stats_data *sd,
                      option_data *od);
void view_term(view_data *vd);
Evas_Object *view_obj_get(view_data *vd);
void view_new(view_data *vd, const char *group);
void view_part_highlight_set(view_data *vd, const char *part_name);
Eina_Bool view_reload_need_get(view_data *vd);
void view_reload_need_set(view_data *vd, Eina_Bool reload);
void view_update(view_data *vd);

