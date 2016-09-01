#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

struct menu_s
{
   Evas_Object *menu_layout;
   Evas_Object *newfile_layout;
   Evas_Object *warning_layout;
   Evas_Object *fileselector_layout;

   const char *last_accessed_path;

   int active_request;

   Eina_Bool template_new : 1;
};

typedef struct menu_s menu_data;

static menu_data *g_md = NULL;

static void warning_no_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                              void *event_info EINA_UNUSED);
static void new_save_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED);
static void edc_file_save(menu_data *md);
static void new_yes_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                           void *event_info EINA_UNUSED);

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
fileselector_close(menu_data *md)
{
   elm_object_signal_emit(md->fileselector_layout, "elm,state,dismiss", "");
}

static void
newfile_close(menu_data *md)
{
   elm_object_signal_emit(md->newfile_layout, "elm,state,dismiss", "");
}

static void
warning_close(menu_data *md)
{
   elm_object_signal_emit(md->warning_layout, "elm,state,dismiss", "");
}

static void
menu_close(menu_data *md)
{
   if (!md->menu_layout) return;
   elm_object_signal_emit(md->menu_layout, "elm,state,dismiss", "");

   tools_menu_update(EINA_FALSE);
}

static void
newfile_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                     const char *emission EINA_UNUSED,
                     const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->newfile_layout);
   md->newfile_layout = NULL;
   menu_deactivate_request();
}

static void
fileselector_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                          const char *emission EINA_UNUSED,
                          const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->fileselector_layout);
   md->fileselector_layout = NULL;
   menu_deactivate_request();
}

static void
menu_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->menu_layout);
   md->menu_layout = NULL;
   menu_deactivate_request();
}

static void
warning_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                     const char *emission EINA_UNUSED,
                     const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->warning_layout);
   md->warning_layout = NULL;
   menu_deactivate_request();
}

static void
newfile_ok_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   newfile_set(md->template_new);
   newfile_close(md);
   menu_close(md);
}

static void
newfile_cancel_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   newfile_close(md);
}

static void
menu_back_btn_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                         void *envent_info EINA_UNUSED)
{
   menu_data *md = data;
   menu_close(md);
}

void
newfile_open(menu_data *md)
{
   if (md->newfile_layout) return;

   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "newfile_layout");
   elm_object_part_text_set(layout, "elm.text.title", _("New File: Choose a Template"));
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  newfile_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   elm_object_part_content_set(layout, "elm.swallow.content",
                               newfile_create(layout, newfile_ok_btn_cb, md));
   Evas_Object *btn;

   //Ok Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, _("Ok"));
   evas_object_smart_callback_add(btn, "clicked", newfile_ok_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.ok_btn", btn);

   //Cancel Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, _("Cancel"));
   evas_object_smart_callback_add(btn, "clicked", newfile_cancel_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.cancel_btn", btn);

   md->newfile_layout = layout;
   menu_activate_request();
}

static void
warning_open(menu_data *md, Evas_Smart_Cb yes_cb, Evas_Smart_Cb save_cb)
{
   if (md->warning_layout) return;

   stats_ctxpopup_dismiss();

   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "warning_layout");
   elm_object_part_text_set(layout, "elm.text.desc",
                            _("You have unsaved changes."));
   elm_object_part_text_set(layout, "elm.text.question",
                            _("Do you want to save the changes?"));
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  warning_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   Evas_Object *btn;

   //Save Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, _("Save"));
   evas_object_smart_callback_add(btn, "clicked", save_cb, md);
   evas_object_show(btn);
   elm_object_focus_set(btn, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.btn1", btn);

   //Discard Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, _("Discard"));
   evas_object_smart_callback_add(btn, "clicked", yes_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.btn2", btn);

   //Cancel Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, _("Cancel"));
   evas_object_smart_callback_add(btn, "clicked", warning_no_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.btn3", btn);

   md->warning_layout = layout;
   menu_activate_request();
}

static void
help_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   help_open();
}

static void
setting_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   setting_open();
}

static void
new_yes_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   newfile_open(md);
   warning_close(md);
   menu_close(md);
}

