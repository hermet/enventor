view_data * view_init(Evas_Object *parent, const char *group,
                      void (*del_cb)(void *data), void *data);
void view_term(view_data *vd);
Evas_Object *view_obj_get(view_data *vd);
void view_new(view_data *vd, const char *group);
void view_part_highlight_set(view_data *vd, const char *part_name);
void view_dummy_toggle(view_data *vd, Eina_Bool msg);
void view_program_run(view_data *vd, const char *program);
Eina_Stringshare *view_group_name_get(view_data *vd);
void *view_data_get(view_data *vd);
void view_scale_set(view_data *vd, double scale);
Eina_List *view_parts_list_get(view_data *vd);
Eina_List *view_images_list_get(view_data *vd);
Eina_List *view_programs_list_get(view_data *vd);
Eina_List *view_part_states_list_get(view_data *vd, const char *part);
Eina_List *view_program_targets_get(view_data *vd, const char *prog);
void view_string_list_free(Eina_List *list);
