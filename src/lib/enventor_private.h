#ifndef __ENVENTOR_PRIVATE_H__
#define __ENVENTOR_PRIVATE_H__

#include "common.h"

#define QUOT "&quot;"
#define QUOT_LEN 6
#define QUOT_UTF8 "\""
#define QUOT_UTF8_LEN 1
#define LESS "&lt;"
#define GREATER "&gt;"
#define AMP "&amp;"
#define EOL "<br/>"
#define EOL_LEN 5
#define TAB "<tab/>"
#define TAB_LEN 6
#define TAB_SPACE 3
#define REL1_X 0.25f
#define REL1_Y 0.25f
#define REL2_X 0.75f
#define REL2_Y 0.75f
#define VIEW_DATA edj_mgr_view_get(NULL)
#define ATTR_VALUE_MAX_CNT 4

extern const char SIG_CURSOR_LINE_CHANGED[];
extern const char SIG_CURSOR_GROUP_CHANGED[];
extern const char SIG_MAX_LINE_CHANGED[];
extern const char SIG_COMPILE_ERROR[];
extern const char SIG_LIVE_VIEW_LOADED[];
extern const char SIG_LIVE_VIEW_UPDATED[];
extern const char SIG_LIVE_VIEW_CURSOR_MOVED[];
extern const char SIG_LIVE_VIEW_RESIZED[];
extern const char SIG_CTXPOPUP_CHANGED[];
extern const char SIG_CTXPOPUP_DISMISSED[];
extern const char SIG_CTXPOPUP_ACTIVATED[];
extern const char SIG_EDC_MODIFIED[];
extern const char SIG_FOCUSED[];
extern const char SIG_FILE_OPEN_REQUESTED[];

typedef struct viewer_s view_data;
typedef struct syntax_color_s color_data;
typedef struct parser_s parser_data;
typedef struct attr_value_s attr_value;
typedef struct syntax_helper_s syntax_helper;
typedef struct indent_s indent_data;
typedef struct redoundo_s redoundo_data;
typedef struct editor_s edit_data;
typedef struct state_info_s state_info;

typedef enum attr_value_type
{
   ATTR_VALUE_BOOLEAN = 1,
   ATTR_VALUE_INTEGER = 2,
   ATTR_VALUE_FLOAT = 4,
   ATTR_VALUE_CONSTANT = 8,
   ATTR_VALUE_PART = 16,
   ATTR_VALUE_STATE = 32,
   ATTR_VALUE_IMAGE = 64,
   ATTR_VALUE_PROGRAM = 128
} attr_value_type;

struct attr_value_s
{
   Eina_Array *strs;
   int cnt;
   float val[ATTR_VALUE_MAX_CNT];
   float min;
   float max;
   attr_value_type type;
   const char *prepend_str;
   const char *append_str;
   Eina_Bool program : 1;
};

struct state_info_s
{
   double value;
   const char *part;
   const char *state;
};

/* auto_comp */
void autocomp_init(void);
void autocomp_term(void);
void autocomp_target_set(edit_data *ed);
void autocomp_enabled_set(Eina_Bool enabled);
Eina_Bool autocomp_enabled_get(void);
Eina_Bool autocomp_event_dispatch(const char *key);
void autocomp_list_show(void);
void autocomp_reset(void);
const char **autocomp_current_context_get(int *name_count);


/* syntax color */
color_data *color_init(Eina_Strbuf *strbuf);
void color_term(color_data *cd);
void color_set(color_data *cd, Enventor_Syntax_Color_Type color_type, const char *val);
const char *color_value_get(Enventor_Syntax_Color_Type color_type);
const char *color_cancel(color_data *cd, const char *str, int length, int from_pos, int to_pos, char **from, char **to);
const char *color_apply(color_data *cd, const char *str, int length, char *from, char *to);
Eina_Bool color_ready(color_data *cd);


/*parser */
parser_data *parser_init(void);
void parser_term(parser_data *pd);
Eina_Stringshare *parser_first_group_name_get(parser_data *pd, Evas_Object *entry);
void parser_cur_context_get(parser_data *pd, Evas_Object *entry, void (*cb)(void *data, Eina_Stringshare *state_name, double state_value, Eina_Stringshare *part_name, Eina_Stringshare *group_name), void *data, Eina_Bool collections);
Eina_Stringshare *parser_cur_context_fast_get(Evas_Object *entry, const char *scope);
Eina_Bool parser_type_name_compare(parser_data *pd, const char *str);
attr_value *parser_attribute_get(parser_data *pd, const char *text, const char *cur, const char *selected);
void parser_attribute_value_set(attr_value *attr, char *cur);
Eina_Stringshare *parser_paragh_name_get(parser_data *pd, Evas_Object *entry);
char *parser_name_get(parser_data *pd, const char *cur);
void parser_cancel(parser_data *pd);
int parser_line_cnt_get(parser_data *pd EINA_UNUSED, const char *src);
Eina_List *parser_states_filtered_name_get(Eina_List *states);
int parser_end_of_parts_block_pos_get(const Evas_Object *entry, const char *group_name);
Eina_Bool parser_images_pos_get(const Evas_Object *entry, int *ret);
Eina_Bool parser_is_image_name(const Evas_Object *entry, const char *str);
Eina_Bool parser_styles_pos_get(const Evas_Object *entry, int *ret);
Eina_Bool parser_state_info_get(Evas_Object *entry, state_info *info);
void parser_macro_update(parser_data *pd, Eina_Bool macro_update);
typedef void (*Bracket_Update_Cb)(void *data, int left, int right);
void parser_bracket_find(parser_data *pd, Evas_Object *entry, Bracket_Update_Cb func, void *data);
void parser_bracket_cancel(parser_data *pd);
Eina_List *parser_group_list_get(parser_data *pd, Evas_Object *entry);

