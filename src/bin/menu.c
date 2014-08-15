#include <Elementary.h>
#include "common.h"

struct menu_s
{
   Evas_Object *menu_layout;
   Evas_Object *newfile_layout;
   Evas_Object *warning_layout;
   Evas_Object *fileselector_layout;
   Evas_Object *about_layout;

   Evas_Object *ctxpopup;

   const char *last_accessed_path;

   int active_request;

   edit_data *ed;
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
about_close(menu_data *md)
{
   elm_object_signal_emit(md->about_layout, "elm,state,dismiss", "");
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
about_dismiss_done(void *data, Evas_Object *obj EINA_UNUSED,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   menu_data *md = data;
   evas_object_del(md->about_layout);
   md->about_layout = NULL;
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
   newfile_set(md->ed);
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

void
newfile_open(menu_data *md)
{
   if (md->newfile_layout) return;

   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "newfile_layout");
   elm_object_part_text_set(layout, "elm.text.title", "New File: Choose a template");
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
   elm_object_text_set(btn, "Ok");
   evas_object_smart_callback_add(btn, "clicked", newfile_ok_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.ok_btn", btn);

   //Cancel Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "Cancel");
   evas_object_smart_callback_add(btn, "clicked", newfile_cancel_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.cancel_btn", btn);

   md->newfile_layout = layout;
   menu_activate_request();
}

static void
warning_open(menu_data *md, Evas_Smart_Cb yes_cb, Evas_Smart_Cb save_cb)
{
   if (md->warning_layout) return;

   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "warning_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  warning_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   Evas_Object *btn;

   //Save Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "Save");
   evas_object_smart_callback_add(btn, "clicked", save_cb, md);
   evas_object_show(btn);
   elm_object_focus_set(btn, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.save", btn);

   //No Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "No");
   evas_object_smart_callback_add(btn, "clicked", warning_no_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.no", btn);

   //Yes Button
   btn = elm_button_add(layout);
   elm_object_text_set(btn, "Yes");
   evas_object_smart_callback_add(btn, "clicked", yes_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.yes", btn);

   md->warning_layout = layout;
   menu_activate_request();
}

static void
about_open(menu_data *md)
{
   //Layout
   Evas_Object *layout = elm_layout_add(base_win_get());
   elm_layout_file_set(layout, EDJE_PATH, "about_layout");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  about_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   //Entry
   Evas_Object *entry = elm_entry_add(layout);
   elm_object_style_set(entry, "about");
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_line_wrap_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_FALSE);
   evas_object_show(entry);
   elm_object_focus_set(entry, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.entry", entry);
   elm_entry_entry_append(entry, "<color=#ffffff>");

   //Read README
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/docs/README", elm_app_data_dir_get());

   Eina_File *file = eina_file_open(buf, EINA_FALSE);
   if (!file) goto err;

   Eina_Iterator *itr = eina_file_map_lines(file);
   if (!itr) goto err;

   Eina_Strbuf *strbuf = eina_strbuf_new();
   if (!strbuf) goto err;

   Eina_File_Line *line;
   int line_num = 0;

   EINA_ITERATOR_FOREACH(itr, line)
     {
        //Append edc ccde
        if (line_num > 0)
          {
             if (!eina_strbuf_append(strbuf, EOL)) goto err;
          }

        if (!eina_strbuf_append_length(strbuf, line->start, line->length))
          goto err;
        line_num++;
     }
   elm_entry_entry_append(entry, eina_strbuf_string_get(strbuf));
   elm_entry_entry_append(entry, "</font_size></color>");

   md->about_layout = layout;
   menu_activate_request();

err:
   if (strbuf) eina_strbuf_free(strbuf);
   if (itr) eina_iterator_free(itr);
   if (file) eina_file_close(file);
}

static void
about_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   about_open(md);
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
exit_save_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   edit_save(md->ed);
   elm_exit();
}

static void
exit_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_exit();
}

static void
prev_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   edit_focus_set(md->ed);
   menu_toggle();
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

   edit_save(md->ed);
   newfile_open(md);
   warning_close(md);
   menu_close(md);
}

static void
new_btn_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
   menu_edc_new();
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
                                 "Support only .edc file.");
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   //Directory?
   if (ecore_file_is_dir(selected))
     {
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg", "Choose a file to save");
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   //Update the edc file and try to save.
   if (strcmp(config_edc_path_get(), selected))
     edit_changed_set(md->ed, EINA_TRUE);

   config_edc_path_set(selected);

   if (!edit_save(md->ed))
     {
        char buf[PATH_MAX];
        snprintf(buf, sizeof(buf), "Failed to load: %s.", selected);
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg", buf);
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   edj_mgr_reload_need_set(EINA_TRUE);
   config_apply();

   base_title_set(selected);
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
                                 "Support only .edc file.");
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   //Directory?
   if (ecore_file_is_dir(selected))
     {
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg", "Choose a file to load.");
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }

   //Show a message if it failed to load the file.
   if (!ecore_file_can_read(selected))
     {
        char buf[PATH_MAX];
        const char *filename = ecore_file_file_get(selected);
        snprintf(buf, sizeof(buf), "Failed to load: %s.", filename);
        elm_object_part_text_set(md->fileselector_layout,
                                 "elm.text.msg", buf);
        elm_object_signal_emit(md->fileselector_layout,
                               "elm,action,msg,show", "");
        return;
     }
   edit_edc_reload(md->ed, selected);
   fileselector_close(md);
   menu_close(md);
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
   elm_object_part_text_set(layout, "elm.text.title", "Save");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  fileselector_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   Evas_Object *fs = elm_fileselector_add(layout);
   elm_object_part_text_set(fs, "ok", "Save");
   elm_object_part_text_set(fs, "cancel", "Close");
   elm_fileselector_path_set(fs, md->last_accessed_path ? md->last_accessed_path : getenv("HOME"));
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
   elm_object_part_text_set(layout, "elm.text.title", "Load");
   elm_object_signal_callback_add(layout, "elm,state,dismiss,done", "",
                                  fileselector_dismiss_done, md);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   base_win_resize_object_add(layout);

   Evas_Object *fs = elm_fileselector_add(layout);
   elm_fileselector_path_set(fs, md->last_accessed_path ? md->last_accessed_path : getenv("HOME"));
   elm_object_part_text_set(fs, "ok", "Load");
   elm_object_part_text_set(fs, "cancel", "Close");
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
   edit_save(md->ed);
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

   //Button
   Evas_Object *btn;

   //Button(New)
   btn = btn_create(layout, "New", new_btn_cb, md);
   elm_object_focus_set(btn, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.new_btn", btn);

   //Button(Save)
   btn = btn_create(layout, "Save", save_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.save_btn", btn);

   //Button(Load)
   btn = btn_create(layout, "Load", load_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.load_btn", btn);

   //Button(Setting)
   btn = btn_create(layout, "Setting", setting_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.setting_btn", btn);

   //Button(About)
   btn = btn_create(layout, "About", about_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.about_btn", btn);

   //Button(Exit)
   btn = btn_create(layout, "Exit", exit_btn_cb, md);
   elm_object_part_content_set(layout, "elm.swallow.exit_btn", btn);

   //Button(Prev)
   btn = elm_button_add(layout);
   elm_object_style_set(btn, "anchor");
   evas_object_smart_callback_add(btn, "clicked", prev_btn_cb, md);
   elm_object_text_set(btn, "Back");
   elm_object_part_content_set(layout, "elm.swallow.prev_btn", btn);

   md->menu_layout = layout;
   md->active_request++;
}

static void
ctxpopup_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   menu_data *md = data;
   md->ctxpopup = NULL;
}

void
menu_init(edit_data *ed)
{
   menu_data *md = calloc(1, sizeof(menu_data));
   if (!md)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }
   g_md = md;

   md->ed = ed;
}

void
menu_term(void)
{
   menu_data *md = g_md;
   free(md);
}

void
menu_about(void)
{
   menu_data *md = g_md;
   about_open(md);
}

void
menu_setting(void)
{
   setting_open();
}

void
menu_edc_new(void)
{
   menu_data *md = g_md;
   if (edit_changed_get(md->ed))
     warning_open(md, new_yes_btn_cb, new_save_btn_cb);
   else
     newfile_open(md);
}

void
menu_edc_save(void)
{
   menu_data *md = g_md;
   edc_file_save(md);
}

void
menu_edc_load(void)
{
   menu_data *md = g_md;
   if (edit_changed_get(md->ed))
     warning_open(md, load_yes_btn_cb, load_save_btn_cb);
   else
     edc_file_load(md);
}

void
menu_toggle(void)
{
   menu_data *md = g_md;

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
   if (md->about_layout)
     {
        about_close(md);
        return;
     }
   if (md->ctxpopup)
     {
        elm_ctxpopup_dismiss(md->ctxpopup);
        return;
     }

   //Main Menu 
   if (md->active_request) menu_close(md);
   else
     {
        menu_open(md);
     }
}

void
menu_ctxpopup_unregister(Evas_Object *ctxpopup)
{
   menu_data *md = g_md;
   evas_object_event_callback_del(ctxpopup, EVAS_CALLBACK_DEL, ctxpopup_del_cb);
   if (ctxpopup == md->ctxpopup) md->ctxpopup = NULL;
}

void
menu_ctxpopup_register(Evas_Object *ctxpopup)
{
   menu_data *md = g_md;
   md->ctxpopup = ctxpopup;
   if (!ctxpopup) return;
   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL, ctxpopup_del_cb,
                                  md);
}

int
menu_activated_get(void)
{
   menu_data *md = g_md;
   if (!md) return 0;
   return md->active_request;
}

void
menu_exit(void)
{
   menu_data *md = g_md;
   if (edit_changed_get(md->ed))
     {
        search_close();
        warning_open(md, exit_yes_btn_cb, exit_save_btn_cb);
     }
   else
     elm_exit();
}

void
menu_deactivate_request(void)
{
   menu_data *md = g_md;
   md->active_request--;
   if (md->active_request == 0) edit_focus_set(md->ed);
   if (!md->menu_layout) return;
   elm_object_disabled_set(md->menu_layout, EINA_FALSE);
   elm_object_focus_set(md->menu_layout, EINA_TRUE);
}

void
menu_activate_request(void)
{
   menu_data *md = g_md;
   if (md->menu_layout) elm_object_disabled_set(md->menu_layout, EINA_TRUE);
   md->active_request++;
}
