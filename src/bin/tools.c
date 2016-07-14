#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

typedef struct tools_s
{
   Evas_Object *swallow_btn;
   Evas_Object *wireframes_btn;
   Evas_Object *file_tab;
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
wireframes_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   tools_wireframes_update(EINA_TRUE);
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
file_tab_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
   tools_file_tab_update(EINA_TRUE);
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
   EINA_SAFETY_ON_NULL_RETURN(td);

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
   if (enventor_item_redo(file_mgr_focused_item_get()))
     stats_info_msg_update(_("Redo text."));
   else
     stats_info_msg_update(_("No text to be redo."));
}

static void
undo_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   if (enventor_item_undo(file_mgr_focused_item_get()))
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
        mem_fail_msg();
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
                          _("Part highlighting (Ctrl + H)<br>"
                            "Show a highlight effect on the selected part<br>"
                            "in the live view."),
                          highlight_cb);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM_RIGHT);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(live_view_ly, "elm.swallow.highlight", btn);
   td->highlight_btn = btn;

   btn = tools_btn_create(live_view_ly, "dummy",
                          _("Dummy parts (Ctrl + U)<br>"
                            "Display virtual images for the swallow and<br>"
                            "spacer parts."),
                          dummy_cb);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM_RIGHT);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(live_view_ly, "elm.swallow.dummy", btn);
   td->swallow_btn = btn;

   btn = tools_btn_create(live_view_ly, "wireframes_icon",
                          _("Wireframes (Ctrl + W)<br>"
                            "Display wireframes to identify the parts<br>"
                            "boundaries."),
                          wireframes_cb);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM_RIGHT);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(live_view_ly, "elm.swallow.wireframes", btn);
   td->wireframes_btn = btn;

   btn = tools_btn_create(live_view_ly, "mirror",
                          _("Mirror mode (Ctrl + M)<br>"
                            "Invert the layout horizontally and review<br>"
                            "the designed layout in RTL(right-to-left)<br>"
                            "LTR(left-to-right) settings."),
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
                          _("Save the file (Ctrl + S)<br>"
                            "Save the current script to a file."),
                          save_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.save", btn);

   btn = tools_btn_create(text_editor_ly, "undo",
                          _("Undo text (Ctrl + Z)"),
                          undo_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.undo", btn);

   btn = tools_btn_create(text_editor_ly, "redo",
                          _("Redo text (Ctrl + R)"),
                          redo_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.redo", btn);

   btn = tools_btn_create(text_editor_ly, "find",
                          _("Find/Replace (Ctrl + F)<br>"
                            "Find or replace text."),
                          find_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.find", btn);
   td->find_btn = btn;

   btn = tools_btn_create(text_editor_ly, "goto",
                          _("Go to line (Ctrl + L)<br>"
                            "Open the Go to window to move the cursor<br>"
                            "line position."),
                          goto_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.goto", btn);
   td->goto_btn = btn;

   btn = tools_btn_create(text_editor_ly, "lines",
                          _("Line numbers (F5)<br>"
                            "Display the script line numbers."),
                          lines_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.lines", btn);
   td->lines_btn = btn;

   btn = tools_btn_create(text_editor_ly, "template",
                          _("Insert a code snippet (Ctrl + T)<br>"
                            "Enventor chooses the best code with regards<br>"
                            "to the current editing context. For instance,<br>"
                            "if the cursor is inside a part section,<br>"
                            "description code is generated."),
                          template_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.template", btn);
   td->template_btn = btn;

   btn = tools_btn_create(text_editor_ly, "console",
                          _("Console box (Alt + Down)<br>"
                            "Display the console box, which shows the EDC<br>"
                            "build logs, such as error messages. It pops<br>"
                            "up automatically when compliation errors occur."),
                          console_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.console", btn);
   td->console_btn = btn;

   btn = tools_btn_create(text_editor_ly, "file_browser",
                          _("File browser (F9)<br>"
                           "Display the file browser, which shows a file list<br>"
                            "in current workspace."),
                          file_browser_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.file_browser", btn);
   td->file_browser_btn = btn;

   btn = tools_btn_create(text_editor_ly, "edc_navigator",
                          _("EDC navigator (F10)<br>"
                            "Display the EDC navigator, which shows the current<br>"
                            "group hierarchy tree that contains parts,<br>"
                            "descriptions and programs lists."),
                          edc_navigator_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.edc_navigator",
                               btn);
   td->edc_navigator_btn = btn;

   btn = tools_btn_create(text_editor_ly, "filetab",
                          _("File tab (F11)<br>"
                            "Display the file tab in the bottom area<br>"
                             "It shows an opened file list to switch<br>"
                             "files quickly. to switch files quickly."),
                          file_tab_cb);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(text_editor_ly, "elm.swallow.file_tab", btn);
   td->file_tab = btn;

   btn = tools_btn_create(text_editor_ly, "menu",
                          _("Enventor menu (Esc)<br>"
                          "Open the Enventor main menu."),
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
   EINA_SAFETY_ON_NULL_RETURN(td);

   if (toggle) config_part_highlight_set(!config_part_highlight_get());
   enventor_object_part_highlight_set(base_enventor_get(),
                                      config_part_highlight_get());
   if (toggle)
     {
        if (config_part_highlight_get())
          stats_info_msg_update(_("Part highlighting enabled."));
        else
          stats_info_msg_update(_("Part highlighting disabled."));
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
   EINA_SAFETY_ON_NULL_RETURN(td);

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
   EINA_SAFETY_ON_NULL_RETURN(td);

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
   EINA_SAFETY_ON_NULL_RETURN(td);

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
   EINA_SAFETY_ON_NULL_RETURN(td);

   if (toggle) config_dummy_parts_set(!config_dummy_parts_get());
   enventor_object_dummy_parts_set(base_enventor_get(),
                                   config_dummy_parts_get());

   if (toggle)
     {
        if (config_dummy_parts_get())
          stats_info_msg_update(_("Dummy parts enabled."));
        else
          stats_info_msg_update(_("Dummy parts disabled."));
     }
   //Toggle on/off
   if (config_dummy_parts_get())
     elm_object_signal_emit(td->swallow_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->swallow_btn, "icon,highlight,disabled", "");
}

void
tools_wireframes_update(Eina_Bool toggle)
{
   tools_data *td = g_td;
   EINA_SAFETY_ON_NULL_RETURN(td);

   if (toggle) config_wireframes_set(!config_wireframes_get());
   enventor_object_wireframes_set(base_enventor_get(),
                                     config_wireframes_get());

   if (toggle)
     {
        if (config_wireframes_get())
          stats_info_msg_update(_("Wireframes enabled."));
        else
          stats_info_msg_update(_("Wireframes disabled."));
     }
   //Toggle on/off
   if (config_wireframes_get())
     elm_object_signal_emit(td->wireframes_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->wireframes_btn, "icon,highlight,disabled", "");
}

void
tools_mirror_mode_update(Eina_Bool toggle)
{
   tools_data *td = g_td;
   EINA_SAFETY_ON_NULL_RETURN(td);

   if (toggle) config_mirror_mode_set(!config_mirror_mode_get());
   enventor_object_mirror_mode_set(base_enventor_get(),
                                   config_mirror_mode_get());
   live_edit_update();

   if (toggle)
     {
        if (config_mirror_mode_get())
          stats_info_msg_update(_("Mirror mode enabled."));
        else
          stats_info_msg_update(_("Mirror mode disabled."));
     }

   //Toggle on/off
   if (config_mirror_mode_get())
     elm_object_signal_emit(td->mirror_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->mirror_btn, "icon,highlight,disabled", "");
}

void
tools_file_tab_update(Eina_Bool toggle)
{
   tools_data *td = g_td;
   EINA_SAFETY_ON_NULL_RETURN(td);

   base_file_tab_toggle(toggle);

   //Toggle on/off
   if (config_file_tab_get())
     elm_object_signal_emit(td->file_tab, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->file_tab, "icon,highlight,disabled", "");
}

void
tools_goto_update(void)
{
   tools_data *td = g_td;
   EINA_SAFETY_ON_NULL_RETURN(td);

   if (goto_is_opened())
     elm_object_signal_emit(td->goto_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->goto_btn, "icon,highlight,disabled", "");
}

void
tools_search_update(void)
{
   tools_data *td = g_td;
   EINA_SAFETY_ON_NULL_RETURN(td);

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

   Enventor_Item *it = file_mgr_focused_item_get();

   char syntax[12];
   if (enventor_item_template_insert(file_mgr_focused_item_get(), syntax,
                                     sizeof(syntax)))
     {
        char msg[64];
        snprintf(msg, sizeof(msg), _("Template code inserted, (%s)"), syntax);
        stats_info_msg_update(msg);
        enventor_item_file_save(it, NULL);
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
   EINA_SAFETY_ON_NULL_RETURN(td);

   if (on)
     elm_object_signal_emit(td->console_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->console_btn, "icon,highlight,disabled", "");
}

void
tools_menu_update(Eina_Bool on)
{
   tools_data *td = g_td;
   EINA_SAFETY_ON_NULL_RETURN(td);

   if (on)
     elm_object_signal_emit(td->menu_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->menu_btn, "icon,highlight,disabled", "");
}