/* syntax helper */
syntax_helper *syntax_init(edit_data *ed);
void syntax_term(syntax_helper *sh);
color_data *syntax_color_data_get(syntax_helper *sh);
indent_data *syntax_indent_data_get(syntax_helper *sh);


/* indent */
indent_data *indent_init(Eina_Strbuf *strbuf, edit_data *ed);
void indent_term(indent_data *id);
int indent_space_get(indent_data *id);
int indent_insert_apply(indent_data *id, const char *insert, int cur_line);
void indent_delete_apply(indent_data *id, const char *del, int cur_line);
Eina_Bool indent_text_check(indent_data *id EINA_UNUSED, const char *utf8);
char * indent_text_create(indent_data *id, const char *utf8, int *indented_line_cnt);


/* build */
void build_edc(void);
void build_init(void);
void build_term(void);
Eina_Bool build_path_set(Enventor_Path_Type type, const Eina_List *pathes);
Eina_List *build_path_get(Enventor_Path_Type type);
void  build_edc_path_set(const char *edc_path);
const char *build_edc_path_get(void);
const char *build_edj_path_get(void);
void build_err_noti_cb_set(void (*cb)(void *data, const char *msg), void *data);

/* dummy_obj */
void dummy_obj_new(Evas_Object *layout);
void dummy_obj_del(Evas_Object *layout);
void dummy_obj_update(Evas_Object *layout);

/* wireframes_obj */
void wireframes_obj_new(Evas_Object *layout);
void wireframes_obj_del(Evas_Object *layout);
void wireframes_obj_update(Evas_Object *layout);

/* edj_mgr */
void edj_mgr_init(Enventor_Object *enventor);
void edj_mgr_term(void);
view_data * edj_mgr_view_new(Enventor_Item *it, const char *group);
view_data *edj_mgr_view_get(Eina_Stringshare *group);
Evas_Object * edj_mgr_obj_get(void);
void edj_mgr_view_switch_to(view_data *vd);
void edj_mgr_view_del(view_data *vd);
void edj_mgr_reload_need_set(Eina_Bool reload);
Eina_Bool edj_mgr_reload_need_get(void);
void edj_mgr_clear(void);
void edj_mgr_all_views_reload(void);


/* redoundo */
redoundo_data *redoundo_init(edit_data *ed, Enventor_Object *enventor);
void redoundo_term(redoundo_data *rd);
void redoundo_clear(redoundo_data *rd);
void redoundo_text_push(redoundo_data *rd, const char *text, int pos, int length, Eina_Bool insert);
void redoundo_text_relative_push(redoundo_data *rd, const char *text);
void redoundo_entry_region_push(redoundo_data *rd, int cursor_pos, int cursor_pos2);
int redoundo_undo(redoundo_data *rd, Eina_Bool *changed);
int redoundo_redo(redoundo_data *rd, Eina_Bool *changed);
void redoundo_n_diff_cancel(redoundo_data *rd, unsigned int n);
void redoundo_diff_buildable(redoundo_data *rd, Eina_Bool buildable);


/* edj_viewer */
view_data * view_init(Enventor_Object *enventor, Enventor_Item *it, const char *group, void (*del_cb)(void *data), void *data);
void view_term(view_data *vd);
Evas_Object *view_obj_get(view_data *vd);
void view_new(view_data *vd, const char *group);
void view_part_highlight_set(view_data *vd, const char *part_name);
void view_dummy_set(view_data *vd, Eina_Bool dummy_on);
void view_wireframes_set(view_data *vd, Eina_Bool wireframes);
void view_mirror_mode_update(view_data *vd);
void view_program_run(view_data *vd, const char *program);
void view_programs_stop(view_data *vd);
Eina_Stringshare *view_group_name_get(view_data *vd);
void *view_data_get(view_data *vd);
void view_scale_set(view_data *vd, double scale);
double view_scale_get(view_data *vd);
void view_size_get(view_data *vd, Evas_Coord *w, Evas_Coord *h);
void view_size_set(view_data *vd, Evas_Coord w, Evas_Coord h);
Eina_List *view_parts_list_get(view_data *vd);
Eina_List *view_images_list_get(view_data *vd);
Eina_List *view_programs_list_get(view_data *vd);
Eina_List *view_part_states_list_get(view_data *vd, const char *part);
Eina_List *view_program_targets_get(view_data *vd, const char *prog);
void view_string_list_free(Eina_List *list);
void view_part_state_set(view_data *vd, const char *part, const char *description, const double state);
void view_obj_need_reload_set(view_data *vd);
Edje_Part_Type view_part_type_get(view_data *vd, const char *part);

