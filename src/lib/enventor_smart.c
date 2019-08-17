#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#define ELM_INTERNAL_API_ARGESFSDFEFC 1
#define EFL_CANVAS_OBJECT_PROTECTED 1
#define EFL_CANVAS_GROUP_PROTECTED
#define EFL_CANVAS_GROUP_BETA

#include <Enventor.h>
#include <Eio.h>
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
   Enventor_Object_Data *pd;
};

struct _Enventor_Object_Data
{
   Enventor_Object *obj;
   Enventor_Item *main_it;
   Eina_List *sub_its;
   Enventor_Item *focused_it;

   Eina_Stringshare *group_name;

   Ecore_Event_Handler *key_down_handler;
   Ecore_Event_Handler *key_up_handler;

   double font_scale;
   Eina_Stringshare *font_name;
   Eina_Stringshare *font_style;
   const char *text_color_val[ENVENTOR_SYNTAX_COLOR_LAST];

   Eina_Bool dummy_parts : 1;
   Eina_Bool wireframes : 1;
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
   {SIG_FILE_OPEN_REQUESTED, ""},
   {NULL, NULL}
};

static Eina_Error ENVENTOR_FILE_SET_ERROR_NONE;
static Eina_Error ENVENTOR_FILE_SET_ERROR_GENERIC;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
_enventor_main_item_free(Enventor_Object_Data *pd)
{
   if (pd->main_it) enventor_item_del(pd->main_it);
}

static void
_enventor_sub_items_free(Enventor_Object_Data *pd)
{
   Enventor_Item *it;

   while(pd->sub_its)
     {
        it = eina_list_data_get(pd->sub_its);
        enventor_item_del(it);
     }
   pd->sub_its = NULL;
}

static Eina_Bool
key_up_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Enventor_Object_Data *pd = data;
   Ecore_Event_Key *event = ev;

   if (!pd->focused_it) return ECORE_CALLBACK_PASS_ON;

   edit_key_up_event_dispatch(pd->focused_it->ed, event->key);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
key_down_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Enventor_Object_Data *pd = data;
   Ecore_Event_Key *event = ev;

   if (!pd->focused_it) return ECORE_CALLBACK_PASS_ON;

   Eina_Bool ret = edit_focus_get(pd->focused_it->ed);
   if (!ret) return ECORE_CALLBACK_PASS_ON;

   if (edit_key_down_event_dispatch(pd->focused_it->ed, event->key))
     return ECORE_CALLBACK_DONE;

   if (autocomp_event_dispatch(event->key))
     return ECORE_CALLBACK_DONE;

   return ECORE_CALLBACK_PASS_ON;
}

static void
edit_view_sync_cb(void *data, Eina_Stringshare *state_name, double state_value,
                  Eina_Stringshare *part_name, Eina_Stringshare *group_name)
{
   Enventor_Item *it = data;
   Enventor_Object_Data *pd = it->pd;

   edj_mgr_all_views_reload();

   //Switch group!
   if (pd->group_name != group_name)
     {
        view_data *vd = edj_mgr_view_get(group_name);
        if (vd)
          {
             //If a group name equals to an other item's,
             //Probably, Enventor switches multiple collections.
             if (view_item_get(vd) == it) edj_mgr_view_switch_to(vd);
             else edj_mgr_view_switch_to(NULL);
          }
        else
          {
             vd = edj_mgr_view_new(it, group_name);
             if (!vd) edj_mgr_view_switch_to(NULL);
          }
        view_dummy_set(vd, pd->dummy_parts);
        view_wireframes_set(vd, pd->wireframes);
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
   //FIXME:
   edit_error_set(pd->main_it->ed, line_num - 1, target);
   if (line_num || target)
     edit_syntax_color_full_apply(pd->main_it->ed, EINA_TRUE);

   redoundo_data *rd = edit_redoundo_get(pd->main_it->ed);

   // When msg == NULL it mean, that needed to reset error state
   if (msg)
     {
        // Ctxpopup should be dismissed only in cases when error happens
        edit_ctxpopup_dismiss(pd->main_it->ed);
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
   if (pd->disabled || !pd->focused_it) return;
   const char *part_name = (const char *)ei;
   edit_part_cursor_set(pd->focused_it->ed, view_group_name_get(VIEW_DATA),
                        part_name);
}


/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/
#if 0
EAPI void
enventor_object_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);

   ENVENTOR_FILE_SET_ERROR_GENERIC = eina_error_msg_static_register("Generic load error");
}

