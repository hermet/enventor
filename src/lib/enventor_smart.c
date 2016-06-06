#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#define ELM_INTERNAL_API_ARGESFSDFEFC 1

#include <Enventor.h>
#include <Eio.h>
#include <elm_widget.h>
#include "enventor_private.h"

#define MY_CLASS_NAME_LEGACY "enventor_object"

#ifdef MY_CLASS
#undef MY_CLASS
#endif

#define MY_CLASS ENVENTOR_OBJECT_CLASS

#define DEFAULT_LINENUMBER EINA_TRUE
#define DEFAULT_FONT_SCALE 1
#define DEFAULT_AUTO_INDENT EINA_TRUE
#define DEFAULT_PART_HIGHLIGHT EINA_TRUE
#define DEFAULT_SMART_UNDO_REDO EINA_FALSE
#define DEFAULT_CTXPOPUP EINA_TRUE

typedef struct _Enventor_Object_Data Enventor_Object_Data;
typedef struct _Enventor_Item_Data Enventor_Item_Data;

struct _Enventor_Item_Data
{
   edit_data *ed;
   Enventor_Object *enventor;
};

struct _Enventor_Object_Data
{
   Enventor_Object *obj;
   Enventor_Item_Data main_it;

   Eina_Stringshare *group_name;

   Ecore_Event_Handler *key_down_handler;
   Ecore_Event_Handler *key_up_handler;

   double font_scale;
   Eina_Stringshare *font_name;
   Eina_Stringshare *font_style;

   Eina_Bool dummy_parts : 1;
   Eina_Bool disabled : 1;
   Eina_Bool mirror_mode : 1;
   Eina_Bool linenumber : 1;
   Eina_Bool auto_indent : 1;
   Eina_Bool part_highlight : 1;
   Eina_Bool smart_undo_redo : 1;
   Eina_Bool ctxpopup : 1;
};

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CURSOR_LINE_CHANGED, ""},
   {SIG_CURSOR_GROUP_CHANGED, ""},
   {SIG_LIVE_VIEW_CURSOR_MOVED, ""},
   {SIG_LIVE_VIEW_RESIZED, ""},
   {SIG_LIVE_VIEW_LOADED, ""},
   {SIG_LIVE_VIEW_UPDATED, ""},
   {SIG_MAX_LINE_CHANGED, ""},
   {SIG_COMPILE_ERROR, ""},
   {SIG_CTXPOPUP_CHANGED, ""},
   {SIG_CTXPOPUP_DISMISSED, ""},
   {SIG_CTXPOPUP_ACTIVATED, ""},
   {SIG_EDC_MODIFIED, ""},
   {SIG_FOCUSED, ""},
   {NULL, NULL}
};

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
_enventor_main_item_free(Enventor_Object_Data *pd)
{
   edit_term(pd->main_it.ed);
}

static Eina_Bool
key_up_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Enventor_Object_Data *pd = data;
   Ecore_Event_Key *event = ev;

   edit_key_up_event_dispatch(pd->main_it.ed, event->key);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
key_down_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Enventor_Object_Data *pd = data;
   Ecore_Event_Key *event = ev;
   Eina_Bool ret = enventor_object_focus_get(pd->obj);
   if (!ret) return ECORE_CALLBACK_PASS_ON;

   if (edit_key_down_event_dispatch(pd->main_it.ed, event->key))
     return ECORE_CALLBACK_DONE;

   if (autocomp_event_dispatch(event->key))
     return ECORE_CALLBACK_DONE;

   return ECORE_CALLBACK_PASS_ON;
}

static void
edit_view_sync_cb(void *data, Eina_Stringshare *state_name, double state_value,
                  Eina_Stringshare *part_name, Eina_Stringshare *group_name)
{
   Enventor_Object_Data *pd = data;

   edj_mgr_all_views_reload();

   if (pd->group_name != group_name)
     {
        view_data *vd = edj_mgr_view_get(group_name);
        if (vd) edj_mgr_view_switch_to(vd);
        else
          {
             vd = edj_mgr_view_new(group_name);
             if (!vd) return;
          }
        eina_stringshare_del(pd->group_name);
        pd->group_name = eina_stringshare_add(group_name);
        evas_object_smart_callback_call(pd->obj, SIG_CURSOR_GROUP_CHANGED,
                                        (void *) group_name);
     }
   if (pd->part_highlight && !pd->disabled)
     view_part_highlight_set(VIEW_DATA, part_name);
   else
     view_part_highlight_set(VIEW_DATA, NULL);

   //reset previous part's state
   if (!state_name)
     {
        view_part_state_set(VIEW_DATA, NULL, NULL, 0);
        return;
     }

   view_part_state_set(VIEW_DATA, part_name, state_name, state_value);
}

