#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

typedef struct tools_s
{
   Evas_Object *swallow_btn;
   Evas_Object *status_btn;
   Evas_Object *lines_btn;
   Evas_Object *highlight_btn;
   Evas_Object *goto_btn;
   Evas_Object *find_btn;
   Evas_Object *live_btn;
   Evas_Object *console_btn;
   Evas_Object *menu_btn;
   Evas_Object *box;
} tools_data;

static tools_data *g_td = NULL;

static void
menu_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *enventor = data;

   if (live_edit_get()) live_edit_cancel();
   if (search_is_opened()) search_close();
   if (goto_is_opened()) tools_goto_update(enventor, EINA_TRUE);

   menu_toggle();
}

static void
highlight_cb(void *data, Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   Evas_Object *enventor = data;
   tools_highlight_update(enventor, EINA_TRUE);
}

static void
swallow_cb(void *data, Evas_Object *obj EINA_UNUSED,
           void *event_info EINA_UNUSED)
{
   Evas_Object *enventor = data;
   tools_swallow_update(enventor, EINA_TRUE);
}

static void
lines_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *enventor = data;
   tools_lines_update(enventor, EINA_TRUE);
}

static void
status_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
          void *event_info EINA_UNUSED)
{
   tools_status_update(NULL, EINA_TRUE);
}

static void
find_cb(void *data, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   Evas_Object *enventor = data;
   live_edit_cancel();
   if (search_is_opened()) search_close();
   else search_open(enventor);
}

static void
goto_cb(void *data, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   Evas_Object *enventor = data;
   live_edit_cancel();
   if (goto_is_opened()) goto_close();
   else goto_open(enventor);
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
live_edit_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   if (search_is_opened()) search_close();
   if (goto_is_opened()) goto_close();
   live_edit_toggle();
}

static void
save_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   file_mgr_edc_save();
}

static void
redo_cb(void *data, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   Evas_Object *enventor = data;
   if (enventor_object_redo(enventor))
     stats_info_msg_update("Redo text.");
   else
     stats_info_msg_update("No text to be redo.");
}

static void
undo_cb(void *data, Evas_Object *obj EINA_UNUSED,
        void *event_info EINA_UNUSED)
{
   Evas_Object *enventor = data;
   if (enventor_object_undo(enventor))
     stats_info_msg_update("Undo text.");
   else
     stats_info_msg_update("No text to be undo.");
}