EAPI void
enventor_object_efl_canvas_group_group_member_add(Enventor_Object *obj, Evas_Object *child)
{
   //Don't go through elm_widget to avoid color set.
   evas_object_data_set(child, "_elm_leaveme", (void*)1);

   efl_canvas_group_member_add(efl_super(obj, MY_CLASS), child);

   Evas_Coord x, y, w, h;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_move(child, x, y);
   evas_object_resize(child, w, h);
   evas_object_clip_set(child, evas_object_clip_get(obj));
}

EAPI void
enventor_object_efl_gfx_entity_position_set(Enventor_Object *obj, Eina_Position2D pos)
{
   efl_gfx_entity_position_set(efl_super(obj, MY_CLASS), pos);

   Eina_Iterator *it = evas_object_smart_iterator_new(obj);
   Evas_Object *o;
   EINA_ITERATOR_FOREACH(it, o)
     evas_object_move(o, pos.x, pos.y);
   eina_iterator_free(it);
}

EAPI void
enventor_object_efl_gfx_entity_size_set(Enventor_Object *obj, Eina_Size2D size)
{
   efl_gfx_entity_size_set(efl_super(obj, MY_CLASS), size);

   Eina_Iterator *it = evas_object_smart_iterator_new(obj);
   Evas_Object *o;
   EINA_ITERATOR_FOREACH(it, o)
     evas_object_resize(o, size.w, size.h);
   eina_iterator_free(it);
}

EAPI void
enventor_object_efl_gfx_entity_visible_set(Enventor_Object *obj, Eina_Bool vis)
{
   efl_gfx_entity_visible_set(efl_super(obj, MY_CLASS), vis);

   if (!pd->focused_it) return;
   Evas_Object *o = edit_obj_get(pd->focused_it->ed);
   if (vis)
     evas_object_show(o);
   else
     evas_object_hide(o);
}

EAPI void
enventor_object_efl_canvas_object_clipper_set(Enventor_Object *obj, Evas_Object *clip)
{
   efl_canvas_object_clipper_set(efl_super(obj, MY_CLASS), clip);

   Eina_Iterator *it = evas_object_smart_iterator_new(obj);
   Evas_Object *o;
   EINA_ITERATOR_FOREACH(it, o)
     evas_object_clip_set(o, clip);
   eina_iterator_free(it);
}

EAPI Enventor_Object *
enventor_object_efl_object_constructor(Enventor_Object *obj)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   pd->obj = obj;

   elm_widget_sub_object_parent_add(obj);

   build_init();
   autocomp_init();
   ref_init();
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

   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);

   return obj;
}

EAPI void
enventor_object_efl_object_destructor(Enventor_Object *obj)
{
   int i;
   for (i = ENVENTOR_SYNTAX_COLOR_STRING; i < ENVENTOR_SYNTAX_COLOR_LAST; i++)
     eina_stringshare_del(pd->text_color_val[i]);

   eina_stringshare_del(pd->font_name);
   eina_stringshare_del(pd->font_style);
   eina_stringshare_del(pd->group_name);
   autocomp_term();
   ref_term();
   ecore_event_handler_del(pd->key_down_handler);
   ecore_event_handler_del(pd->key_up_handler);

   _enventor_sub_items_free(pd);
   _enventor_main_item_free(pd);

   edj_mgr_term();
   build_term();

   efl_destructor(efl_super(obj, MY_CLASS));
}

