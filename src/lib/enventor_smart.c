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

typedef struct _Enventor_Object_Data
{
   EINA_REFCOUNT;
   Evas_Object *obj;

   edit_data *ed;
   Eina_Stringshare *group_name;

   Ecore_Event_Handler *key_down_handler;
   Ecore_Event_Handler *key_up_handler;

   Eina_Bool dummy_parts : 1;
   Eina_Bool key_down : 1;
   Eina_Bool part_cursor_jump : 1;
   Eina_Bool mirror_mode : 1;

} Enventor_Object_Data;

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CURSOR_LINE_CHANGED, ""},
   {SIG_CURSOR_GROUP_CHANGED, ""},
   {SIG_LIVE_VIEW_CURSOR_MOVED, ""},
   {SIG_LIVE_VIEW_RESIZED, ""},
   {SIG_LIVE_VIEW_LOADED, ""},
   {SIG_LIVE_VIEW_UPDATED, ""},
   {SIG_MAX_LINE_CHANGED, ""},
   {SIG_COMPILE_ERROR, ""},
   {SIG_PROGRAM_RUN, ""},
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
static Eina_Bool
key_up_cb(void *data, int type EINA_UNUSED, void *ev EINA_UNUSED)
{
   Enventor_Object_Data *pd = data;
   pd->key_down = EINA_FALSE;
   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
key_down_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Enventor_Object_Data *pd = data;
   Ecore_Event_Key *event = ev;
   Eina_Bool ret;

   ret = enventor_object_focus_get(pd->obj);
   if (!ret) return ECORE_CALLBACK_PASS_ON;

   if (pd->key_down) return ECORE_CALLBACK_PASS_ON;
   pd->key_down = EINA_TRUE;

   if (autocomp_event_dispatch(event->key)) return ECORE_CALLBACK_DONE;
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
   if (edit_part_highlight_get(pd->ed))
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
   edit_error_set(pd->ed, line_num - 1, target);
   if (line_num || target)
     edit_syntax_color_full_apply(pd->ed, EINA_TRUE);

   redoundo_data *rd = evas_object_data_get(edit_entry_get(pd->ed), "redoundo");

   // When msg == NULL it mean, that needed to reset error state
   if (msg)
     {
        // Ctxpopup should be dismissed only in cases when error happens
        edit_ctxpopup_dismiss(pd->ed);
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
   if (!pd->part_cursor_jump) return;
   const char *part_name = (const char *)ei;
   edit_part_cursor_set(pd->ed, view_group_name_get(VIEW_DATA), part_name);
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
   EINA_REFCOUNT_INIT(pd);
   pd->obj = obj;

   elm_widget_sub_object_parent_add(obj);
   evas_obj_smart_add(eo_super(obj, MY_CLASS));

   build_init();
   autocomp_init();
   edj_mgr_init(obj);
   pd->ed = edit_init(obj);
   edit_view_sync_cb_set(pd->ed, edit_view_sync_cb, pd);
   build_err_noti_cb_set(build_err_noti_cb, pd);

   evas_object_smart_member_add(edit_obj_get(pd->ed), obj);
   elm_widget_can_focus_set(obj, EINA_FALSE);

   pd->key_down_handler =
      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, key_down_cb, pd);
   pd->key_up_handler =
      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, key_up_cb, pd);

   evas_object_smart_callback_add(pd->obj, "part,clicked",
                                  _enventor_part_clicked_cb, pd);

   pd->part_cursor_jump = EINA_TRUE;
}

EOLIAN static void
_enventor_object_evas_object_smart_del(Evas_Object *obj EINA_UNUSED,
                                       Enventor_Object_Data *pd)
{
   EINA_REFCOUNT_UNREF(pd)
     {
        eina_stringshare_del(pd->group_name);
        autocomp_term();
        edit_term(pd->ed);
        ecore_event_handler_del(pd->key_down_handler);
        ecore_event_handler_del(pd->key_up_handler);
        edj_mgr_term();
        build_term();
     }
}

EOLIAN static void
_enventor_object_evas_object_smart_move(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd, Evas_Coord x, Evas_Coord y)
{
   Evas_Object *o = edit_obj_get(pd->ed);
   evas_object_move(o, x, y);
}

EOLIAN static void
_enventor_object_evas_object_smart_resize(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd, Evas_Coord w, Evas_Coord h)
{
   Evas_Object *o = edit_obj_get(pd->ed);
   evas_object_resize(o, w, h);
}

EOLIAN static void
_enventor_object_evas_object_smart_show(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   Evas_Object *o = edit_obj_get(pd->ed);
   evas_object_show(o);
}

