#ifndef ENVENTOR_H
#define ENVENTOR_H

#ifndef ENVENTOR_BETA_API_SUPPORT
#error "Enventor APIs still unstable. It's under BETA and changeable!! If you really want to use the APIs, Please define ENVENTOR_BETA_API_SUPPORT"
#endif

#define EFL_UI_FOCUS_OBJECT_PROTECTED 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <Efl_Config.h>
#include <Elementary.h>

/***
 * Compatible ABI for Win32
 ***/
#ifdef _WIN32
# ifdef EAPI
#  undef EAPI
# endif
# ifdef ENVENTOR_WIN32_BUILD_SUPPORT
#  define EAPI __declspec(dllexport)
# else
#  define EAPI __declspec(dllimport)
# endif
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif

/**
 * Enventor emits the following signals
 * @li "cursor,line,changed":
 * @li "cursor,group,changed":
 * @li "live_view,updated":
 * @li "live_view,loaded":
 * @li "live_view,cursor,moved":
 * @li "live_view,resized":
 * @li "max_line,changed":
 * @li "compile,error":
 * @li "ctxpopup,activated":
 * @li "ctxpopup,changed":
 * @li "ctxpopup,dismissed":
 * @li "edc,modified":
 * @li "focused":
 * @li "file,open,requested":
*/

typedef Eo Enventor_Object;
typedef struct _Enventor_Item_Data Enventor_Item;

typedef enum
{
   ENVENTOR_CTXPOPUP_TYPE_SLIDER = 0,
   ENVENTOR_CTXPOPUP_TYPE_LIST,
   ENVENTOR_CTXPOPUP_TYPE_TOGGLE,
   ENVENTOR_CTXPOPUP_TYPE_IMAGE
} Enventor_Ctxpopup_Type;

typedef enum
{
   ENVENTOR_PATH_TYPE_EDJ = 0,
   ENVENTOR_PATH_TYPE_IMAGE,
   ENVENTOR_PATH_TYPE_SOUND,
   ENVENTOR_PATH_TYPE_FONT,
   ENVENTOR_PATH_TYPE_DATA,
   ENVENTOR_PATH_TYPE_LAST
} Enventor_Path_Type;

typedef struct
{
   Evas_Coord x;
   Evas_Coord y;
   float relx;
   float rely;
} Enventor_Live_View_Cursor;

typedef struct
{
   Evas_Coord w;
   Evas_Coord h;
} Enventor_Live_View_Size;

typedef struct
{
   int cur_line;
   int max_line;
} Enventor_Cursor_Line;

typedef struct
{
   Eina_Bool self_changed : 1;
} Enventor_EDC_Modified;

typedef enum {
   ENVENTOR_TEMPLATE_INSERT_DEFAULT,
   ENVENTOR_TEMPLATE_INSERT_LIVE_EDIT
} Enventor_Template_Insert_Type;

typedef enum {
   ENVENTOR_SYNTAX_COLOR_STRING,
   ENVENTOR_SYNTAX_COLOR_COMMENT,
   ENVENTOR_SYNTAX_COLOR_MACRO,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD,
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,
   ENVENTOR_SYNTAX_COLOR_CONSTANT,
   ENVENTOR_SYNTAX_COLOR_SCRIPT_FUNC,
   ENVENTOR_SYNTAX_COLOR_SCRIPT_KEYWORD,
   ENVENTOR_SYNTAX_COLOR_LAST
} Enventor_Syntax_Color_Type;

EAPI int enventor_init(int argc, char **argv);
EAPI int enventor_shutdown(void);