static void
exit_yes_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   elm_exit();
}

static void
exit_save_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   file_mgr_save_all();
   elm_exit();
}

static void
exit_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_exit();
}

static Evas_Object *
btn_create(Evas_Object *parent, const char *label, Evas_Smart_Cb cb, void *data)
{
   Evas_Object *btn;

   btn  = elm_button_add(parent);
   elm_object_style_set(btn, "anchor");
   elm_object_scale_set(btn, 1.25);
   evas_object_smart_callback_add(btn, "clicked", cb, data);
   elm_object_text_set(btn, label);
   evas_object_show(btn);

   return btn;
}

static void
warning_no_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   warning_close(md);
}

static void
new_save_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   file_mgr_save_all();
   newfile_open(md);
   warning_close(md);
   menu_close(md);
}

static void
new_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
   menu_edc_new(EINA_FALSE);
}

static void
save_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   edc_file_save(md);
}

static void
fileselector_save_done_cb(void *data, Evas_Object *obj, void *event_info)
{
   menu_data *md = data;
   const char *selected = event_info;
   Eina_Bool is_edc = EINA_FALSE;
   Eina_Bool is_edj = EINA_FALSE;

   eina_stringshare_refplace(&(md->last_accessed_path),
                            elm_fileselector_path_get(obj));

   if (!selected)
     {
        fileselector_close(md);
        return;
     }
   else if (ecore_file_is_dir(selected))
     {
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg", _("Choose a file to save"));
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   //Filter to read only edc or edj extensions file.
   is_edc = eina_str_has_extension(selected, "edc");
   is_edj = eina_str_has_extension(selected, "edj");
   if (!is_edc && !is_edj)
     {
        selected = eina_stringshare_printf("%s.edc", selected);
        is_edc = EINA_TRUE;
     }

   Enventor_Object *enventor = base_enventor_get();
   Enventor_Item *it = file_mgr_focused_item_get();

   if (is_edc)
     {
        Eina_Bool main_file;
        if (file_mgr_main_item_get() == it) main_file = EINA_TRUE;
        else main_file = EINA_TRUE;

        //Update config path if main file.
        if (main_file)
          {
             config_input_path_set(selected);
             Eina_List *list = eina_list_append(NULL, config_output_path_get());
             enventor_object_path_set(enventor, ENVENTOR_PATH_TYPE_EDJ,
                                      list);
             eina_list_free(list);
          }

        if (!enventor_item_file_save(it, selected))
          {
             char buf[PATH_MAX];
             snprintf(buf, sizeof(buf), _("Failed to save: %s."), selected);
             elm_object_part_text_set(md->fileselector_layout,
                                      "elm.text.msg", buf);
             elm_object_signal_emit(md->fileselector_layout,
                                    "elm,action,msg,show", "");
             eina_stringshare_del(selected);
             return;
          }

        if (main_file) file_mgr_main_file_set(selected);
     }
   else if (is_edj)
     {
        Eina_List *edj_pathes = NULL;
        edj_pathes = eina_list_append(edj_pathes, selected);
        enventor_object_path_set(enventor, ENVENTOR_PATH_TYPE_EDJ,
                                 edj_pathes);
        enventor_item_modified_set(it, EINA_TRUE);
        enventor_item_file_save(it, NULL);
        eina_list_free(edj_pathes);
     }

   file_mgr_reset();
   fileselector_close(md);
   menu_close(md);
}

static void
fileselector_save_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   fileselector_save_done_cb(data, obj, event_info);
}