EAPI Eina_Error
enventor_object_efl_file_file_set(Enventor_Object *obj, const char *file)
{
   build_edc_path_set(file);
   if (!file) goto err;

   if (!edit_load(pd->main_it->ed, file)) goto err;
   build_edc();
   edit_changed_set(pd->main_it->ed, EINA_FALSE);
   return 0;
err:
   eina_error_set(ENVENTOR_FILE_SET_ERROR_GENERIC);
   build_edc_path_set(NULL);
   return 1;
}
#endif

EAPI Eina_List *
enventor_object_programs_list_get(Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return view_programs_list_get(VIEW_DATA);
}

EAPI Eina_List *
enventor_object_part_states_list_get(Enventor_Object *obj, const char *part)
{
   Enventor_Object_Data *pd;

   return view_part_states_list_get(VIEW_DATA, part);
}

EAPI Edje_Part_Type
enventor_object_part_type_get(Enventor_Object *obj, const char *part_name)
{
   Enventor_Object_Data *pd;

   return view_part_type_get(VIEW_DATA, part_name);
}

EAPI Eina_List *
enventor_object_parts_list_get(Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return view_parts_list_get(VIEW_DATA);
}

EAPI void
enventor_object_linenumber_set(Enventor_Object *obj, Eina_Bool linenumber)
{
   Enventor_Object_Data *pd;

   linenumber = !!linenumber;

   if (pd->linenumber == linenumber) return;

   if (!pd->focused_it) return;

   //Main Item
   edit_linenumber_set(pd->focused_it->ed, linenumber);

   pd->linenumber = linenumber;
}

EAPI Eina_Bool
enventor_object_linenumber_get(const Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return pd->linenumber;
}

EAPI void
enventor_object_smart_undo_redo_set(Enventor_Object *obj, Eina_Bool smart_undo_redo)
{
   Enventor_Object_Data *pd;

   smart_undo_redo = !!smart_undo_redo;
   pd->smart_undo_redo = smart_undo_redo;
}

EAPI Eina_Bool
enventor_object_smart_undo_redo_get(const Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return pd->smart_undo_redo;
}

EAPI void
enventor_object_auto_indent_set(Enventor_Object *obj, Eina_Bool auto_indent)
{
   Enventor_Object_Data *pd;

   pd->auto_indent = !!auto_indent;
}

EAPI Eina_Bool
enventor_object_auto_indent_get(const Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return pd->auto_indent;
}

EAPI void
enventor_object_auto_complete_set(Enventor_Object *obj, Eina_Bool auto_complete)
{
   Enventor_Object_Data *pd;

   autocomp_enabled_set(auto_complete);
}

EAPI Eina_Bool
enventor_object_auto_complete_get(const Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return autocomp_enabled_get();
}

EAPI void
enventor_object_auto_complete_list_show(Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   autocomp_list_show();
}

EAPI Eina_Bool
enventor_object_path_set(Enventor_Object *obj, Enventor_Path_Type type,
                         Eina_List *pathes)
{
   Enventor_Object_Data *pd;

   return build_path_set(type, pathes);
}

EAPI const Eina_List *
enventor_object_path_get(Enventor_Object *obj, Enventor_Path_Type type)
{
   Enventor_Object_Data *pd;

   return build_path_get(type);
}

EAPI void
enventor_object_live_view_scale_set(Enventor_Object *obj, double scale)
{
   Enventor_Object_Data *pd;

   view_scale_set(VIEW_DATA, scale);
}

EAPI void
enventor_object_live_view_size_set(Enventor_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Enventor_Object_Data *pd;

   view_size_set(VIEW_DATA, w, h);
}

EAPI void
enventor_object_live_view_size_get(Enventor_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   Enventor_Object_Data *pd;

   view_size_get(VIEW_DATA, w, h);
}

EAPI double
enventor_object_live_view_scale_get(const Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return view_scale_get(VIEW_DATA);
}

EAPI void
enventor_object_dummy_parts_set(Enventor_Object *obj, Eina_Bool dummy_parts)
{
   Enventor_Object_Data *pd;

   dummy_parts = !!dummy_parts;
   if (pd->dummy_parts == dummy_parts) return;

   view_dummy_set(VIEW_DATA, dummy_parts);
   pd->dummy_parts = dummy_parts;
}