EAPI Evas_Object *enventor_object_add(Evas_Object *parent);
EAPI Eina_List *enventor_object_programs_list_get(Enventor_Object *obj);
EAPI Eina_List *enventor_object_part_states_list_get(Enventor_Object *obj, const char *part);
EAPI Edje_Part_Type enventor_object_part_type_get(Enventor_Object *obj, const char *part_name);
EAPI Eina_List *enventor_object_parts_list_get(Enventor_Object *obj);
EAPI void enventor_object_linenumber_set(Enventor_Object *obj, Eina_Bool linenumber);
EAPI Eina_Bool enventor_object_linenumber_get(const Enventor_Object *obj);
EAPI void enventor_object_smart_undo_redo_set(Enventor_Object *obj, Eina_Bool smart_undo_redo);
EAPI Eina_Bool enventor_object_smart_undo_redo_get(const Enventor_Object *obj);
EAPI void enventor_object_auto_indent_set(Enventor_Object *obj, Eina_Bool auto_indent);
EAPI Eina_Bool enventor_object_auto_indent_get(const Enventor_Object *obj);
EAPI void enventor_object_auto_complete_set(Enventor_Object *obj, Eina_Bool auto_complete);
EAPI Eina_Bool enventor_object_auto_complete_get(const Enventor_Object *obj);
EAPI void enventor_object_auto_complete_list_show(Enventor_Object *obj);
EAPI Eina_Bool enventor_object_path_set(Enventor_Object *obj, Enventor_Path_Type type, Eina_List *pathes);
EAPI const Eina_List *enventor_object_path_get(Enventor_Object *obj, Enventor_Path_Type type);
EAPI void enventor_object_live_view_scale_set(Enventor_Object *obj, double scale);
EAPI void enventor_object_live_view_size_set(Enventor_Object *obj, Evas_Coord w, Evas_Coord h);
EAPI void enventor_object_live_view_size_get(Enventor_Object *obj, Evas_Coord *w, Evas_Coord *h);
EAPI double enventor_object_live_view_scale_get(const Enventor_Object *obj);
EAPI void enventor_object_dummy_parts_set(Enventor_Object *obj, Eina_Bool dummy_parts);
EAPI Eina_Bool enventor_object_ctxpopup_get(const Enventor_Object *obj);
EAPI void enventor_object_ctxpopup_set(Enventor_Object *obj, Eina_Bool ctxpopup);
EAPI Eina_Bool enventor_object_ctxpopup_visible_get(Enventor_Object *obj);
EAPI void enventor_object_ctxpopup_dismiss(Enventor_Object *obj);
EAPI Eina_Bool enventor_object_dummy_parts_get(const Enventor_Object *obj);
EAPI void enventor_object_wireframes_set(Enventor_Object *obj, Eina_Bool wireframes);
EAPI Eina_Bool enventor_object_wireframes_get(const Enventor_Object *obj);
EAPI void enventor_object_part_highlight_set(Enventor_Object *obj, Eina_Bool part_highlight);
EAPI Eina_Bool enventor_object_part_highlight_get(const Enventor_Object *obj);
EAPI void enventor_object_mirror_mode_set(Enventor_Object *obj, Eina_Bool mirror_mode);
EAPI Eina_Bool enventor_object_mirror_mode_get(const Enventor_Object *obj);
EAPI void enventor_object_focus_set(Enventor_Object *obj, Eina_Bool focus);
EAPI Eina_Bool enventor_object_focus_get(const Enventor_Object *obj);
EAPI void enventor_object_font_scale_set(Enventor_Object *obj, double font_scale);
EAPI double enventor_object_font_scale_get(const Enventor_Object *obj);
EAPI void enventor_object_font_set(Enventor_Object *obj, const char *font_name, const char *font_style);
EAPI void enventor_object_font_get(Enventor_Object *obj, const char **font_name, const char **font_style);
EAPI void enventor_object_syntax_color_set(Enventor_Object *obj, Enventor_Syntax_Color_Type color_type, const char *val);
EAPI const char *enventor_object_syntax_color_get(Enventor_Object *obj, Enventor_Syntax_Color_Type color_type);
EAPI Eo *enventor_object_live_view_get(Enventor_Object *obj);
EAPI void enventor_object_disabled_set(Enventor_Object *obj, Eina_Bool disabled);
EAPI void enventor_object_program_run(Enventor_Object *obj, const char *program);
EAPI void enventor_object_programs_stop(Enventor_Object *obj);
EAPI void enventor_object_keyword_reference_show(Enventor_Object *obj);
EAPI double enventor_object_base_scale_get(Enventor_Object *obj);

EAPI Enventor_Item *enventor_object_main_item_set(Evas_Object *obj, const char *file);
EAPI Enventor_Item *enventor_object_sub_item_add(Evas_Object *obj, const char *file);
EAPI Enventor_Item *enventor_object_main_item_get(const Evas_Object *obj);
EAPI const Eina_List *enventor_object_sub_items_get(const Evas_Object *obj);
EAPI Evas_Object *enventor_item_editor_get(const Enventor_Item *it);
EAPI const char *enventor_item_file_get(const Enventor_Item *it);
EAPI Enventor_Item *enventor_object_focused_item_get(const Evas_Object *obj);
EAPI Eina_Bool enventor_item_represent(Enventor_Item *it);
EAPI int enventor_item_max_line_get(const Enventor_Item *it);
EAPI Eina_Bool enventor_item_line_goto(Enventor_Item *it, int line);
EAPI Eina_Bool enventor_item_syntax_color_full_apply(Enventor_Item *it, Eina_Bool force);
EAPI Eina_Bool enventor_item_syntax_color_partial_apply(Enventor_Item *it, double interval);
EAPI Eina_Bool enventor_item_select_region_set(Enventor_Item *it, int start, int end);
EAPI Eina_Bool enventor_item_select_none(Enventor_Item *it);
EAPI Eina_Bool enventor_item_cursor_pos_set(Enventor_Item *it, int position);
EAPI int enventor_item_cursor_pos_get(const Enventor_Item *it);
EAPI const char *enventor_item_selection_get(const Enventor_Item *it);
EAPI Eina_Bool enventor_item_text_insert(Enventor_Item *it, const char *text);
EAPI const char * enventor_item_text_get(const Enventor_Item *it);
EAPI Eina_Bool enventor_item_line_delete(Enventor_Item *it);
EAPI Eina_Bool enventor_item_file_save(Enventor_Item *it, const char *file);
EAPI Eina_Bool enventor_item_modified_get(const Enventor_Item *it);
EAPI void enventor_item_modified_set(Enventor_Item *it, Eina_Bool modified);
EAPI Eina_Bool enventor_item_del(Enventor_Item *it);
EAPI Eina_Bool enventor_item_template_insert(Enventor_Item *it, char *syntax, size_t n);
EAPI Eina_Bool enventor_item_template_part_insert(Enventor_Item *it, Edje_Part_Type part, Enventor_Template_Insert_Type insert_type, Eina_Bool fixed_w, Eina_Bool fixed_h, char *rel1_x_to, char *rel1_y_to, char *rel2_x_to, char *rel2_y_to, float align_x, float align_y, int min_w, int min_h, float rel1_x, float rel1_y, float rel2_x,float rel2_y, char *syntax, size_t n);
EAPI Eina_Bool enventor_item_redo(Enventor_Item *it);
EAPI Eina_Bool enventor_item_undo(Enventor_Item *it);
EAPI Eina_List *enventor_item_group_list_get(Enventor_Item *it);

#ifdef __cplusplus
}
#endif

#endif