static void
fileselector_load_done_cb(void *data, Evas_Object *obj, void *event_info)
{
   menu_data *md = data;
   const char *selected = event_info;

   eina_stringshare_refplace(&(md->last_accessed_path),
                             elm_fileselector_path_get(obj));

   if (!selected)
     {
        fileselector_close(md);
        return;
     }

   //Filter to read only edc extension file.
   char *ext = strrchr(selected, '.');
   if (!ext || strcmp(ext, ".edc"))
     {
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg",
                                 _("Support only .edc file."));
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   //Directory?
   if (ecore_file_is_dir(selected))
     {
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg", _("Choose a file to load."));
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   //Show a message if it failed to load the file.
   if (!ecore_file_can_read(selected))
     {
        char buf[PATH_MAX];
        const char *filename = ecore_file_file_get(selected);
        snprintf(buf, sizeof(buf), _("Failed to load: %s."), filename);
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg", buf);
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }
   config_input_path_set(selected);
   file_mgr_main_file_set(selected);
   fileselector_close(md);
   menu_close(md);
   file_mgr_reset();
}

static void
fileselector_load_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   fileselector_load_done_cb(data, obj, event_info);
}

static void
edc_file_save(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "fileselector_layout");
   elm_object_part_text_set(layout, "elm.text.title",
                            _("Save File: Choose a EDC"));
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  fileselector_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   Evas_Object *fs = elm_fileselector_add(layout);
   elm_object_part_text_set(fs, "ok", _("Save"));
   elm_object_part_text_set(fs, "cancel", _("Close"));
   elm_fileselector_path_set(fs, md->last_accessed_path ? md->last_accessed_path : eina_environment_home_get());
   elm_fileselector_expandable_set(fs, EINA_FALSE);
   elm_fileselector_is_save_set(fs, EINA_TRUE);
   evas_object_smart_callback_add(fs, "done", fileselector_save_done_cb, md);
   evas_object_smart_callback_add(fs, "selected", fileselector_save_selected_cb,
                                  md);
   evas_object_show(fs);

   elm_object_part_content_set(layout, "elm.swallow.fileselector", fs);

   menu_activate_request();

   elm_object_focus_set(fs, EINA_TRUE);

   md->fileselector_layout = layout;
}

static void
edc_file_load(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "fileselector_layout");
   elm_object_part_text_set(layout, "elm.text.title",
                            _("Load File: Choose a EDC"));
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  fileselector_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   Evas_Object *fs = elm_fileselector_add(layout);
   elm_fileselector_path_set(fs, md->last_accessed_path ? md->last_accessed_path : eina_environment_home_get());
   elm_object_part_text_set(fs, "ok", _("Load"));
   elm_object_part_text_set(fs, "cancel", _("Close"));
   elm_fileselector_expandable_set(fs, EINA_FALSE);
   elm_fileselector_is_save_set(fs, EINA_TRUE);
   evas_object_smart_callback_add(fs, "done", fileselector_load_done_cb, md);
   evas_object_smart_callback_add(fs, "selected", fileselector_load_selected_cb,
                                  md);
   evas_object_show(fs);

   elm_object_part_content_set(layout, "elm.swallow.fileselector", fs);

   menu_activate_request();

   elm_object_focus_set(fs, EINA_TRUE);

   md->fileselector_layout = layout;
}

static void
load_yes_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   edc_file_load(md);
   warning_close(md);
}

static void
load_save_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   file_mgr_save_all();
   edc_file_load(md);
   warning_close(md);
}

static void
load_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_edc_load();
}