EAPI Eina_Bool
enventor_object_ctxpopup_get(const Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return pd->ctxpopup;
}

EAPI void
enventor_object_ctxpopup_set(Enventor_Object *obj, Eina_Bool ctxpopup)
{
   Enventor_Object_Data *pd;

   ctxpopup = !!ctxpopup;
   pd->ctxpopup = ctxpopup;
}

EAPI Eina_Bool
enventor_object_ctxpopup_visible_get(Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   if (!pd->focused_it) return EINA_FALSE;
   return edit_ctxpopup_visible_get(pd->focused_it->ed);
}

EAPI void
enventor_object_ctxpopup_dismiss(Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   if (!pd->focused_it) return;
   edit_ctxpopup_dismiss(pd->focused_it->ed);
}

EAPI Eina_Bool
enventor_object_dummy_parts_get(const Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return pd->dummy_parts;
}

EAPI void
enventor_object_wireframes_set(Enventor_Object *obj, Eina_Bool wireframes)
{
   Enventor_Object_Data *pd;

   wireframes = !!wireframes;

   view_wireframes_set(VIEW_DATA, wireframes);
   pd->wireframes = wireframes;
}

EAPI Eina_Bool
enventor_object_wireframes_get(const Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return pd->wireframes;
}

EAPI void
enventor_object_part_highlight_set(Enventor_Object *obj, Eina_Bool part_highlight)
{
   Enventor_Object_Data *pd;

   part_highlight = !!part_highlight;
   if (pd->part_highlight == part_highlight) return;
   pd->part_highlight = part_highlight;

   if (!pd->focused_it) return;

   //Main Item
   if (part_highlight) edit_view_sync(pd->focused_it->ed);
   else view_part_highlight_set(VIEW_DATA, NULL);
}

EAPI Eina_Bool
enventor_object_part_highlight_get(const Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return pd->part_highlight;
}

EAPI void
enventor_object_mirror_mode_set(Enventor_Object *obj, Eina_Bool mirror_mode)
{
   Enventor_Object_Data *pd;

   pd->mirror_mode = !!mirror_mode;
   view_mirror_mode_update(VIEW_DATA);
}

EAPI Eina_Bool
enventor_object_mirror_mode_get(const Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return pd->mirror_mode;
}

EAPI void
enventor_object_focus_set(Enventor_Object *obj, Eina_Bool focus)
{
   Enventor_Object_Data *pd;

   if (!pd->focused_it) return;

   edit_focus_set(pd->focused_it->ed, focus);
}

EAPI Eina_Bool
enventor_object_focus_get(const Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   if (!pd->focused_it) return EINA_FALSE;
   return edit_focus_get(pd->focused_it->ed);
}

EAPI void
enventor_object_font_scale_set(Enventor_Object *obj, double font_scale)
{
   Enventor_Object_Data *pd;

   if (pd->font_scale == font_scale) return;
   pd->font_scale = font_scale;

   if (!pd->focused_it) return;
     edit_font_scale_set(pd->focused_it->ed, font_scale);
}

EAPI double
enventor_object_font_scale_get(const Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return pd->font_scale;
}

EAPI void
enventor_object_font_set(Enventor_Object *obj, const char *font_name,
                         const char *font_style)
{
   Enventor_Object_Data *pd;

   if (!font_name) return;
   if ((font_name == pd->font_name) && (font_style == pd->font_style)) return;

   eina_stringshare_replace(&pd->font_name, font_name);
   eina_stringshare_replace(&pd->font_style, font_style);

   char *font = NULL;
   if (font_name) font = elm_font_fontconfig_name_get(font_name, font_style);
   elm_config_font_overlay_set("enventor_entry", font, -100);
   elm_config_font_overlay_apply();

   elm_font_fontconfig_name_free(font);
}