/* template */
Eina_Bool template_part_insert(edit_data *ed, Edje_Part_Type part_type, Enventor_Template_Insert_Type insert_type, Eina_Bool fixed_w, Eina_Bool fixed_h, char *rel1_x_to, char *rel1_y_to, char *rel2_x_to, char *rel2_y_to, float align_x, float align_y, int min_w, int min_h,
float rel1_x, float rel1_y, float rel2_x, float rel2_y, const Eina_Stringshare *group_name, char *syntax, size_t n);

Eina_Bool template_insert(edit_data *ed, char *syntax, size_t n);


/* ctxpopup */
Evas_Object *ctxpopup_candidate_list_create(edit_data *ed, attr_value *attr, Evas_Smart_Cb ctxpopup_dismiss_cb, Evas_Smart_Cb ctxpopup_changed_cb, Enventor_Ctxpopup_Type *type);
Evas_Object *ctxpopup_img_preview_create(edit_data*ed, const char *imgpath, Evas_Smart_Cb ctxpopup_dismiss_cb, Evas_Smart_Cb ctxpopup_relay_cb);
void ctxpopup_img_preview_reload(Evas_Object *ctxpopup, const char *imgpath);

/* edc_editor */
void edit_font_update(edit_data *ed);
Eina_Bool edit_key_down_event_dispatch(edit_data *ed, const char *key);
Eina_Bool edit_key_up_event_dispatch(edit_data *ed, const char *key);
edit_data *edit_init(Enventor_Object *enventor, Enventor_Item *it);
void edit_term(edit_data *ed);
Evas_Object *edit_obj_get(edit_data *ed);
Eina_Bool edit_changed_get(edit_data *ed);
void edit_changed_set(edit_data *ed, Eina_Bool changed);
void edit_linenumber_set(edit_data *ed, Eina_Bool linenumber);
Eina_Bool edit_saved_get(edit_data *ed);
void edit_saved_set(edit_data *ed, Eina_Bool saved);
Eina_Bool edit_save(edit_data *ed, const char *file);
void edit_new(edit_data* ed);
void edit_view_sync_cb_set(edit_data *ed, void (*cb)(void *data, Eina_Stringshare *state_name, double state_value, Eina_Stringshare *part_name, Eina_Stringshare *group_name), void *data);
void edit_view_sync(edit_data *ed);
void edit_font_scale_set(edit_data *ed, double font_scale);
void edit_line_delete(edit_data *ed);
Eina_Stringshare *edit_cur_prog_name_get(edit_data *ed);
Eina_Stringshare *edit_cur_part_name_get(edit_data *ed);
Eina_Stringshare *edit_cur_paragh_get(edit_data *ed);
int edit_max_line_get(edit_data *ed);
void edit_goto(edit_data *ed, int line);
void edit_syntax_color_set(edit_data *ed, Enventor_Syntax_Color_Type color_type, const char *val);
void edit_syntax_color_full_apply(edit_data *ed, Eina_Bool force);
void edit_syntax_color_partial_apply(edit_data *ed, double interval);
Evas_Object *edit_entry_get(edit_data *ed);
void edit_line_increase(edit_data *ed, int cnt);
void edit_line_decrease(edit_data *ed, int cnt);
int edit_cur_indent_depth_get(edit_data *ed);
void edit_redoundo_region_push(edit_data *ed, int cursor_pos1, int cursor_pos2);
Eina_Bool edit_ctxpopup_visible_get(edit_data *ed);
void edit_ctxpopup_dismiss(edit_data *ed);
Eina_Bool edit_load(edit_data *ed, const char *edc_path);
void edit_selection_clear(edit_data *ed);
const char *edit_text_get(edit_data *ed);
Eina_Bool edit_redoundo(edit_data *ed, Eina_Bool undo);
void edit_disabled_set(edit_data *ed, Eina_Bool disabled);
void edit_error_set(edit_data *ed, int line, const char *target);
void edit_text_insert(edit_data *ed, const char *text);
void edit_part_cursor_set(edit_data *ed, const char *group_name, const char *part_name);
redoundo_data *edit_redoundo_get(edit_data *ed);
void edit_selection_region_center_set(edit_data *ed, int start, int end);
const char *edit_file_get(edit_data *ed);
void edit_select_none(edit_data *ed);
void edit_cursor_pos_set(edit_data *ed, int position);
int edit_cursor_pos_get(edit_data *ed);
const char *edit_selection_get(edit_data *ed);
Eina_Bool edit_is_main_file(edit_data *ed);
Eina_Bool edit_focus_get(edit_data *ed);
void edit_focus_set(edit_data *ed, Eina_Bool focus);
Eina_List *edit_group_list_get(edit_data *ed);

/* util */
void mem_fail_msg(void);


/* reference */
void ref_init(void);
void ref_term(void);
void ref_show(edit_data *ed);

#endif
