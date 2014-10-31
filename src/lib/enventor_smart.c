#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#define ELM_INTERNAL_API_ARGESFSDFEFC 1
#define ENVENTOR_BETA_API_SUPPORT 1

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
   Eio_Monitor *edc_monitor;
   Eina_Stringshare *group_name;

   Eina_Bool dummy_swallow : 1;

} Enventor_Object_Data;

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CURSOR_LINE_CHANGED, ""},
   {SIG_CURSOR_GROUP_CHANGED, ""},
   {SIG_LIVE_VIEW_CURSOR_MOVED, ""},
   {SIG_LIVE_VIEW_RESIZED, ""},
   {SIG_MAX_LINE_CHANGED, ""},
   {SIG_COMPILE_ERROR, ""},
   {SIG_PROGRAM_RUN, ""},
   {SIG_CTXPOPUP_SELECTED, ""},
   {SIG_CTXPOPUP_DISMISSED, ""},
   {SIG_EDC_MODIFIED, ""},
   {NULL, NULL}
};

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/
static Eina_Bool
file_modified_cb(void *data, int type EINA_UNUSED, void *event)
{
   Eio_Monitor_Event *ev = event;
   Enventor_Object_Data *pd = data;
   Enventor_EDC_Modified modified;

   if (ev->monitor != pd->edc_monitor) return ECORE_CALLBACK_PASS_ON;
   if (!edit_saved_get(pd->ed))
     {
        modified.self_changed = EINA_FALSE;
        evas_object_smart_callback_call(pd->obj, SIG_EDC_MODIFIED, &modified);
        return ECORE_CALLBACK_DONE;
     }
   if (!edit_changed_get(pd->ed)) return ECORE_CALLBACK_DONE;
   if (strcmp(ev->filename, build_edc_path_get())) return ECORE_CALLBACK_DONE;

   build_edc();
   edit_changed_set(pd->ed, EINA_FALSE);

   edit_saved_set(pd->ed, EINA_FALSE);
   modified.self_changed = EINA_TRUE;
   evas_object_smart_callback_call(pd->obj, SIG_EDC_MODIFIED, &modified);

   return ECORE_CALLBACK_DONE;
}

static void
edit_view_sync_cb(void *data, Eina_Stringshare *part_name,
                  Eina_Stringshare *group_name)
{
   Enventor_Object_Data *pd = data;
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
   view_part_highlight_set(VIEW_DATA, part_name);
}

static void
build_err_noti_cb(void *data, const char *msg)
{
   Evas_Object *enventor = data;
   evas_object_smart_callback_call(enventor, SIG_COMPILE_ERROR, (char *)msg);
}

/*****************************************************************************/
/* Internal Eo object required routines                                      */
/*****************************************************************************/
static void
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
   eo_do_super(obj, MY_CLASS, evas_obj_smart_add());

   build_init();
   autocomp_init();
   edj_mgr_init(obj);
   pd->ed = edit_init(obj);
   edit_view_sync_cb_set(pd->ed, edit_view_sync_cb, pd);
   build_err_noti_cb_set(build_err_noti_cb, obj);

   evas_object_smart_member_add(edit_obj_get(pd->ed), obj);
   elm_widget_can_focus_set(obj, EINA_FALSE);

   ecore_event_handler_add(EIO_MONITOR_FILE_MODIFIED, file_modified_cb, pd);
}

EOLIAN static void
_enventor_object_evas_object_smart_del(Evas_Object *obj EINA_UNUSED,
                                       Enventor_Object_Data *pd)
{
   EINA_REFCOUNT_UNREF(pd)
     {
        eio_monitor_del(pd->edc_monitor);
        eina_stringshare_del(pd->group_name);
        edit_term(pd->ed);
        edj_mgr_term();
        autocomp_term();
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

EOLIAN static void
_enventor_object_eo_base_constructor(Eo *obj,
                                     Enventor_Object_Data *pd EINA_UNUSED)
{
   eo_do_super(obj, MY_CLASS, eo_constructor());
   eo_do(obj,
         evas_obj_type_set(MY_CLASS_NAME_LEGACY),
         evas_obj_smart_callbacks_descriptions_set(_smart_callbacks));
}

EOLIAN static Eina_Bool
_enventor_object_efl_file_file_set(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd,
                                   const char *file,
                                   const char *group EINA_UNUSED)
{
   eio_monitor_del(pd->edc_monitor);
   build_edc_path_set(file);
   if (!edit_load(pd->ed, file)) goto err;
   autocomp_target_set(pd->ed);
   pd->edc_monitor = eio_monitor_add(file);
   build_edc();
   edit_changed_set(pd->ed, EINA_FALSE);

   return EINA_TRUE;

err:
   build_edc_path_set(NULL);
   pd->edc_monitor = NULL;
   return EINA_FALSE;
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

EOLIAN static double
_enventor_object_live_view_scale_get(Eo *obj EINA_UNUSED,
                                     Enventor_Object_Data *pd EINA_UNUSED)
{
   return edj_mgr_view_scale_get();
}

EOLIAN static void
_enventor_object_dummy_swallow_set(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd,
                                   Eina_Bool dummy_swallow)
{
   dummy_swallow = !!dummy_swallow;
   if (pd->dummy_swallow == dummy_swallow) return;

   view_dummy_set(VIEW_DATA, dummy_swallow);
   pd->dummy_swallow = dummy_swallow;
}

EOLIAN static Eina_Bool
_enventor_object_ctxpopup_get(Eo *obj EINA_UNUSED,
                              Enventor_Object_Data *pd)
{
   return edit_ctxpopup_get(pd->ed);
}

EOLIAN static void
_enventor_object_ctxpopup_set(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd,
                                  Eina_Bool ctxpopup)
{
   edit_ctxpopup_set(pd->ed, ctxpopup);
}

EOLIAN static Eina_Bool
_enventor_object_dummy_swallow_get(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd)
{
   return pd->dummy_swallow;
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
   elm_entry_entry_insert(edit_entry_get(pd->ed), text);
}

EOLIAN static void
_enventor_object_select_region_set(Eo *obj EINA_UNUSED,
                                   Enventor_Object_Data *pd, int start, int end)
{
   edit_selection_clear(pd->ed);
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
   return edit_save(pd->ed, file);
}

EOLIAN static void
_enventor_object_line_delete(Eo *obj EINA_UNUSED, Enventor_Object_Data *pd)
{
   edit_line_delete(pd->ed);
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
   return template_insert(pd->ed, TEMPLATE_INSERT_DEFAULT, syntax, n);
}

EOLIAN static Eina_Bool
_enventor_object_template_part_insert(Eo *obj EINA_UNUSED,
                                      Enventor_Object_Data *pd,
                                      Edje_Part_Type part, float rel1_x,
                                      float rel1_y, float rel2_x, float rel2_y,
                                      char *syntax, size_t n)
{
   return template_part_insert(pd->ed, part, TEMPLATE_INSERT_DEFAULT, rel1_x,
                              rel1_y, rel2_x, rel2_y, NULL, syntax, n);
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
   return eo_do(obj, efl_file_set(file, NULL));
}

#include "enventor_object.eo.c"