EAPI void
enventor_object_font_get(Enventor_Object *obj, const char **font_name,
                         const char **font_style)
{
   Enventor_Object_Data *pd;

   if (font_name) *font_name = pd->font_name;
   if (font_style) *font_style = pd->font_style;
}

EAPI void
enventor_object_syntax_color_set(Enventor_Object *obj,
                                 Enventor_Syntax_Color_Type color_type,
                                 const char *val)
{
   Enventor_Object_Data *pd;

   EINA_SAFETY_ON_NULL_RETURN(val);

   if ((color_type < ENVENTOR_SYNTAX_COLOR_STRING) ||
       (color_type >= ENVENTOR_SYNTAX_COLOR_LAST))
     {
        EINA_LOG_ERR("Invalid color_type(%d)", color_type);
        return;
     }

   eina_stringshare_del(pd->text_color_val[color_type]);
   pd->text_color_val[color_type] = eina_stringshare_add(val);

   //Main Item
   if (pd->main_it)
     edit_syntax_color_set(pd->main_it->ed, color_type, val);

   //Sub Items
   Eina_List *l;
   Enventor_Item *it;
   EINA_LIST_FOREACH(pd->sub_its, l, it)
     edit_syntax_color_set(it->ed, color_type, val);
}

EAPI const char *
enventor_object_syntax_color_get(Enventor_Object *obj,
                                 Enventor_Syntax_Color_Type color_type)
{
   Enventor_Object_Data *pd;

   if ((color_type < ENVENTOR_SYNTAX_COLOR_STRING) ||
       (color_type >= ENVENTOR_SYNTAX_COLOR_LAST))
     {
        EINA_LOG_ERR("Invalid color_type(%d)", color_type);
        return NULL;
     }

   //Return overriden color values or defaults.
   if (pd->text_color_val[color_type])
     return pd->text_color_val[color_type];
   else
     return color_value_get(color_type);
}

EAPI Eo *
enventor_object_live_view_get(Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   return edj_mgr_obj_get();
}

EAPI void
enventor_object_disabled_set(Enventor_Object *obj, Eina_Bool disabled)
{
   Enventor_Object_Data *pd;

   disabled = !!disabled;
   if (pd->disabled == disabled) return;

   if (!pd->focused_it) return;
   edit_disabled_set(pd->focused_it->ed, disabled);

   pd->disabled = !!disabled;
}

EAPI void
enventor_object_program_run(Enventor_Object *obj, const char *program)
{
   Enventor_Object_Data *pd;

   view_program_run(VIEW_DATA, program);
}

EAPI void
enventor_object_programs_stop(Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   view_programs_stop(VIEW_DATA);
}

EAPI void
enventor_object_keyword_reference_show(Enventor_Object *obj)
{
   Enventor_Object_Data *pd;

   if (!pd->focused_it) return;
   ref_show(pd->focused_it->ed);
}

EAPI double
enventor_object_base_scale_get(Enventor_Object *obj)
{
   Enventor_Object_Data *pd;
   return view_base_scale_get(VIEW_DATA);
}

EAPI Enventor_Object *
enventor_object_add(Eo *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
//   Evas_Object *obj = efl_add(MY_CLASS, parent);
//   return obj;
   return NULL;
}

EAPI Enventor_Item *
enventor_object_sub_item_add(Enventor_Object *obj, const char *file)
{
//   Enventor_Object_Data *pd = efl_data_scope_get(obj, ENVENTOR_OBJECT_CLASS);
   Enventor_Object_Data *pd;

   if (!file)
     {
        EINA_LOG_ERR("No file path!!");
        return NULL;
     }

   Enventor_Item_Data *it = calloc(1, sizeof(Enventor_Item_Data));
   if (!it)
     {
        mem_fail_msg();
        return NULL;
     }

   it->ed = edit_init(obj, it);
   it->pd = pd;

   if (!edit_load(it->ed, file))
     {
        edit_term(it->ed);
        free(it);
        return NULL;
     }

   edit_changed_set(it->ed, EINA_FALSE);
   edit_disabled_set(it->ed, EINA_TRUE);

   pd->sub_its = eina_list_append(pd->sub_its, it);

   //Update Syntax Color Here.
   int i;
   for (i = ENVENTOR_SYNTAX_COLOR_STRING; i < ENVENTOR_SYNTAX_COLOR_LAST; i++)
     {
        if (!pd->text_color_val[i]) continue;
        edit_syntax_color_set(it->ed, i, pd->text_color_val[i]);
     }

   return it;
}

