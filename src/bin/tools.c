#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

typedef struct tools_s
{
   Evas_Object *swallow_btn;
   Evas_Object *status_btn;
   Evas_Object *file_browser_btn;
   Evas_Object *edc_navigator_btn;
   Evas_Object *lines_btn;
   Evas_Object *template_btn;
   Evas_Object *highlight_btn;
   Evas_Object *mirror_btn;
   Evas_Object *goto_btn;
   Evas_Object *find_btn;
   Evas_Object *console_btn;
   Evas_Object *menu_btn;
   Evas_Object *live_view_ly;
   Evas_Object *text_editor_ly;
} tools_data;

static tools_data *g_td = NULL;

static void
menu_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   live_edit_cancel(EINA_FALSE);
   search_close();
   tools_goto_update();

   menu_toggle();
}

static void
highlight_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   tools_highlight_update(EINA_TRUE);
}

static void
dummy_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
         void *event_info EINA_UNUSED)
{
   tools_dummy_update(EINA_TRUE);
}

static void
mirror_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   tools_mirror_mode_update(EINA_TRUE);
}

static void
lines_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
         void *event_info EINA_UNUSED)
{
   tools_lines_update(EINA_TRUE);
}

static void
template_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   tools_template_insert();
}

static void
file_browser_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   tools_file_browser_update(EINA_TRUE);
}

static void
edc_navigator_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   tools_edc_navigator_update(EINA_TRUE);
}

static void
status_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
          void *event_info EINA_UNUSED)
{
   tools_status_update(EINA_TRUE);
}

static void
find_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   live_edit_cancel(EINA_FALSE);
   if (search_close()) return;
   else search_open();
}

static void
goto_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   live_edit_cancel(EINA_FALSE);
   if (goto_close()) return;
   else goto_open();
}

static void
console_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
   tools_data *td = g_td;
   if (!td) return;

   base_console_toggle();
}

static void
save_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   file_mgr_edc_save();
}

static void
redo_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   if (enventor_object_redo(base_enventor_get()))
     stats_info_msg_update(_("Redo text."));
   else
     stats_info_msg_update(_("No text to be redo."));
}

static void
undo_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   if (enventor_object_undo(base_enventor_get()))
     stats_info_msg_update(_("Undo text."));
   else
     stats_info_msg_update(_("No text to be undo."));
}

static Evas_Object *
tools_btn_create(Evas_Object *parent, const char *icon,
                 const char *tooltip_msg, Evas_Smart_Cb func)
{
   Evas_Object *btn = elm_button_add(parent);
   elm_object_style_set(btn, ENVENTOR_NAME);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   elm_object_tooltip_text_set(btn, tooltip_msg);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM);

   Evas_Object *img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, icon);
   elm_object_content_set(btn, img);

   evas_object_smart_callback_add(btn, "clicked", func, NULL);
   evas_object_show(btn);

   return btn;
}

void
tools_term(void)
{
   tools_data *td = g_td;
   assert(td);
   free(td);
}

