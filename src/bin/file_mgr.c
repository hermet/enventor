#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Eio.h>
#include "common.h"

#define FILE_QUEUE_CNT 20

typedef struct file_mgr_s {
     Eina_List *file_queue;
     Evas_Object *warning_layout;
     Enventor_Item *focused_it;
     Eina_Bool edc_modified : 1;
     Eina_Bool no_queue : 1;  //file queueing?
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

static void
file_mgr_file_push(file_mgr_data *fmd, const char *file)
{
   if (!file) return;
   if (fmd->no_queue) return;

   //Prevent overflow. Remove first node.
   if (eina_list_count(fmd->file_queue) >= FILE_QUEUE_CNT)
     fmd->file_queue = eina_list_remove_list(fmd->file_queue, fmd->file_queue);

   //Append new file.
   Eina_Stringshare *tmp = eina_stringshare_add(file);
   fmd->file_queue = eina_list_append(fmd->file_queue, tmp);
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
   if (!fmd) return;

   //Remove file queue
   Eina_Stringshare *file;
   EINA_LIST_FREE(fmd->file_queue, file)
     eina_stringshare_del(file);

   free(fmd);
}

void
file_mgr_file_del(Enventor_Item *it)
{
   if (!it) return;

   file_mgr_data *fmd = g_fmd;

   if (fmd->focused_it == it)
     fmd->focused_it = NULL;

   file_tab_it_remove(it);
   enventor_item_del(it);
}

Enventor_Item *
file_mgr_sub_file_add(const char *path, Eina_Bool focus)
{
   Enventor_Item *it = enventor_object_sub_item_add(base_enventor_get(), path);
   EINA_SAFETY_ON_NULL_RETURN_VAL(it, NULL);

   file_tab_it_add(it);
   if (focus) file_mgr_file_focus(it);

   return it;
}

Enventor_Item *
file_mgr_main_file_set(const char *path)
{
   file_mgr_data *fmd = g_fmd;
   if (!fmd) return NULL;

   if (!path)
     {
        EINA_LOG_ERR("No path??");
        return NULL;
     }

   char *realpath;

   if (ecore_file_exists(path))
     realpath = ecore_file_realpath(path);
   else
     {
        //This main file is not created yet.
        FILE *fp = fopen(path, "w");
        if (fp) fclose(fp);
        realpath = ecore_file_realpath(path);
     }

   //If this file is already openend with sub file, remove it.
   Eina_List *sub_its =
      (Eina_List *) enventor_object_sub_items_get(base_enventor_get());
   Eina_List *l;
   Enventor_Item *it;
   Eina_Bool replace_focus = EINA_FALSE;

   EINA_LIST_FOREACH(sub_its, l, it)
     {
        const char *path2 = enventor_item_file_get(it);
        if (!path2) continue;
        if (strcmp(realpath, path2)) continue;
        file_mgr_file_del(it);
        if (fmd->focused_it == it)
          replace_focus = EINA_TRUE;
        break;
     }

   //Replace the current main file to a sub file.
   Enventor_Item *main_it = file_mgr_main_item_get();
   if (main_it)
     {
        const char *prev_path = enventor_item_file_get(main_it);
        if (prev_path)
          {
             const char *file_path = NULL;
             file_path = enventor_item_file_get(main_it);
             Enventor_Item *it2 = file_mgr_sub_file_add(file_path, EINA_FALSE);
             file_mgr_file_del(main_it);
             if (fmd->focused_it == main_it)
               fmd->focused_it = it2;
          }
     }

   main_it = enventor_object_main_item_set(base_enventor_get(), realpath);
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_it, NULL);

   if (replace_focus)
     fmd->focused_it = main_it;

   file_tab_it_add(main_it);
   file_mgr_file_focus(main_it);
   base_console_reset();

   free(realpath);

   return main_it;
}

void
file_mgr_file_focus(Enventor_Item *it)
{
   EINA_SAFETY_ON_NULL_RETURN(it);

   file_mgr_data *fmd = g_fmd;

   if (fmd->focused_it && (fmd->focused_it != it))
     file_mgr_file_push(fmd, enventor_item_file_get(fmd->focused_it));

   file_tab_it_select(it);
   enventor_item_represent(it);
   base_title_set(enventor_item_file_get(it));
   base_edc_navigator_group_update();
   fmd->focused_it = it;

   //Reset file based contexts.
   search_reset();
   goto_close();
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

Eina_Bool
file_mgr_file_open(const char *file_path)
{
   if (!file_path) return EINA_FALSE;

   //skip non edc nor header files.
   if (!(eina_str_has_extension(file_path, "edc") ||
         eina_str_has_extension(file_path, "h"))) return EINA_FALSE;

   Enventor_Item *eit;
   const char *it_file_path;

   //Case 1. main file.
   eit = file_mgr_main_item_get();
   if (eit)
     {
        it_file_path = enventor_item_file_get(eit);
        if (!it_file_path)
          {
             EINA_LOG_ERR("No main item file path??");
             return EINA_FALSE;
          }
        //Ok, This selected file is already openend, let's activate the item.
        if (!strcmp(file_path, it_file_path))
          {
             file_mgr_file_focus(eit);
             return EINA_TRUE;
          }
     }

   //Case 2. sub files.
   Eina_List *sub_items =
      (Eina_List *)enventor_object_sub_items_get(base_enventor_get());
   Eina_List *l;
   EINA_LIST_FOREACH(sub_items, l, eit)
     {
        it_file_path = enventor_item_file_get(eit);
        if (!it_file_path) continue;

        //Ok, This selected file is already openend, let's activate the item.
        if (!strcmp(file_path, it_file_path))
          {
             file_mgr_file_focus(eit);
             return EINA_TRUE;
          }
     }

   //This selected file hasn't been opened yet, so let's open this file newly.
   file_mgr_sub_file_add(file_path, EINA_TRUE);
   return EINA_TRUE;
}

Eina_Bool
file_mgr_file_backward(void)
{
   file_mgr_data *fmd = g_fmd;
  if (!fmd) return EINA_FALSE;

   Eina_List *last = eina_list_last(fmd->file_queue);
   if (!last) return EINA_FALSE;

   Eina_Stringshare *file = eina_list_data_get(last);
   if (!file)
     {
        EINA_LOG_ERR("No file path??");
        return EINA_FALSE;
     }

   fmd->file_queue = eina_list_remove_list(fmd->file_queue, last);

   Eina_Bool ret;

   fmd->no_queue = EINA_TRUE;
   ret = file_mgr_file_open(file);
   fmd->no_queue = EINA_FALSE;

   eina_stringshare_del(file);

   return ret;
}