static void
build_err_noti_cb(void *data, const char *msg)
{
   Enventor_Object_Data *pd = data;

   int line_num = 0;
   Eina_Stringshare *target = NULL;
   char *ptr = NULL;
   char *utf8 = evas_textblock_text_markup_to_utf8(NULL, msg);

   if (!utf8) goto call_error;

   if ((ptr = strstr(utf8, ".edc")))
     {
        ptr += strlen(".edc");
        if (!ptr || (ptr[0] != ':')) goto call_error;
        if (!(++ptr)) goto call_error;
        line_num = atoi(ptr);
     }
   else if ((ptr = strstr(utf8, "image")) ||
            (ptr = strstr(utf8, "group")) ||
            (ptr = strstr(utf8, "part")))
      {
        ptr = strchr(ptr, '\"');
        if (!ptr || !(ptr++)) goto call_error;
        char *ptr2 = strchr(ptr, '\"');
        if (!ptr2) goto call_error;
        int length = ptr2 - ptr;
        target = eina_stringshare_add_length(ptr, length);
     }

call_error:
   free(utf8);
   edit_error_set(pd->main_it.ed, line_num - 1, target);
   if (line_num || target)
     edit_syntax_color_full_apply(pd->main_it.ed, EINA_TRUE);

   redoundo_data *rd = edit_redoundo_get(pd->main_it.ed);

   // When msg == NULL it mean, that needed to reset error state
   if (msg)
     {
        // Ctxpopup should be dismissed only in cases when error happens
        edit_ctxpopup_dismiss(pd->main_it.ed);
        evas_object_smart_callback_call(pd->obj, SIG_COMPILE_ERROR, (char *)msg);
        redoundo_diff_buildable(rd, EINA_FALSE);
     }
   else
     {
        redoundo_diff_buildable(rd, EINA_TRUE);
     }
}

static void
_enventor_part_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED, void *ei)
{
   Enventor_Object_Data *pd = (Enventor_Object_Data *)data;
   if (pd->disabled) return;
   const char *part_name = (const char *)ei;
   edit_part_cursor_set(pd->main_it.ed, view_group_name_get(VIEW_DATA),
                        part_name);
}


/*****************************************************************************/
/* Internal Eo object required routines                                      */
/*****************************************************************************/
EOLIAN static void
_enventor_object_class_constructor(Eo_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

EOLIAN static void
_enventor_object_evas_object_smart_add(Eo *obj, Enventor_Object_Data *pd)
{
   pd->obj = obj;

   elm_widget_sub_object_parent_add(obj);
   eo_do_super(obj, MY_CLASS, evas_obj_smart_add());

   build_init();
   autocomp_init();
   edj_mgr_init(obj);
   build_err_noti_cb_set(build_err_noti_cb, pd);

   elm_widget_can_focus_set(obj, EINA_FALSE);

   pd->key_down_handler =
      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, key_down_cb, pd);
   pd->key_up_handler =
      ecore_event_handler_add(ECORE_EVENT_KEY_UP, key_up_cb, pd);

   evas_object_smart_callback_add(pd->obj, "part,clicked",
                                  _enventor_part_clicked_cb, pd);

   pd->font_scale = DEFAULT_FONT_SCALE;
   pd->linenumber = DEFAULT_LINENUMBER;
   pd->auto_indent = DEFAULT_AUTO_INDENT;
   pd->part_highlight = DEFAULT_PART_HIGHLIGHT;
   pd->smart_undo_redo = DEFAULT_SMART_UNDO_REDO;
   pd->ctxpopup = DEFAULT_CTXPOPUP;
}

