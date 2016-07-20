#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Eio.h>
#include "common.h"

typedef struct file_mgr_s {
     Evas_Object *warning_layout;
     Eina_Bool edc_modified : 1;
} file_mgr_data;

static file_mgr_data *g_fmd = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
warning_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                     const char *emission EINA_UNUSED,
                     const char *source EINA_UNUSED)
{
   file_mgr_data *fmd = data;
   evas_object_del(fmd->warning_layout);
   enventor_object_focus_set(base_enventor_get(), EINA_TRUE);
   fmd->warning_layout = NULL;
}

static void
warning_close(file_mgr_data *fmd)
{
   elm_object_signal_emit(fmd->warning_layout, "elm,state,dismiss", "");
}

static void
warning_ignore_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   file_mgr_data *fmd = data;

   //FIXME: Specify which file has been changed?
   Enventor_Item *it = enventor_object_focused_item_get(base_enventor_get());
   enventor_item_modified_set(it, EINA_TRUE);

   warning_close(fmd);
}

static void
warning_save_as_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   file_mgr_data *fmd = data;

   //FIXME: Sepcify which file has been changed?
   Enventor_Item *it = enventor_object_focused_item_get(base_enventor_get());
   enventor_item_modified_set(it, EINA_TRUE);

   menu_edc_save();
   warning_close(fmd);
}

static void
warning_replace_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   file_mgr_data *fmd = data;

   //FIXME: Specify which file has been changed?
   file_mgr_main_file_set(config_input_path_get());

   warning_close(fmd);
}

static void
warning_open(file_mgr_data *fmd)
{
   if (fmd->warning_layout) return;

   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "warning_layout");
   elm_object_part_text_set(layout, "elm.text.desc",
                            _("EDC has been changed on the file system."));
   elm_object_part_text_set(layout, "elm.text.question",
                            _("Do you want to replace the contents?"));
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  warning_dismiss_done, fmd);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   Evas_Object *btn;

   //Save As Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, _("Save As"));
   evas_object_smart_callback_add(btn, "clicked", warning_save_as_btn_cb, fmd);
   elm_object_part_content_set(layout, "elm.swallow.btn1", btn);
   evas_object_show(btn);
   elm_object_focus_set(btn, EINA_TRUE);

   //Replace Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, _("Replace"));
   evas_object_smart_callback_add(btn, "clicked", warning_replace_btn_cb, fmd);
   elm_object_part_content_set(layout, "elm.swallow.btn2", btn);

   //Igrore Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, _("Ignore"));
   evas_object_smart_callback_add(btn, "clicked", warning_ignore_btn_cb, fmd);
   elm_object_part_content_set(layout, "elm.swallow.btn3", btn);

   fmd->warning_layout = layout;

   fmd->edc_modified = EINA_FALSE;
}