static void
menu_open(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "menu_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  menu_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   //Back Button
   Evas_Object *back_button = elm_button_add(layout);
   elm_object_style_set(back_button, ENVENTOR_NAME);
   elm_object_scale_set(back_button, 1.15);
   elm_object_focus_allow_set(back_button, EINA_FALSE);
   evas_object_show(back_button);
   evas_object_smart_callback_add(back_button, "clicked",
                                  menu_back_btn_clicked_cb, md);

   //Back Button Icon
   Evas_Object *back_img = elm_image_add(back_button);
   elm_image_file_set(back_img, EDJE_PATH, "close");
   elm_object_content_set(back_button, back_img);

   elm_object_part_content_set(layout, "elm.swallow.back_button", back_button);

   //Button
   Evas_Object *btn;

   //Button(New)
   btn = btn_create(layout, _("New"), new_btn_cb, md);
   elm_object_focus_set(btn, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.new_btn", btn);

   //Button(Save)
   btn = btn_create(layout, _("Save"), save_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.save_btn", btn);

   //Button(Load)
   btn = btn_create(layout, _("Load"), load_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.load_btn", btn);

   //Button(Setting)
   btn = btn_create(layout, _("Settings"), setting_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.setting_btn", btn);

   //Button(Help)
   btn = btn_create(layout, _("Help"), help_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.help_btn", btn);

   //Button(Exit)
   btn = btn_create(layout, _("Exit"), exit_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.exit_btn", btn);

   tools_menu_update(EINA_TRUE);

   md->menu_layout = layout;
   md->active_request++;
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
menu_init(void)
{
   menu_data *md = calloc(1, sizeof(menu_data));
   if (!md)
     {
        mem_fail_msg();
        return;
     }
   g_md = md;
}

void
menu_term(void)
{
   menu_data *md = g_md;
   free(md);
}

void
menu_setting(void)
{
   setting_open();
}

void
menu_edc_new(Eina_Bool template_new)
{
   menu_data *md = g_md;
   md->template_new = template_new;
   if (file_mgr_edc_modified_get())
     warning_open(md, new_yes_btn_cb, new_save_btn_cb);
   else
     newfile_open(md);
}

void
menu_edc_save(void)
{
   menu_data *md = g_md;
   EINA_SAFETY_ON_NULL_RETURN(md);
   edc_file_save(md);
}

void
menu_edc_load(void)
{
   menu_data *md = g_md;
   EINA_SAFETY_ON_NULL_RETURN(md);

   if (file_mgr_modified_get())
     warning_open(md, load_yes_btn_cb, load_save_btn_cb);
   else
     edc_file_load(md);
}

void
menu_toggle(void)
{
   menu_data *md = g_md;
   EINA_SAFETY_ON_NULL_RETURN(md);

   if (setting_is_opened())
     {
        setting_close();
        return;
     }
   if (md->warning_layout)
     {
        warning_close(md);
        return;
     }
   if (md->newfile_layout)
     {
        newfile_close(md);
        return;
     }
   if (md->fileselector_layout)
     {
        fileselector_close(md);
        return;
     }

   //Main Menu
   if (md->active_request) menu_close(md);
   else
     {
        menu_open(md);
     }
}

int
menu_activated_get(void)
{
   menu_data *md = g_md;
   if (!md) return 0;
   return md->active_request;
}

static void
enventor_ctxpopup_dismissed_cb(void *data, Evas_Object *obj EINA_UNUSED,
                               void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   EINA_SAFETY_ON_NULL_RETURN(md);

   warning_open(md, exit_yes_btn_cb, exit_save_btn_cb);
}

void
menu_exit(void)
{
   //Quit Enventor Application.

   menu_data *md = g_md;
   EINA_SAFETY_ON_NULL_RETURN(md);

   if (file_mgr_edc_modified_get())
     {
        Enventor_Object *enventor = base_enventor_get();

        search_close();
        if (enventor_object_ctxpopup_visible_get(enventor))
          {
             /* bacause the main.c handles the ctxpopup dismissed callback,
                here we needs to override the function */
             evas_object_smart_callback_priority_add(enventor,
                                                 "ctxpopup,dismissed", 100,
                                                 enventor_ctxpopup_dismissed_cb,
                                                 md);
             enventor_object_ctxpopup_dismiss(enventor);
          }
        else
          warning_open(md, exit_yes_btn_cb, exit_save_btn_cb);
     }
   else
     elm_exit();
}

void
menu_deactivate_request(void)
{
   menu_data *md = g_md;
   EINA_SAFETY_ON_NULL_RETURN(md);

   md->active_request--;

   if (md->active_request == 0)
     {
        enventor_object_focus_set(base_enventor_get(), EINA_TRUE);
     }
   if (!md->menu_layout) return;
   elm_object_disabled_set(md->menu_layout, EINA_FALSE);
   elm_object_focus_set(md->menu_layout, EINA_TRUE);
}

void
menu_activate_request(void)
{
   menu_data *md = g_md;
   EINA_SAFETY_ON_NULL_RETURN(md);

   if (md->menu_layout) elm_object_disabled_set(md->menu_layout, EINA_TRUE);
   md->active_request++;
}