EOLIAN static void
_enventor_object_evas_object_smart_del(Evas_Object *obj EINA_UNUSED,
                                       Enventor_Object_Data *pd)
{
   eina_stringshare_del(pd->font_name);
   eina_stringshare_del(pd->font_style);
   eina_stringshare_del(pd->group_name);
   autocomp_term();
   ecore_event_handler_del(pd->key_down_handler);
   ecore_event_handler_del(pd->key_up_handler);
   edj_mgr_term();
   build_term();

   _enventor_main_item_free(pd);
}

EOLIAN static void
_enventor_object_evas_object_smart_member_add(Eo *obj, Enventor_Object_Data *pd EINA_UNUSED, Evas_Object *child)
{
   eo_do_super(obj, MY_CLASS, evas_obj_smart_member_add(child));

   if (evas_object_visible_get(obj)) evas_object_show(child);
   else evas_object_hide(child);

   Evas_Coord x, y, w, h;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_move(child, x, y);
   evas_object_resize(child, w, h);

   evas_object_clip_set(child, evas_object_clip_get(obj));
}

EOLIAN static void
_enventor_object_evas_object_smart_move(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd, Evas_Coord x, Evas_Coord y)
{
   //Main Item
   Evas_Object *o = edit_obj_get(pd->main_it.ed);
   evas_object_move(o, x, y);
}

EOLIAN static void
_enventor_object_evas_object_smart_resize(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd, Evas_Coord w, Evas_Coord h)
{
   //Main Item
   Evas_Object *o = edit_obj_get(pd->main_it.ed);
   evas_object_resize(o, w, h);
}

EOLIAN static void
_enventor_object_evas_object_smart_show(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   //Main Item
   Evas_Object *o = edit_obj_get(pd->main_it.ed);
   evas_object_show(o);
}

EOLIAN static void
_enventor_object_evas_object_smart_hide(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   //Main Item
   Evas_Object *o = edit_obj_get(pd->main_it.ed);
   evas_object_hide(o);
}

EOLIAN static void
_enventor_object_evas_object_smart_clip_set(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd, Evas_Object *clip)
{
   //Main Item
   Evas_Object *o = edit_obj_get(pd->main_it.ed);
   evas_object_clip_set(o, clip);
}

EOLIAN static void
_enventor_object_evas_object_smart_clip_unset(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   //Main Item
   Evas_Object *o = edit_obj_get(pd->main_it.ed);
   evas_object_clip_unset(o);
}

EOLIAN static Eo *
_enventor_object_eo_base_constructor(Eo *obj,
                                     Enventor_Object_Data *pd EINA_UNUSED)
{
   obj = eo_do_super_ret(obj, MY_CLASS, obj, eo_constructor());
   eo_do(obj,
         evas_obj_type_set(MY_CLASS_NAME_LEGACY),
         evas_obj_smart_callbacks_descriptions_set(_smart_callbacks));

   return obj;
}

EOLIAN static Eina_Bool
_enventor_object_efl_file_file_set(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd,
                                   const char *file,
                                   const char *group EINA_UNUSED)
{
   build_edc_path_set(file);
   autocomp_target_set(pd->main_it.ed);
   if (!file) goto err;

   /* Create empty file*/
   if (!ecore_file_exists(file))
     {
         FILE *fp = fopen(file, "w");
         fclose(fp);
     }

   edit_load(pd->main_it.ed, file);
   build_edc();
   edit_changed_set(pd->main_it.ed, EINA_FALSE);

   return EINA_TRUE;

err:
   build_edc_path_set(NULL);
   return EINA_FALSE;
}

EOLIAN static Eina_List *
_enventor_object_programs_list_get(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd EINA_UNUSED)
{
   return view_programs_list_get(VIEW_DATA);
}

EOLIAN static Eina_List *
_enventor_object_part_states_list_get(Eo *obj EINA_UNUSED,
                                      Enventor_Object_Data *pd EINA_UNUSED,
                                      const char *part)
{
   return view_part_states_list_get(VIEW_DATA, part);
}

