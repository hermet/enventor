Evas_Object *stats_init(Evas_Object *parent);
void stats_term();
void stats_view_size_update();
void stats_cursor_pos_update(Evas_Coord x, Evas_Coord y, float rel_x, float rel_y);
void stats_info_msg_update(const char *msg);
void stats_line_num_update(int cur_line, int max_line);
Evas_Object *stats_obj_get();
void stats_edc_group_set(const char *group_name);
Eina_Stringshare *stats_group_name_get();