EOLIAN static void
_enventor_object_evas_object_smart_hide(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   Evas_Object *o = edit_obj_get(pd->ed);
   evas_object_hide(o);
}

EOLIAN static void
_enventor_object_evas_object_smart_color_set(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd, int r, int g, int b, int a)
{
   Evas_Object *o = edit_obj_get(pd->ed);
   evas_object_color_set(o, r, g, b, a);
}

EOLIAN static void
_enventor_object_evas_object_smart_clip_set(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd, Evas_Object *clip)
{
   Evas_Object *o = edit_obj_get(pd->ed);
   evas_object_clip_set(o, clip);
}

EOLIAN static void
_enventor_object_evas_object_smart_clip_unset(Evas_Object *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   Evas_Object *o = edit_obj_get(pd->ed);
   evas_object_clip_unset(o);
}

EOLIAN static Eo *
_enventor_object_eo_base_constructor(Eo *obj,
                                     Enventor_Object_Data *pd EINA_UNUSED)
{
   obj = eo_constructor(eo_super(obj, MY_CLASS));
   evas_obj_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_obj_smart_callbacks_descriptions_set(obj, _smart_callbacks);

   return obj;
}

EOLIAN static Eina_Bool
_enventor_object_efl_file_file_set(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd,
                                   const char *file,
                                   const char *group EINA_UNUSED)
{
   build_edc_path_set(file);
   autocomp_target_set(pd->ed);
   if (!file) goto err;

   /* Create empty file*/
   if (!ecore_file_exists(file))
     {
         FILE *fp = fopen(file, "w");
         fclose(fp);
     }

   edit_load(pd->ed, file);
   build_edc();
   edit_changed_set(pd->ed, EINA_FALSE);

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
   edit_linenumber_set(pd->ed, linenumber);
}

EOLIAN static Eina_Bool
_enventor_object_linenumber_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return edit_linenumber_get(pd->ed);
}

EOLIAN static void
_enventor_object_smart_undo_redo_set(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                                     Eina_Bool smart_undo_redo)
{
   edit_smart_undo_redo_set(pd->ed, smart_undo_redo);
}

EOLIAN static Eina_Bool
_enventor_object_smart_undo_redo_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return edit_smart_undo_redo_get(pd->ed);
}

EOLIAN static void
_enventor_object_auto_indent_set(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                                 Eina_Bool auto_indent)
{
   edit_auto_indent_set(pd->ed, auto_indent);
}

EOLIAN static Eina_Bool
_enventor_object_auto_indent_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return edit_auto_indent_get(pd->ed);
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
   edit_changed_set(pd->ed, modified);
}

EOLIAN static Eina_Bool
_enventor_object_modified_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return edit_changed_get(pd->ed);
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
   return edit_ctxpopup_enabled_get(pd->ed);
}

EOLIAN static void
_enventor_object_ctxpopup_set(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                              Eina_Bool ctxpopup)
{
   edit_ctxpopup_enabled_set(pd->ed, ctxpopup);
}

EOLIAN static Eina_Bool
_enventor_object_ctxpopup_visible_get(Eo *obj EINA_UNUSED,
                                      Enventor_Object_Data *pd)
{
   return edit_ctxpopup_visible_get(pd->ed);
}

EOLIAN static void
_enventor_object_ctxpopup_dismiss(Eo *obj EINA_UNUSED,
                                  Enventor_Object_Data *pd)
{
   edit_ctxpopup_dismiss(pd->ed);
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
   edit_part_highlight_set(pd->ed, part_highlight);
}

EOLIAN static Eina_Bool
_enventor_object_part_highlight_get(Eo *obj EINA_UNUSED,
                                    Enventor_Object_Data *pd)
{
   return edit_part_highlight_get(pd->ed);
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
   elm_object_focus_set(edit_entry_get(pd->ed), focus);
}

EOLIAN static Eina_Bool
_enventor_object_focus_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return elm_object_focus_get(edit_entry_get(pd->ed));
}

EOLIAN static const char *
_enventor_object_text_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return elm_entry_entry_get(edit_entry_get(pd->ed));
}

EOLIAN static int
_enventor_object_cursor_pos_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return elm_entry_cursor_pos_get(edit_entry_get(pd->ed));
}

EOLIAN static void
_enventor_object_cursor_pos_set(Eo *obj EINA_UNUSED,
                                Enventor_Object_Data *pd,
                                int position)
{
   elm_entry_cursor_pos_set(edit_entry_get(pd->ed), position);
}

EOLIAN static const char *
_enventor_object_selection_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return elm_entry_selection_get(edit_entry_get(pd->ed));
}

