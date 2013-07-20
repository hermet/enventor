
edit_data *edit_init(Evas_Object *win, stats_data *sd, option_data *od);
void edit_term(edit_data *ed);
void edit_edc_read(edit_data *ed, const char *file_path);
void edit_focus_set(edit_data *ed);
Evas_Object *edit_obj_get(edit_data *ed);
Eina_Bool edit_changed_get(edit_data *ed);
void edit_changed_reset(edit_data *ed);
void edit_line_number_toggle(edit_data *ed);
void edit_editable_set(edit_data *ed, Eina_Bool editable);
void edit_save(edit_data *ed);
const char *edit_group_name_get(edit_data *ed);
void edit_new(edit_data* ed);
void edit_part_changed_cb_set(edit_data *ed, void (*cb)(void *data, const char *part_name), void *data);
void edit_cur_part_update(edit_data *ed);