EOLIAN static Edje_Part_Type
_enventor_object_part_type_get(Eo *obj EINA_UNUSED,
                               Enventor_Object_Data *pd EINA_UNUSED,
                               const char *part_name)
{
   return view_part_type_get(VIEW_DATA, part_name);
}

EOLIAN static Eina_List *
_enventor_object_parts_list_get(Eo *obj EINA_UNUSED,
                                Enventor_Object_Data *pd EINA_UNUSED)
{
   return view_parts_list_get(VIEW_DATA);
}

EOLIAN static void
_enventor_object_linenumber_set(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                                Eina_Bool linenumber)
{
   linenumber = !!linenumber;

   if (pd->linenumber == linenumber) return;

   //Main Item
   edit_linenumber_set(pd->main_it.ed, linenumber);

   pd->linenumber = linenumber;
}

EOLIAN static Eina_Bool
_enventor_object_linenumber_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return pd->linenumber;
}

EOLIAN static void
_enventor_object_smart_undo_redo_set(Eo *obj EINA_UNUSED,
                                     Enventor_Object_Data *pd,
                                     Eina_Bool smart_undo_redo)
{
   smart_undo_redo = !!smart_undo_redo;
   pd->smart_undo_redo = smart_undo_redo;
}

EOLIAN static Eina_Bool
_enventor_object_smart_undo_redo_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return pd->smart_undo_redo;
}

EOLIAN static void
_enventor_object_auto_indent_set(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                                 Eina_Bool auto_indent)
{
   pd->auto_indent = !!auto_indent;
}

EOLIAN static Eina_Bool
_enventor_object_auto_indent_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return pd->auto_indent;
}

EOLIAN static void
_enventor_object_auto_complete_set(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd EINA_UNUSED,
                                   Eina_Bool auto_complete)
{
   autocomp_enabled_set(auto_complete);
}

EOLIAN static Eina_Bool
_enventor_object_auto_complete_get(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd EINA_UNUSED)
{
   return autocomp_enabled_get();
}

EOLIAN static void
_enventor_object_auto_complete_list_show(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd EINA_UNUSED)
{
   autocomp_list_show();
}

EOLIAN static void
_enventor_object_modified_set(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                                  Eina_Bool modified)
{
   //Main Item
   edit_changed_set(pd->main_it.ed, modified);
}

EOLIAN static Eina_Bool
_enventor_object_modified_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   //Main Item
   return edit_changed_get(pd->main_it.ed);
}

EOLIAN static Eina_Bool
_enventor_object_path_set(Eo *obj EINA_UNUSED,
                          Enventor_Object_Data *pd EINA_UNUSED,
                          Enventor_Path_Type type, const Eina_List *pathes)
{
   //edj file is changed
   if (type == ENVENTOR_PATH_TYPE_EDJ)
     {
        const char *path = eina_list_data_get(pathes);
        const char *ppath = build_edj_path_get();
        if (path && ppath && strcmp(path, ppath))
          {
             edj_mgr_clear();
             eina_stringshare_del(pd->group_name);
             pd->group_name = NULL;
          }
     }
   return build_path_set(type, pathes);
}

EOLIAN static const Eina_List *
_enventor_object_path_get(Eo *obj EINA_UNUSED,
                          Enventor_Object_Data *pd EINA_UNUSED,
                          Enventor_Path_Type type)
{
   return build_path_get(type);
}

EOLIAN static void
_enventor_object_live_view_scale_set(Eo *obj EINA_UNUSED,
                                     Enventor_Object_Data *pd EINA_UNUSED,
                                     double scale)
{
   edj_mgr_view_scale_set(scale);
}

EOLIAN static void
_enventor_object_live_view_size_set(Eo *obj EINA_UNUSED,
                                    Enventor_Object_Data *pd EINA_UNUSED,
                                    Evas_Coord w, Evas_Coord h)
{
   view_size_set(VIEW_DATA, w, h);
}

EOLIAN static void
_enventor_object_live_view_size_get(Eo *obj EINA_UNUSED,
                                    Enventor_Object_Data *pd EINA_UNUSED,
                                    Evas_Coord *w, Evas_Coord *h)
{
   view_size_get(VIEW_DATA, w, h);
}