void
tools_init(Evas_Object *parent)
{
   tools_data *td = g_td;
   if (td) return;

   td = calloc(1, sizeof(tools_data));
   if (!td)
     {
        EINA_LOG_ERR(_("Failed to allocate Memory!"));
        return;
     }
   g_td = td;

   //Live view tools
   Evas_Object *live_view_ly = elm_layout_add(parent);
   elm_layout_file_set(live_view_ly, EDJE_PATH, "live_view_tools_layout");
   evas_object_size_hint_weight_set(live_view_ly, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(live_view_ly, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);

   Evas_Object *btn;
   btn = tools_btn_create(live_view_ly, "highlight",
                          _("Part Highlighting (Ctrl + H)<br>"
                            "Highlight effect on the selected part in the<br>"
                            "live view."),
                          highlight_cb);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM_RIGHT);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(live_view_ly, "elm.swallow.highlight", btn);
   td->highlight_btn = btn;

   btn = tools_btn_create(live_view_ly, "dummy",
                          _("Dummy Parts (Ctrl + W)<br>"
                            "Display virtual images for the swallow and<br>"
                            "spacer parts."),
                          dummy_cb);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM_RIGHT);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(live_view_ly, "elm.swallow.dummy", btn);
   td->swallow_btn = btn;

   //icon image is temporary, it should be changed to its own icon.
   btn = tools_btn_create(live_view_ly, "mirror",
                          _("Mirror Mode (Ctrl + M)<br>"
                            "Invert layout horizontally. This previews <br>"
                            "design layout for the environemnt, RTL(Right<br>"
                            "to Left)/LTR(Left to Right) setting."),
                          mirror_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(live_view_ly, "elm.swallow.mirror", btn);
   td->mirror_btn = btn;

   //Live edit tools
   Eina_List *btn_list = live_edit_tools_create(live_view_ly);
   Eina_List *l = NULL;
   int i = 1;
   EINA_LIST_FOREACH(btn_list, l, btn)
     {
        evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);

        char swallow_part[32];
        snprintf(swallow_part, sizeof(swallow_part), "elm.swallow.live_edit%d",
                 i);
        elm_object_part_content_set(live_view_ly, swallow_part, btn);
        i++;
     }
   eina_list_free(btn_list);

   td->live_view_ly = live_view_ly;

   //Text editor tools
   Evas_Object *text_editor_ly = elm_layout_add(parent);
   elm_layout_file_set(text_editor_ly, EDJE_PATH, "text_editor_tools_layout");
   evas_object_size_hint_weight_set(text_editor_ly, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(text_editor_ly, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);

   btn = tools_btn_create(text_editor_ly, "save",
                          _("Save File (Ctrl + S)<br>"
                            "Save current script to file."),
                          save_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.save", btn);

   btn = tools_btn_create(text_editor_ly, "undo",
                          _("Undo Text (Ctrl + Z)"),
                          undo_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.undo", btn);

   btn = tools_btn_create(text_editor_ly, "redo",
                          _("Redo Text (Ctrl + R)"),
                          redo_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.redo", btn);

   btn = tools_btn_create(text_editor_ly, "find",
                          _("Find/Replace (Ctrl + F)<br>"
                            "Open Find/Replace window to find or replace "
                            "text."),
                          find_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.find", btn);
   td->find_btn = btn;

   btn = tools_btn_create(text_editor_ly, "goto",
                          _("Goto Lines (Ctrl + L)<br>"
                            "Open Goto window to move the cursor line position."),
                          goto_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.goto", btn);
   td->goto_btn = btn;

   btn = tools_btn_create(text_editor_ly, "lines",
                          _("Line Numbers (F5)<br>"
                            "Display script line number."),
                          lines_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.lines", btn);
   td->lines_btn = btn;

   btn = tools_btn_create(text_editor_ly, "template",
                          _("Insert Template Code Snippet (Ctrl + T)<br>"
                            "Enventor decides best template code with<br>"
                            "regards to the current editing context. For<br>"
                            "instance, if the cursor is inside of part <br>"
                            "section, description code will be generated."),
                          template_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.template", btn);
   td->template_btn = btn;

   btn = tools_btn_create(text_editor_ly, "console",
                          _("Console Box (Alt + Down)<br>"
                            "Show Console box which displays build logs."),
                          console_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.console", btn);
   td->console_btn = btn;

   btn = tools_btn_create(text_editor_ly, "file_browser",
                          _("File Browser (F9)<br>"
                            "Show File Browser wihch displays list of files."),
                          file_browser_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.file_browser", btn);
   td->file_browser_btn = btn;

   btn = tools_btn_create(text_editor_ly, "edc_navigator",
                          _("EDC Navigator (F10)<br>"
                            "Show EDC Navigator which displays current parts<br>"
                            "hierarchically."),
                          edc_navigator_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.edc_navigator",
                               btn);
   td->edc_navigator_btn = btn;

   btn = tools_btn_create(text_editor_ly, "status",
                          _("Status (F11)<br>"
                            "Show Status bar which displays live view informations."),
                          status_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.status", btn);
   td->status_btn = btn;

   btn = tools_btn_create(text_editor_ly, "menu",
                          _("Enventor Menu (Esc)<br>"
                          "Show Menu for setting enventor."),
                          menu_cb);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM_LEFT);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.menu", btn);
   td->menu_btn = btn;

   td->text_editor_ly = text_editor_ly;

   //Turn on if console is valid size.
   if (!config_console_get() && (config_console_size_get() > 0))
     tools_console_update(EINA_TRUE);
}

Evas_Object *
tools_live_view_get(void)
{
   tools_data *td = g_td;
   if (!td) return NULL;

   return td->live_view_ly;
}

Evas_Object *
tools_text_editor_get(void)
{
   tools_data *td = g_td;
   if (!td) return NULL;

   return td->text_editor_ly;
}

void
tools_highlight_update(Eina_Bool toggle)
{
   tools_data *td = g_td;
   if (!td) return;

   if (toggle) config_part_highlight_set(!config_part_highlight_get());
   enventor_object_part_highlight_set(base_enventor_get(),
                                      config_part_highlight_get());
   if (toggle)
     {
        if (config_part_highlight_get())
          stats_info_msg_update(_("Part Highlighting Enabled."));
        else
          stats_info_msg_update(_("Part Highlighting Disabled."));
     }

   //Toggle on/off
   if (config_part_highlight_get())
     elm_object_signal_emit(td->highlight_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->highlight_btn, "icon,highlight,disabled", "");
}

void
tools_file_browser_update(Eina_Bool toggle)
{
   tools_data *td = g_td;
   if (!td) return;

   if (toggle) config_file_browser_set(!config_file_browser_get());

   base_file_browser_toggle(EINA_FALSE);

   //Toggle on/off
   if (config_file_browser_get())
     {
        elm_object_signal_emit(td->file_browser_btn, "icon,highlight,enabled",
                               "");
     }
   else
     {
        elm_object_signal_emit(td->file_browser_btn, "icon,highlight,disabled",
                               "");
     }
}

void
tools_edc_navigator_update(Eina_Bool toggle)
{
   tools_data *td = g_td;
   if (!td) return;

   if (toggle) config_edc_navigator_set(!config_edc_navigator_get());

   base_edc_navigator_toggle(EINA_FALSE);

   //Toggle on/off
   if (config_edc_navigator_get())
     {
        elm_object_signal_emit(td->edc_navigator_btn, "icon,highlight,enabled",
                               "");
     }
   else
     {
        elm_object_signal_emit(td->edc_navigator_btn, "icon,highlight,disabled",
                               "");
     }
}

void
tools_lines_update(Eina_Bool toggle)
{
   tools_data *td = g_td;
   if (!td) return;

   if (toggle) config_linenumber_set(!config_linenumber_get());
   enventor_object_linenumber_set(base_enventor_get(), config_linenumber_get());

   //Toggle on/off
   if (config_linenumber_get())
     elm_object_signal_emit(td->lines_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->lines_btn, "icon,highlight,disabled", "");
}

void
tools_dummy_update(Eina_Bool toggle)
{
   tools_data *td = g_td;
   if (!td) return;

   if (toggle) config_dummy_parts_set(!config_dummy_parts_get());
   enventor_object_dummy_parts_set(base_enventor_get(),
                                   config_dummy_parts_get());

   if (toggle)
     {
        if (config_dummy_parts_get())
          stats_info_msg_update(_("Dummy Parts Enabled."));
        else
          stats_info_msg_update(_("Dummy Parts Disabled."));
     }
   //Toggle on/off
   if (config_dummy_parts_get())
     elm_object_signal_emit(td->swallow_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->swallow_btn, "icon,highlight,disabled", "");
}

void
tools_mirror_mode_update(Eina_Bool toggle)
{
   tools_data *td = g_td;
   if (!td) return;

   if (toggle) config_mirror_mode_set(!config_mirror_mode_get());
   enventor_object_mirror_mode_set(base_enventor_get(),
                                   config_mirror_mode_get());
   live_edit_update();

   if (toggle)
     {
        if (config_mirror_mode_get())
          stats_info_msg_update(_("Mirror Mode Enabled."));
        else
          stats_info_msg_update(_("Mirror Mode Disabled."));
     }

   //Toggle on/off
   if (config_mirror_mode_get())
     elm_object_signal_emit(td->mirror_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->mirror_btn, "icon,highlight,disabled", "");
}

void
tools_status_update(Eina_Bool toggle)
{
   tools_data *td = g_td;
   if (!td) return;

   base_statusbar_toggle(toggle);

   //Toggle on/off
   if (config_stats_bar_get())
     elm_object_signal_emit(td->status_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->status_btn, "icon,highlight,disabled", "");
}

void
tools_goto_update(void)
{
   tools_data *td = g_td;
   if (!td) return;

   if (goto_is_opened())
     elm_object_signal_emit(td->goto_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->goto_btn, "icon,highlight,disabled", "");
}

void
tools_search_update(void)
{
   tools_data *td = g_td;
   if (!td) return;

   if (search_is_opened())
     elm_object_signal_emit(td->find_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->find_btn, "icon,highlight,disabled", "");
}

void
tools_template_insert(void)
{
   if (live_edit_get())
     {
        stats_info_msg_update(_("Insertion of template code is disabled while in Live Edit mode"));
        return;
     }

   char syntax[12];
   if (enventor_object_template_insert(base_enventor_get(), syntax,
                                       sizeof(syntax)))
     {
        char msg[64];
        snprintf(msg, sizeof(msg), _("Template code inserted, (%s)"), syntax);
        stats_info_msg_update(msg);
        enventor_object_save(base_enventor_get(), config_input_path_get());
     }
   else
     {
        stats_info_msg_update(_("Can't insert template code here. Move the "
                              "cursor inside the \"Collections,Images,Parts,"
                              "Part,Programs\" scope."));
     }
}

void
tools_console_update(Eina_Bool on)
{
   tools_data *td = g_td;
   if (!td) return;

   if (on)
     elm_object_signal_emit(td->console_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->console_btn, "icon,highlight,disabled", "");
}

void
tools_menu_update(Eina_Bool on)
{
   tools_data *td = g_td;
   if (!td) return;

   if (on)
     elm_object_signal_emit(td->menu_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->menu_btn, "icon,highlight,disabled", "");
}