static void
enventor_edc_modified_cb(void *data, Evas_Object *obj EINA_UNUSED,
                         void *event_info)
{
   file_mgr_data *fmd = data;
   Enventor_EDC_Modified *modified = event_info;

   //Reset console messages.
   base_console_reset();

   if (modified->self_changed)
     {
        fmd->edc_modified = EINA_FALSE;
        return;
     }

   fmd->edc_modified = EINA_TRUE;

   /* FIXME: Here ignore edc changes, if any menu is closed, 
      then we need to open warning box. */
   if (menu_activated_get()) return;

   warning_open(fmd);
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
file_mgr_reset(void)
{
   file_mgr_data *fmd = g_fmd;
   EINA_SAFETY_ON_NULL_RETURN(fmd);

   fmd->edc_modified = EINA_FALSE;
}

int
file_mgr_edc_modified_get(void)
{
   file_mgr_data *fmd = g_fmd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(fmd, 0);

   return fmd->edc_modified;
}

void
file_mgr_edc_save(void)
{
   char buf[PATH_MAX];

   Enventor_Item *it = file_mgr_focused_item_get();
   Eina_Bool save_success = enventor_item_file_save(it, NULL);
   if (!config_stats_bar_get()) return;

   if (save_success)
     snprintf(buf, sizeof(buf), _("File saved. \"%s\""), config_input_path_get());
   else
     snprintf(buf, sizeof(buf), _("Already saved. \"%s\""),  config_input_path_get());

   stats_info_msg_update(buf);
}

Eina_Bool
file_mgr_warning_is_opened(void)
{
   file_mgr_data *fmd = g_fmd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(fmd, EINA_FALSE);

   return (fmd->warning_layout ? EINA_TRUE : EINA_FALSE);
}

void
file_mgr_warning_open(void)
{
   file_mgr_data *fmd = g_fmd;
   EINA_SAFETY_ON_NULL_RETURN(fmd);

   warning_open(fmd);
}

void
file_mgr_warning_close(void)
{
   file_mgr_data *fmd = g_fmd;
   EINA_SAFETY_ON_NULL_RETURN(fmd);

   if (fmd->warning_layout)
     warning_close(fmd);
}

void
file_mgr_init(void)
{
   file_mgr_data *fmd = calloc(1, sizeof(file_mgr_data));
   if (!fmd)
     {
        mem_fail_msg();
        return;
     }
   g_fmd = fmd;

   evas_object_smart_callback_add(base_enventor_get(), "edc,modified",
                                  enventor_edc_modified_cb, fmd);
}

void
file_mgr_term(void)
{
   file_mgr_data *fmd = g_fmd;
   EINA_SAFETY_ON_NULL_RETURN(fmd);

   free(fmd);
}

Enventor_Item *
file_mgr_sub_file_add(const char *path)
{
   Enventor_Item *it = enventor_object_sub_item_add(base_enventor_get(), path);
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, NULL);

   file_tab_it_add(it);
   file_tab_it_select(it);
   file_mgr_file_focus(it);

   return it;
}

Enventor_Item *
file_mgr_main_file_set(const char *path)
{
   Enventor_Item *it = enventor_object_main_item_set(base_enventor_get(), path);
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, NULL);

   file_tab_clear();
   file_tab_it_add(it);
   file_mgr_file_focus(it);
   base_console_reset();

   return it;
}

void
file_mgr_file_focus(Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN(it);

   file_tab_it_select(it);
   enventor_item_represent(it);
   base_title_set(enventor_item_file_get(it));

   //Reset context if the find/replace is working on.
   search_reset();
   //Cancel if the live edit mode is turned on.
   live_edit_cancel(EINA_FALSE);
}

Enventor_Item *
file_mgr_focused_item_get(void)
{
   return enventor_object_focused_item_get(base_enventor_get());
}

Eina_Bool
file_mgr_save_all(void)
{
   file_mgr_data *fmd = g_fmd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(fmd, EINA_FALSE);

   if (!fmd->edc_modified) return EINA_TRUE;

   Enventor_Item *it;
   Eina_Bool ret = EINA_TRUE;

   //Main file.
   it = file_mgr_main_item_get();
   if (!enventor_item_file_save(it, NULL)) ret = EINA_FALSE;

   //Sub files.
   Eina_List *l;
   Eina_List *sub_its =
      (Eina_List *) enventor_object_sub_items_get(base_enventor_get());
   EINA_LIST_FOREACH(sub_its, l, it)
     {
        if (!enventor_item_file_save(it, NULL)) ret = EINA_FALSE;
     }

   fmd->edc_modified = EINA_FALSE;

   return ret;
}

Enventor_Item *
file_mgr_main_item_get(void)
{
   return enventor_object_main_item_get(base_enventor_get());
}

Eina_Bool
file_mgr_modified_get(void)
{
   file_mgr_data *fmd = g_fmd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(fmd, EINA_FALSE);

   Enventor_Item *it;

   //Main file.
   it = file_mgr_main_item_get();
   if (enventor_item_modified_get(it)) return EINA_TRUE;

   //Sub files.
   Eina_List *l;
   Eina_List *sub_its =
      (Eina_List *) enventor_object_sub_items_get(base_enventor_get());
   EINA_LIST_FOREACH(sub_its, l, it)
     {
        if (enventor_item_modified_get(it)) return EINA_TRUE;
     }

   return EINA_FALSE;
}