EOLIAN static double
_enventor_object_live_view_scale_get(Eo *obj EINA_UNUSED,
                                     Enventor_Object_Data *pd EINA_UNUSED)
{
   return edj_mgr_view_scale_get();
}

EOLIAN static void
_enventor_object_dummy_parts_set(Eo *obj EINA_UNUSED,
                                 Enventor_Object_Data *pd,
                                 Eina_Bool dummy_parts)
{
   dummy_parts = !!dummy_parts;
   if (pd->dummy_parts == dummy_parts) return;

   view_dummy_set(VIEW_DATA, dummy_parts);
   pd->dummy_parts = dummy_parts;
}

EOLIAN static Eina_Bool
_enventor_object_ctxpopup_get(Eo *obj EINA_UNUSED,
                              Enventor_Object_Data *pd)
{
   return pd->ctxpopup;
}

EOLIAN static void
_enventor_object_ctxpopup_set(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                              Eina_Bool ctxpopup)
{
   ctxpopup = !!ctxpopup;
   pd->ctxpopup = ctxpopup;
}

EOLIAN static Eina_Bool
_enventor_object_ctxpopup_visible_get(Eo *obj EINA_UNUSED,
                                      Enventor_Object_Data *pd)
{
   return edit_ctxpopup_visible_get(pd->main_it.ed);
}

EOLIAN static void
_enventor_object_ctxpopup_dismiss(Eo *obj EINA_UNUSED,
                                  Enventor_Object_Data *pd)
{
   edit_ctxpopup_dismiss(pd->main_it.ed);
}

EOLIAN static Eina_Bool
_enventor_object_dummy_parts_get(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd)
{
   return pd->dummy_parts;
}

EOLIAN static void
_enventor_object_part_highlight_set(Eo *obj EINA_UNUSED,
                                    Enventor_Object_Data *pd,
                                    Eina_Bool part_highlight)
{
   part_highlight = !!part_highlight;
   if (pd->part_highlight == part_highlight) return;
   pd->part_highlight = part_highlight;

   //Main Item
   if (part_highlight) edit_view_sync(pd->main_it.ed);
   else view_part_highlight_set(VIEW_DATA, NULL);
}

EOLIAN static Eina_Bool
_enventor_object_part_highlight_get(Eo *obj EINA_UNUSED,
                                    Enventor_Object_Data *pd)
{
   return pd->part_highlight;
}

EOLIAN static void
_enventor_object_mirror_mode_set(Eo *obj EINA_UNUSED,
                                 Enventor_Object_Data *pd,
                                 Eina_Bool mirror_mode)
{
   pd->mirror_mode = !!mirror_mode;
   view_mirror_mode_update(VIEW_DATA);
}

EOLIAN static Eina_Bool
_enventor_object_mirror_mode_get(Eo *obj EINA_UNUSED,
                                 Enventor_Object_Data *pd)
{
   return pd->mirror_mode;
}

EOLIAN static void
_enventor_object_focus_set(Eo *obj EINA_UNUSED,
                           Enventor_Object_Data *pd EINA_UNUSED,
                           Eina_Bool focus)
{
   elm_object_focus_set(edit_entry_get(pd->main_it.ed), focus);
}

EOLIAN static Eina_Bool
_enventor_object_focus_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return elm_object_focus_get(edit_entry_get(pd->main_it.ed));
}

//TODO: Itemize
EOLIAN static const char *
_enventor_object_text_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return elm_entry_entry_get(edit_entry_get(pd->main_it.ed));
}

//TODO: Itemize
EOLIAN static int
_enventor_object_cursor_pos_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return elm_entry_cursor_pos_get(edit_entry_get(pd->main_it.ed));
}

//TODO: Itemize
EOLIAN static void
_enventor_object_cursor_pos_set(Eo *obj EINA_UNUSED,
                                Enventor_Object_Data *pd,
                                int position)
{
   elm_entry_cursor_pos_set(edit_entry_get(pd->main_it.ed), position);
}

//TODO: Itemize
EOLIAN static const char *
_enventor_object_selection_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return elm_entry_selection_get(edit_entry_get(pd->main_it.ed));
}

//TODO: Itemize
EOLIAN static void
_enventor_object_select_none(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   elm_entry_select_none(edit_entry_get(pd->main_it.ed));
}