EAPI Enventor_Item *
enventor_object_main_item_set(Enventor_Object *obj, const char *file)
{
//   Enventor_Object_Data *pd = efl_data_scope_get(obj, ENVENTOR_OBJECT_CLASS);
   Enventor_Object_Data *pd;
   edj_mgr_clear();
   _enventor_main_item_free(pd);

   Enventor_Item_Data *it = calloc(1, sizeof(Enventor_Item_Data));
   if (!it)
     {
        mem_fail_msg();
        return NULL;
     }

   pd->main_it = it;

   it->ed = edit_init(obj, it);
   it->pd = pd;

   if (efl_file_set(obj, file))
     {
        edit_term(it->ed);
        pd->main_it = NULL;
        free(it);
        return NULL;
     }

   return it;
}

EAPI Enventor_Item *
enventor_object_main_item_get(const Enventor_Object *obj)
{
//   Enventor_Object_Data *pd = efl_data_scope_get(obj, ENVENTOR_OBJECT_CLASS);
   Enventor_Object_Data *pd;
   return pd->main_it;
}

EAPI const Eina_List *
enventor_object_sub_items_get(const Enventor_Object *obj)
{
//   Enventor_Object_Data *pd = efl_data_scope_get(obj, ENVENTOR_OBJECT_CLASS);
   Enventor_Object_Data *pd;

   return pd->sub_its;
}

EAPI Enventor_Item *
enventor_object_focused_item_get(const Enventor_Object *obj)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, EINA_FALSE);

//   Enventor_Object_Data *pd = efl_data_scope_get(obj, ENVENTOR_OBJECT_CLASS);
   Enventor_Object_Data *pd;

   return  pd->focused_it;
}

///////////////////////////////////////////////////////////////////////////////
/* Enventor_Item Functions.                                                  */
///////////////////////////////////////////////////////////////////////////////
EAPI Eina_Bool
enventor_item_represent(Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   Enventor_Object_Data *pd = it->pd;

   if (pd->focused_it == it) return EINA_TRUE;

   eina_stringshare_del(pd->group_name);
   pd->group_name = NULL;

   if (pd->focused_it)
     {
        edit_view_sync_cb_set(pd->focused_it->ed, NULL, NULL);
        evas_object_hide(edit_obj_get(pd->focused_it->ed));
        edj_mgr_view_switch_to(NULL);
     }
   edit_view_sync_cb_set(it->ed, edit_view_sync_cb, it);

   pd->focused_it = it;

   //Update Editor State
   edit_linenumber_set(it->ed, pd->linenumber);
   edit_font_scale_set(it->ed, pd->font_scale);
   edit_disabled_set(it->ed, pd->disabled);

   if (evas_object_visible_get(it->pd->obj))
     evas_object_show(edit_obj_get(it->ed));

   autocomp_target_set(it->ed);

   return EINA_TRUE;
}

EAPI Evas_Object *
enventor_item_editor_get(const Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, NULL);

   if (!it->ed) return NULL;

   return edit_obj_get(it->ed);
}

EAPI const char *
enventor_item_file_get(const Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, NULL);

   if (!it->ed) return NULL;

   return edit_file_get(it->ed);
}

EAPI int
enventor_item_max_line_get(const Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, 0);

   return edit_max_line_get(it->ed);
}

EAPI Eina_Bool
enventor_item_line_goto(Enventor_Item *it, int line)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   edit_goto(it->ed, line);

   return EINA_TRUE;
}

EAPI Eina_Bool
enventor_item_syntax_color_full_apply(Enventor_Item *it, Eina_Bool force)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   edit_syntax_color_full_apply(it->ed, force);

   return EINA_TRUE;
}

