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
   Evas_Object *highlight_btn;
   Evas_Object *mirror_btn;
   Evas_Object *goto_btn;
   Evas_Object *find_btn;
   Evas_Object *console_btn;
   Evas_Object *menu_btn;
   Evas_Object *box;
} tools_data;

static tools_data *g_td = NULL;

static void
menu_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   live_edit_cancel();
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
   live_edit_cancel();
   if (search_close()) return;
   else search_open();
}

static void
goto_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   live_edit_cancel();
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
   elm_object_style_set(btn, "enventor");
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

Evas_Object *
tools_init(Evas_Object *parent)
{
   tools_data *td = g_td;
   if (td) return (td->box);

   td = calloc(1, sizeof(tools_data));
   if (!td)
     {
        EINA_LOG_ERR(_("Failed to allocate Memory!"));
        return NULL;
     }
   g_td = td;

   Evas_Object *box = elm_box_add(parent);
   elm_box_horizontal_set(box, EINA_TRUE);
   elm_box_padding_set(box, 10, 0);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);

   Evas_Object *btn;
   btn = tools_btn_create(box, "highlight", _("Part Highlighting (Ctrl + H)"),
                          highlight_cb);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM_RIGHT);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->highlight_btn = btn;

   btn = tools_btn_create(box, "dummy", _("Dummy Parts (Ctrl + W)"),
                          dummy_cb);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM_RIGHT);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->swallow_btn = btn;

   //icon image is temporary, it should be changed to its own icon.
   btn = tools_btn_create(box, "mirror", _("Mirror Mode (Ctrl + M)"),
                          mirror_cb);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->mirror_btn = btn;

   Evas_Object *sp;
   sp = elm_separator_add(box);
   evas_object_show(sp);
   elm_box_pack_end(box, sp);

   //Live edit tool
   btn = live_edit_tools_create(box);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   sp = elm_separator_add(box);
   evas_object_show(sp);
   elm_box_pack_end(box, sp);

   //For a empty space
   Evas_Object *rect = evas_object_rectangle_add(evas_object_evas_get(box));
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, rect);

   btn = tools_btn_create(box, "save",_("Save File (Ctrl + S)"),
                          save_cb);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   btn = tools_btn_create(box, "undo", _("Undo Text (Ctrl + Z)"),
                          undo_cb);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   btn = tools_btn_create(box, "redo", _("Redo Text (Ctrl + R)"),
                          redo_cb);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   btn = tools_btn_create(box, "find", _("Find/Replace (Ctrl + F)"),
                          find_cb);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->find_btn = btn;

   btn = tools_btn_create(box, "goto", _("Goto Lines (Ctrl + L)"),
                          goto_cb);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->goto_btn = btn;

   btn = tools_btn_create(box, "lines", _("Line Numbers (F5)"),
                          lines_cb);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->lines_btn = btn;

   sp = elm_separator_add(box);
   evas_object_show(sp);
   elm_box_pack_end(box, sp);

   btn = tools_btn_create(box, "console", _("Console Box (Alt + Down)"),
                          console_cb);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 1.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->console_btn = btn;

   btn = tools_btn_create(box, "file_browser", _("File Browser (F9)"),
                          file_browser_cb);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 1.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->file_browser_btn = btn;

   btn = tools_btn_create(box, "edc_navigator", _("EDC Navigator (F10)"),
                          edc_navigator_cb);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 1.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->edc_navigator_btn = btn;

   btn = tools_btn_create(box, "status", _("Status (F11)"), status_cb);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 1.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->status_btn = btn;

   sp = elm_separator_add(box);
   evas_object_show(sp);
   elm_box_pack_end(box, sp);

   btn = tools_btn_create(box, "menu", _("Enventor Menu (Esc)"),
                          menu_cb);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM_LEFT);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->menu_btn = btn;

   evas_object_show(box);

   td->box = box;

   //Turn on if console is valid size.
   if (!config_console_get() && (config_console_size_get() > 0))
     tools_console_update(EINA_TRUE);

   return box;
}

Evas_Object *
tools_live_edit_get(Evas_Object *tools)
{
   return evas_object_data_get(tools, "live_edit");
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