//TODO: Itemize
EOLIAN static void
_enventor_object_text_insert(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                             const char *text)
{
   edit_text_insert(pd->main_it.ed, text);
}

//TODO: Itemize
EOLIAN static void
_enventor_object_select_region_set(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd, int start, int end)
{
   edit_selection_clear(pd->main_it.ed);
   elm_entry_cursor_pos_set(edit_entry_get(pd->main_it.ed), start);
   elm_entry_select_region_set(edit_entry_get(pd->main_it.ed), start, end);
}

EOLIAN static void
_enventor_object_font_scale_set(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                                double font_scale)
{
   if (pd->font_scale == font_scale) return;
   pd->font_scale = font_scale;

   //Main Item
   edit_font_scale_set(pd->main_it.ed, font_scale);
}

EOLIAN static double
_enventor_object_font_scale_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return pd->font_scale;
}

EOLIAN static void
_enventor_object_font_set(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                          const char *font_name, const char *font_style)
{
   if (!font_name) return;

   eina_stringshare_replace(&pd->font_name, font_name);
   eina_stringshare_replace(&pd->font_style, font_style);

   char *font = NULL;
   if (font_name) font = elm_font_fontconfig_name_get(font_name, font_style);
   elm_config_font_overlay_set("enventor_entry", font, -100);
   elm_config_font_overlay_apply();
   elm_config_save();

   elm_font_fontconfig_name_free(font);

   //Main Item
   edit_font_update(pd->main_it.ed);
}

EOLIAN static void
_enventor_object_font_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                          const char **font_name, const char **font_style)
{
   if (font_name) *font_name = pd->font_name;
   if (font_style) *font_style = pd->font_style;
}

//TODO: Itemize
EOLIAN static int
_enventor_object_max_line_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return edit_max_line_get(pd->main_it.ed);
}

//TODO: Itemize
EOLIAN static void
_enventor_object_line_goto(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                           int line)
{
   edit_goto(pd->main_it.ed, line);
}

EOLIAN static void
_enventor_object_syntax_color_set(Eo *obj EINA_UNUSED,
                                  Enventor_Object_Data *pd,
                                  Enventor_Syntax_Color_Type color_type,
                                  const char *val)
{
   edit_syntax_color_set(pd->main_it.ed, color_type, val);
}

EOLIAN static const char *
_enventor_object_syntax_color_get(Eo *obj EINA_UNUSED,
                                  Enventor_Object_Data *pd,
                                  Enventor_Syntax_Color_Type color_type)
{
   return edit_syntax_color_get(pd->main_it.ed, color_type);
}

EOLIAN static void
_enventor_object_syntax_color_full_apply(Eo *obj EINA_UNUSED,
                                         Enventor_Object_Data *pd,
                                         Eina_Bool force)
{
   edit_syntax_color_full_apply(pd->main_it.ed, force);
}

EOLIAN static void
_enventor_object_syntax_color_partial_apply(Eo *obj EINA_UNUSED,
                                            Enventor_Object_Data *pd,
                                            double interval)
{
   edit_syntax_color_partial_apply(pd->main_it.ed, interval);
}

//TODO: Might need for items
EOLIAN static Eina_Bool
_enventor_object_save(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                      const char *file)
{
   //Update edc file and try to save if the edc path is different.
   if (build_edc_path_get() != file)
     edit_changed_set(pd->main_it.ed, EINA_TRUE);

   Eina_Bool saved = edit_save(pd->main_it.ed, file);
   if (saved) build_edc();
   return saved;
}

//TODO: Itemize
EOLIAN static void
_enventor_object_line_delete(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   edit_line_delete(pd->main_it.ed);
   //Close auto-completion popup if it's shown.
   autocomp_reset();
}

EOLIAN static Eo *
_enventor_object_live_view_get(Eo *obj EINA_UNUSED,
                               Enventor_Object_Data *pd EINA_UNUSED)
{
   return edj_mgr_obj_get();
}

EOLIAN static Eina_Bool
_enventor_object_template_insert(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                                 char *syntax, size_t n)
{
   return template_insert(pd->main_it.ed, syntax, n);
}