EAPI Eina_Bool
enventor_item_syntax_color_partial_apply(Enventor_Item *it, double interval)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   edit_syntax_color_partial_apply(it->ed, interval);

   return EINA_TRUE;
}

EAPI Eina_Bool
enventor_item_select_region_set(Enventor_Item *it, int start, int end)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   edit_selection_clear(it->ed);
   edit_selection_region_center_set(it->ed, start, end);

   return EINA_TRUE;
}

EAPI Eina_Bool
enventor_item_select_none(Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   edit_select_none(it->ed);

   return EINA_TRUE;
}

EAPI Eina_Bool
enventor_item_cursor_pos_set(Enventor_Item *it, int position)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   edit_cursor_pos_set(it->ed, position);

   return EINA_TRUE;
}

EAPI int
enventor_item_cursor_pos_get(const Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, 0);

   return edit_cursor_pos_get(it->ed);
}

EAPI const char *
enventor_item_selection_get(const Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, NULL);

   return edit_selection_get(it->ed);
}

EAPI Eina_Bool
enventor_item_text_insert(Enventor_Item *it, const char *text)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   edit_text_insert(it->ed, text);

   return EINA_TRUE;
}

EAPI const char *
enventor_item_text_get(const Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, NULL);

   return edit_text_get(it->ed);
}

EAPI Eina_Bool
enventor_item_line_delete(Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   edit_line_delete(it->ed);

   //Close auto-completion popup if it's shown.
   autocomp_reset();

   return EINA_TRUE;
}

EAPI Eina_Bool
enventor_item_file_save(Enventor_Item *it, const char *file)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   if (!file) file = edit_file_get(it->ed);

   //Update edc file and try to save if the edc path is different.
   if (it->pd->main_it == it)
     {
        if (build_edc_path_get() != file)
          edit_changed_set(it->ed, EINA_TRUE);
     }

   Eina_Bool saved = edit_save(it->ed, file);
   if (saved) build_edc();
   return saved;
}

EAPI Eina_Bool
enventor_item_modified_get(const Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   return edit_changed_get(it->ed);
}

EAPI void
enventor_item_modified_set(Enventor_Item *it, Eina_Bool modified)
{
   EINA_SAFETY_ON_NULL_RETURN(it);

   edit_changed_set(it->ed, modified);
}

EAPI Eina_Bool
enventor_item_del(Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);
   Enventor_Object_Data *pd = it->pd;

   if (pd->focused_it == it)
     {
        edj_mgr_view_switch_to(NULL);
        autocomp_target_set(NULL);
        pd->focused_it = NULL;
     }

   edit_term(it->ed);

   //Main Item?
   if (it == pd->main_it)
     {
        pd->main_it = NULL;
        free(it);
     }
   //Sub Items
   else
     {
        pd->sub_its = eina_list_remove(pd->sub_its, it);
        free(it);
     }

   return EINA_TRUE;
}

EAPI Eina_Bool
enventor_item_template_insert(Enventor_Item *it, char *syntax, size_t n)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   return template_insert(it->ed, syntax, n);
}

EAPI Eina_Bool
enventor_item_template_part_insert(Enventor_Item *it,
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
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   Enventor_Object_Data *pd = it->pd;

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

   return template_part_insert(it->ed, part, insert_type,
                               fixed_w, fixed_h,
                               rel1_x_to, rel1_y_to,
                               rel2_x_to, rel2_y_to,
                               align_x, align_y, min_w, min_h,
                               rel1_x, rel1_y, rel2_x, rel2_y,
                               NULL, syntax, n);
}

EAPI Eina_Bool
enventor_item_redo(Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   return edit_redoundo(it->ed, EINA_FALSE);
}

EAPI Eina_Bool
enventor_item_undo(Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, EINA_FALSE);

   return edit_redoundo(it->ed, EINA_TRUE);
}

EAPI Eina_List *
enventor_item_group_list_get(Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, NULL);

   return edit_group_list_get(it->ed);
}