EOLIAN static void
_enventor_object_select_none(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   elm_entry_select_none(edit_entry_get(pd->ed));
}

EOLIAN static void
_enventor_object_text_insert(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                             const char *text)
{
   edit_text_insert(pd->ed, text);
}

EOLIAN static void
_enventor_object_select_region_set(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd, int start, int end)
{
   edit_selection_clear(pd->ed);
   elm_entry_cursor_pos_set(edit_entry_get(pd->ed), start);
   elm_entry_select_region_set(edit_entry_get(pd->ed), start, end);
}

EOLIAN static void
_enventor_object_font_scale_set(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                                double font_scale)
{
   edit_font_scale_set(pd->ed, font_scale);
}

EOLIAN static double
_enventor_object_font_scale_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return edit_font_scale_get(pd->ed);
}

EOLIAN static void
_enventor_object_font_set(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                          const char *font_name, const char *font_style)
{
   edit_font_set(pd->ed, font_name, font_style);
}

EOLIAN static void
_enventor_object_font_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                          const char **font_name, const char **font_style)
{
   edit_font_get(pd->ed, font_name, font_style);
}

EOLIAN static int
_enventor_object_max_line_get(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return edit_max_line_get(pd->ed);
}

EOLIAN static void
_enventor_object_line_goto(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                           int line)
{
   edit_goto(pd->ed, line);
}

EOLIAN static void
_enventor_object_syntax_color_set(Eo *obj EINA_UNUSED,
                                  Enventor_Object_Data *pd,
                                  Enventor_Syntax_Color_Type color_type,
                                  const char *val)
{
   edit_syntax_color_set(pd->ed, color_type, val);
}

EOLIAN static const char *
_enventor_object_syntax_color_get(Eo *obj EINA_UNUSED,
                                  Enventor_Object_Data *pd,
                                  Enventor_Syntax_Color_Type color_type)
{
   return edit_syntax_color_get(pd->ed, color_type);
}

EOLIAN static void
_enventor_object_syntax_color_full_apply(Eo *obj EINA_UNUSED,
                                         Enventor_Object_Data *pd,
                                         Eina_Bool force)
{
   edit_syntax_color_full_apply(pd->ed, force);
}

EOLIAN static void
_enventor_object_syntax_color_partial_apply(Eo *obj EINA_UNUSED,
                                            Enventor_Object_Data *pd,
                                            double interval)
{
   edit_syntax_color_partial_apply(pd->ed, interval);
}

EOLIAN static Eina_Bool
_enventor_object_save(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                      const char *file)
{
   //Update edc file and try to save if the edc path is different.
   if (build_edc_path_get() != file) edit_changed_set(pd->ed, EINA_TRUE);

   Eina_Bool saved = edit_save(pd->ed, file);
   if (saved) build_edc();
   return saved;
}

EOLIAN static void
_enventor_object_line_delete(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   edit_line_delete(pd->ed);
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
                                 Enventor_Template_Insert_Type insert_type,
                                 char *syntax, size_t n)
{
   return template_insert(pd->ed, insert_type, syntax, n);
}

EOLIAN static Eina_Bool
_enventor_object_template_part_insert(Eo *obj EINA_UNUSED,
                                      Enventor_Object_Data *pd,
                                      Edje_Part_Type part,
                                      Enventor_Template_Insert_Type insert_type,
                                      float rel1_x, float rel1_y, float rel2_x,
                                      float rel2_y, char *syntax, size_t n)
{
   return template_part_insert(pd->ed, part, insert_type, rel1_x, rel1_y, rel2_x,
                               rel2_y, NULL, syntax, n);
}

EOLIAN static void
_enventor_object_disabled_set(Eo *obj EINA_UNUSED,
                              Enventor_Object_Data *pd,
                              Eina_Bool disabled)
{
   edit_disabled_set(pd->ed, disabled);

   if (disabled) pd->part_cursor_jump = EINA_FALSE;
   else pd->part_cursor_jump = EINA_TRUE;
}

EOLIAN static Eina_Bool
_enventor_object_redo(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return edit_redoundo(pd->ed, EINA_FALSE);
}

EOLIAN static Eina_Bool
_enventor_object_undo(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   return edit_redoundo(pd->ed, EINA_TRUE);
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/
EAPI Evas_Object *
enventor_object_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   Evas_Object *obj = eo_add(MY_CLASS, parent);
   return obj;
}

EAPI Eina_Bool
enventor_object_file_set(Evas_Object *obj, const char *file)
{
   return efl_file_set(obj, file, NULL);
}

#include "enventor_object.eo.c"