EOLIAN static Eina_Bool
_enventor_object_template_part_insert(Eo *obj EINA_UNUSED,
                                      Enventor_Object_Data *pd,
                                      Edje_Part_Type part,
                                      Enventor_Template_Insert_Type insert_type,
                                      Eina_Bool fixed_w, Eina_Bool fixed_h,
                                      char *rel1_x_to, char *rel1_y_to,
                                      char *rel2_x_to, char *rel2_y_to,
                                      float align_x, float align_y,
                                      int min_w, int min_h,
                                      float rel1_x, float rel1_y,
                                      float rel2_x,float rel2_y,
                                      char *syntax, size_t n)
{
   // if mirror mode, exchange properties about left and right
   if (pd->mirror_mode)
     {
       float x1, x2;
       x1 = 1.0 - rel2_x;
       x2 = 1.0 - rel1_x;
       rel1_x = x1;
       rel2_x = x2;

       if (align_x == 0.0)
         align_x = 1.0;
       else if (align_x == 1.0)
         align_x = 0.0;

       char *buf;
       buf = rel1_x_to;
       rel1_x_to = rel2_x_to;
       rel2_x_to =  buf;
     }

   return template_part_insert(pd->main_it.ed, part, insert_type,
                               fixed_w, fixed_h,
                               rel1_x_to, rel1_y_to,
                               rel2_x_to, rel2_y_to,
                               align_x, align_y, min_w, min_h,
                               rel1_x, rel1_y, rel2_x, rel2_y,
                               NULL, syntax, n);
}

//TODO: Might need for items
EOLIAN static void
_enventor_object_disabled_set(Eo *obj EINA_UNUSED,
                              Enventor_Object_Data *pd,
                              Eina_Bool disabled)
{
   disabled = !!disabled;
   if (pd->disabled == disabled) return;

   edit_disabled_set(pd->main_it.ed, disabled);

   pd->disabled = !!disabled;
}

//TODO: Itemize
EOLIAN static Eina_Bool
_enventor_object_redo(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return edit_redoundo(pd->main_it.ed, EINA_FALSE);
}

//TODO: Itemize
EOLIAN static Eina_Bool
_enventor_object_undo(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return edit_redoundo(pd->main_it.ed, EINA_TRUE);
}

EOLIAN static void
_enventor_object_program_run(Eo *obj EINA_UNUSED,
                             Enventor_Object_Data *pd EINA_UNUSED,
                             const char *program)
{
   view_program_run(VIEW_DATA, program);
}

EOLIAN static void
_enventor_object_programs_stop(Eo *obj EINA_UNUSED,
                               Enventor_Object_Data *pd EINA_UNUSED)
{
   view_programs_stop(VIEW_DATA);
}


/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/
EAPI Enventor_Object *
enventor_object_add(Enventor_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   Evas_Object *obj = eo_add(MY_CLASS, parent);
   return obj;
}

EAPI Enventor_Item *
enventor_object_main_file_set(Enventor_Object *obj, const char *file)
{
   Enventor_Object_Data *pd = eo_data_scope_get(obj, ENVENTOR_OBJECT_CLASS);

   //FIXME:...
   autocomp_target_set(NULL);

   _enventor_main_item_free(pd);

   pd->main_it.enventor = obj;
   pd->main_it.ed = edit_init(obj);
   edit_view_sync_cb_set(pd->main_it.ed, edit_view_sync_cb, pd);

   Eina_Bool ret;
   if (!eo_do_ret(obj, ret, efl_file_set(file, NULL))) return NULL;

   //Update Editor State
   if (pd->linenumber != DEFAULT_LINENUMBER)
     edit_linenumber_set(pd->main_it.ed, pd->linenumber);
   if (pd->font_scale != DEFAULT_FONT_SCALE)
     edit_font_scale_set(pd->main_it.ed, pd->font_scale);
   if (pd->disabled)
     edit_disabled_set(pd->main_it.ed, EINA_TRUE);

   return &pd->main_it;
}

Evas_Object *
enventor_item_editor_get(const Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, NULL);

   if (!it->ed) return NULL;

   return edit_obj_get(it->ed);
}

#include "enventor_object.eo.c"
