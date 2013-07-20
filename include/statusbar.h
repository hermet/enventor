stats_data *stats_init(Evas_Object *parent, option_data *od);
void stats_term(stats_data *sd);
void stats_view_size_update(stats_data *sd);
void stats_cursor_pos_update(stats_data *sd, Evas_Coord x, Evas_Coord y,
                             float rel_x, float rel_y);
void stats_info_msg_update(stats_data *sd, const char *msg);
void stats_line_num_update(stats_data *sd, int cur_line, int max_line);
Evas_Object *stats_obj_get(stats_data *sd);
void stats_edc_file_set(stats_data *sd, const char *group_name);