static Evas_Object *
tools_btn_create(Evas_Object *parent, const char *icon,
                 const char *tooltip_msg, Evas_Smart_Cb func, void *data)
{
   Evas_Object *btn = elm_button_add(parent);
   elm_object_style_set(btn, elm_app_name_get());
   elm_object_focus_allow_set(btn, EINA_FALSE);
   elm_object_tooltip_text_set(btn, tooltip_msg);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM);

   Evas_Object *img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, icon);
   elm_object_content_set(btn, img);

   evas_object_smart_callback_add(btn, "clicked", func, data);
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
tools_init(Evas_Object *parent, Evas_Object *enventor)
{
   tools_data *td = g_td;
   if (td) return (td->box);

   td = calloc(1, sizeof(tools_data));
   if (!td)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
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
   btn = tools_btn_create(box, "menu", "Enventor Menu (Esc)",
                          menu_cb, enventor);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM_RIGHT);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->menu_btn = btn;

   Evas_Object *sp;
   sp = elm_separator_add(box);
   evas_object_show(sp);
   elm_box_pack_end(box, sp);

   btn = tools_btn_create(box, "save","Save File (Ctrl + S)",
                          save_cb, enventor);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   btn = tools_btn_create(box, "undo", "Undo Text (Ctrl + Z)",
                          undo_cb, enventor);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   btn = tools_btn_create(box, "redo", "Redo Text (Ctrl + R)",
                          redo_cb, enventor);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);

   btn = tools_btn_create(box, "find", "Find/Replace (Ctrl + F)",
                          find_cb, enventor);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->find_btn = btn;

   btn = tools_btn_create(box, "goto", "Goto Lines (Ctrl + L)",
                          goto_cb, enventor);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->goto_btn = btn;

   btn = tools_btn_create(box, "lines", "Line Numbers (F5)",
                          lines_cb, enventor);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->lines_btn = btn;

   sp = elm_separator_add(box);
   evas_object_show(sp);
   elm_box_pack_end(box, sp);

   btn = tools_btn_create(box, "highlight", "Part Highlighting (Ctrl + H)",
                          highlight_cb, enventor);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->highlight_btn = btn;

   btn = tools_btn_create(box, "swallow_s", "Dummy Swallow (Ctrl + W)",
                          swallow_cb, enventor);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->swallow_btn = btn;

   btn = tools_btn_create(box, "live_edit", "Live View Edit (Ctrl + E)",
                          live_edit_cb, enventor);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   evas_object_data_set(box, "live_edit", btn);
   td->live_btn = btn;

   sp = elm_separator_add(box);
   evas_object_show(sp);
   elm_box_pack_end(box, sp);

   btn = tools_btn_create(box, "console", "Console Box (Ctrl + Down)",
                          console_cb, NULL);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM_LEFT);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 1.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->console_btn = btn;

   btn = tools_btn_create(box, "status", "Status (F11)", status_cb, NULL);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM_LEFT);
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 1.0, EVAS_HINT_FILL);
   elm_box_pack_end(box, btn);
   td->status_btn = btn;

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
tools_highlight_update(Evas_Object *enventor, Eina_Bool toggle)
{
   tools_data *td = g_td;
   if (!td) return;

   if (toggle) config_part_highlight_set(!config_part_highlight_get());
   enventor_object_part_highlight_set(enventor,
                                      config_part_highlight_get());
   if (toggle)
     {
        if (config_part_highlight_get())
          stats_info_msg_update("Part Highlighting Enabled.");
        else
          stats_info_msg_update("Part Highlighting Disabled.");
     }

   //Toggle on/off
   if (config_part_highlight_get())
     elm_object_signal_emit(td->highlight_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->highlight_btn, "icon,highlight,disabled", "");
}

void
tools_lines_update(Evas_Object *enventor, Eina_Bool toggle)
{
   tools_data *td = g_td;
   if (!td) return;

   if (toggle) config_linenumber_set(!config_linenumber_get());
   enventor_object_linenumber_set(enventor, config_linenumber_get());

   //Toggle on/off
   if (config_linenumber_get())
     elm_object_signal_emit(td->lines_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->lines_btn, "icon,highlight,disabled", "");
}

void
tools_swallow_update(Evas_Object *enventor, Eina_Bool toggle)
{
   tools_data *td = g_td;
   if (!td) return;

   if (toggle) config_dummy_swallow_set(!config_dummy_swallow_get());
   enventor_object_dummy_swallow_set(enventor, config_dummy_swallow_get());

   if (toggle)
     {
        if (config_dummy_swallow_get())
          stats_info_msg_update("Dummy Swallow Enabled.");
        else
          stats_info_msg_update("Dummy Swallow Disabled.");
     }
   //Toggle on/off
   if (config_dummy_swallow_get())
     elm_object_signal_emit(td->swallow_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->swallow_btn, "icon,highlight,disabled", "");
}

void
tools_status_update(Evas_Object *enventor EINA_UNUSED, Eina_Bool toggle)
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
tools_goto_update(Evas_Object *enventor EINA_UNUSED,
                  Eina_Bool toggle EINA_UNUSED)
{
   tools_data *td = g_td;
   if (!td) return;

   if (goto_is_opened())
     elm_object_signal_emit(td->goto_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->goto_btn, "icon,highlight,disabled", "");
}

void
tools_search_update(Evas_Object *enventor EINA_UNUSED,
                    Eina_Bool toggle EINA_UNUSED)
{
   tools_data *td = g_td;
   if (!td) return;

   if (search_is_opened())
     elm_object_signal_emit(td->find_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->find_btn, "icon,highlight,disabled", "");
}

void
tools_live_update(Eina_Bool on)
{
   tools_data *td = g_td;
   if (!td) return;

   if (on)
     elm_object_signal_emit(td->live_btn, "icon,highlight,enabled", "");
   else
     elm_object_signal_emit(td->live_btn, "icon,highlight,disabled", "");
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
