#ifndef __COMMON_H__
#define __COMMON_H__


#define DEBUG_MODE 1

#ifdef DEBUG_MODE
  #define DFUNC_BEGIN() printf("%s - begin\n", __func__)
  #define DFUNC_END() printf("%s - end\n", __func__)
  #define DFUNC_NAME() printf("%s(%d)\n", __func__, __LINE__)
#else
  #define DFUNC_BEGIN()
  #define DFUNC_END()
  #define DFUNC_NAME()
#endif

#define PROTO_EDC_PATH "/tmp/.proto.edc"

extern char EDJE_PATH[PATH_MAX];

struct attr_value_s
{
   Eina_List *strs;
   float min;
   float max;
   Eina_Bool integer : 1;
};

typedef struct menu_s menu_data;
typedef struct viewer_s view_data;
typedef struct app_s app_data;
typedef struct statusbar_s stats_data;
typedef struct editor_s edit_data;
typedef struct syntax_color_s color_data;
typedef struct config_s option_data;
typedef struct parser_s parser_data;
typedef struct attr_value_s attr_value;
typedef struct fake_obj_s fake_obj;

//edit functions
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

//menu functions
menu_data *menu_init(Evas_Object *win, edit_data *ed, option_data *od, view_data *vd, void (*close_cb)(void *data), void *data);
void menu_term(menu_data *md);
Eina_Bool menu_option_toggle();
void menu_ctxpopup_register(Evas_Object *ctxpopup);
Eina_Bool menu_edc_load(menu_data *md);
void menu_exit(menu_data *md);

//view functions
view_data * view_init(Evas_Object *parent, const char *group, stats_data *sd,
                      option_data *od);
void view_term(view_data *vd);
Evas_Object *view_obj_get(view_data *vd);
void view_new(view_data *vd, const char *group);
void view_part_highlight_set(view_data *vd, const char *part_name);
Eina_Bool view_reload_need_get(view_data *vd);
void view_reload_need_set(view_data *vd, Eina_Bool reload);
void view_update(view_data *vd);

//stats functions
stats_data *stats_init(Evas_Object *parent, option_data *od);
void stats_term(stats_data *sd);
void stats_view_size_update(stats_data *sd);
void stats_cursor_pos_update(stats_data *sd, Evas_Coord x, Evas_Coord y,
                             float rel_x, float rel_y);
void stats_info_msg_update(stats_data *sd, const char *msg);
void stats_line_num_update(stats_data *sd, int cur_line, int max_line);
Evas_Object *stats_obj_get(stats_data *sd);
void stats_edc_file_set(stats_data *sd, const char *group_name);

//syntax color
color_data *color_init();
void color_term(color_data *cd);
const char *color_cancel(color_data *cd, const char *str, int length);
const char *color_apply(color_data *cd, const char *str, int length,
                        Eina_Bool realtime);

//config data
option_data *option_init(const char *edc_path, const char *edc_img_path,
                         const char *edc_snd_path);
void option_term(option_data *od);
const char *option_edc_path_get(option_data *od);
const char *option_edj_path_get(option_data *od);
const char *option_edc_img_path_get(option_data *od);
const char *option_edc_snd_path_get(option_data *od);
void option_edc_img_path_set(option_data *od, const char *edc_img_path);
void option_edc_snd_path_set(option_data *od, const char *edc_snd_path);
const Eina_List *option_edc_img_path_list_get(option_data *od);
const Eina_List *option_edc_snd_path_list_get(option_data *od);
void option_update_cb_set(option_data *od,
                          void (*cb)(void *data, option_data *od),
                          void *data);
void option_stats_bar_set(option_data *od, Eina_Bool enabled);
void option_linenumber_set(option_data *od, Eina_Bool enabled);
Eina_Bool option_stats_bar_get(option_data *od);
Eina_Bool option_linenumber_get(option_data *od);
void option_apply(option_data *od);
void option_edc_path_set(option_data *od, const char *edc_path);
void option_view_size_get(option_data *od, Evas_Coord *w, Evas_Coord *h);
void option_view_size_set(option_data *od, Evas_Coord w, Evas_Coord h);
Eina_Bool option_part_highlight_get(option_data *od);
void option_part_highlight_set(option_data *od, Eina_Bool highlight);

//parser
parser_data *parser_init();
void parser_term(parser_data *pd);
Eina_Stringshare *parser_group_name_get(parser_data *pd, Evas_Object *entry);
void parser_part_name_get(parser_data *pd, Evas_Object *entry, void (*cb)(void *data, Eina_Stringshare *part_name), void *data);
Eina_Bool parser_type_name_compare(parser_data *pd, const char *str);
const char *parser_markup_escape(parser_data *pd EINA_UNUSED, const char *str);
attr_value *parser_attribute_get(parser_data *pd, const char *text, const char *cur);

//panes
Evas_Object *panes_create(Evas_Object *parent);
void panes_full_view_right(Evas_Object *panes);
void panes_full_view_left(Evas_Object *panes);
void panes_full_view_cancel(Evas_Object *panes);

//fake obj
void fake_obj_new(Evas_Object *layout);
void fake_obj_del(Evas_Object *layout);

#endif
