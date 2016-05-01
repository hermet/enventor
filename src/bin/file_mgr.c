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
   enventor_object_modified_set(base_enventor_get(), EINA_TRUE);
   warning_close(fmd);
}

static void
warning_save_as_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   file_mgr_data *fmd = data;
   enventor_object_modified_set(base_enventor_get(), EINA_TRUE);
   menu_edc_save();
   warning_close(fmd);
}

static void
warning_replace_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   file_mgr_data *fmd = data;
   enventor_object_main_file_set(base_enventor_get(), config_input_path_get());
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

void
file_mgr_reset(void)
{
   file_mgr_data *fmd = g_fmd;
   fmd->edc_modified = EINA_FALSE;
}

int
file_mgr_edc_modified_get(void)
{
   file_mgr_data *fmd = g_fmd;
   return fmd->edc_modified;
}

void
file_mgr_edc_save(void)
{
   char buf[PATH_MAX];

   Eina_Bool save_success = enventor_object_save(base_enventor_get(),
                                                 config_input_path_get());

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
   return ((fmd && fmd->warning_layout) ? EINA_TRUE : EINA_FALSE);
}

void
file_mgr_warning_open(void)
{
   file_mgr_data *fmd = g_fmd;
   warning_open(fmd);
}

void
file_mgr_warning_close(void)
{
   file_mgr_data *fmd = g_fmd;

   if (fmd->warning_layout)
     warning_close(fmd);
}

void
file_mgr_init(void)
{
   file_mgr_data *fmd = calloc(1, sizeof(file_mgr_data));
   if (!fmd)
     {
        EINA_LOG_ERR(_("Failed to allocate Memory!"));
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
   free(fmd);
}
