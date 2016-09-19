#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "common.h"

typedef struct preference_setting_s
{
   Evas_Object *box;
   Evas_Object *view_size_w_entry;
   Evas_Object *view_size_h_entry;
   Evas_Object *toggle_tools;
   Evas_Object *toggle_status;
   Evas_Object *toggle_console;
   Evas_Object *toggle_indent;
   Evas_Object *toggle_autocomp;
   Evas_Object *toggle_smart_undo_redo;
   Evas_Object *toggle_red_alert;
} preference_setting_data;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/
static Evas_Object *
label_create(Evas_Object *parent, const char *text)
{
   Evas_Object *label = elm_label_add(parent);
   elm_object_text_set(label, text);
   evas_object_show(label);

   return label;
}

static Evas_Object *
toggle_create(Evas_Object *parent, const char *text, Eina_Bool state,
              const char *tooltip_msg)
{
   Evas_Object *toggle = elm_check_add(parent);
   elm_object_style_set(toggle, "toggle");
   elm_check_state_set(toggle, state);
   evas_object_size_hint_weight_set(toggle, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(toggle, EVAS_HINT_FILL, 0);
   elm_object_text_set(toggle, text);
   elm_object_tooltip_text_set(toggle, tooltip_msg);
   evas_object_show(toggle);

   return toggle;
}

static Evas_Object *
entry_create(Evas_Object *parent)
{
   Evas_Object *entry = elm_entry_add(parent);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   evas_object_show(entry);

   return entry;
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/
void
preference_setting_focus_set(preference_setting_data *psd)
{
   EINA_SAFETY_ON_NULL_RETURN(psd);
   elm_object_focus_set(psd->view_size_w_entry, EINA_TRUE);
}

void
preference_setting_config_set(preference_setting_data *psd)
{
   EINA_SAFETY_ON_NULL_RETURN(psd);

   config_tools_set(elm_check_state_get(psd->toggle_tools));
   config_stats_bar_set(elm_check_state_get(psd->toggle_status));
   config_console_set(elm_check_state_get(psd->toggle_console));
   config_auto_indent_set(elm_check_state_get(psd->toggle_indent));
   config_auto_complete_set(elm_check_state_get(psd->toggle_autocomp));
   config_smart_undo_redo_set(elm_check_state_get(psd->toggle_smart_undo_redo));
   config_red_alert_set(elm_check_state_get(psd->toggle_red_alert));

   Evas_Coord w = 0;
   Evas_Coord h = 0;
   const char *w_entry = elm_entry_entry_get(psd->view_size_w_entry);
   if (w_entry) w = (Evas_Coord)atoi(w_entry);
   const char *h_entry = elm_entry_entry_get(psd->view_size_h_entry);
   if (h_entry) h = (Evas_Coord)atoi(h_entry);
   config_view_size_set(w, h);
}

void
preference_setting_reset(preference_setting_data *psd)
{
   EINA_SAFETY_ON_NULL_RETURN(psd);

   elm_check_state_set(psd->toggle_tools, config_tools_get());
   elm_check_state_set(psd->toggle_status, config_stats_bar_get());
   elm_check_state_set(psd->toggle_console, config_console_get());
   elm_check_state_set(psd->toggle_indent, config_auto_indent_get());
   elm_check_state_set(psd->toggle_autocomp, config_auto_complete_get());
   elm_check_state_set(psd->toggle_smart_undo_redo,
                       config_smart_undo_redo_get());
   elm_check_state_set(psd->toggle_red_alert, config_red_alert_get());

   //Reset view scale
   int view_size_w, view_size_h;
   config_view_size_get(&view_size_w, &view_size_h);
   char buf[10];
   snprintf(buf, sizeof(buf), "%d", view_size_w);
   elm_entry_entry_set(psd->view_size_w_entry, buf);
   snprintf(buf, sizeof(buf), "%d", view_size_h);
   elm_entry_entry_set(psd->view_size_h_entry, buf);
}

Evas_Object *
preference_setting_content_get(preference_setting_data *psd,
                               Evas_Object *parent)
{
   static Elm_Entry_Filter_Accept_Set digits_filter_data;
   static Elm_Entry_Filter_Limit_Size limit_filter_data;

   EINA_SAFETY_ON_NULL_RETURN_VAL(psd, NULL);
   if (psd->box) return psd->box;

   //Preference

   //Box
   Evas_Object *box = elm_box_add(parent);
   elm_box_padding_set(box, 0, ELM_SCALE_SIZE(5));
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   Evas_Object *rect;

   //Spacer
   rect = evas_object_rectangle_add(evas_object_evas_get(box));
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_min_set(rect, 0, ELM_SCALE_SIZE(1));
   elm_box_pack_end(box, rect);

   Evas_Object *box2;
   Evas_Object *layout_padding3;

   //View Size

   //Box for View Size
   box2 = elm_box_add(box);
   elm_box_horizontal_set(box2, EINA_TRUE);
   elm_box_padding_set(box2, ELM_SCALE_SIZE(5), 0);
   evas_object_size_hint_weight_set(box2, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(box2, EVAS_HINT_FILL, 0);
   evas_object_show(box2);

   elm_box_pack_end(box, box2);

   elm_object_tooltip_text_set(box2,
                               _("Set default size of live view.<br>"
                                 "When you open a new group, Its<br>"
                                 "view size will be set with this."));
   //Label (View Size)

   /* This layout is intended to put the label aligned to left side
      far from 3 pixels. */
   layout_padding3 = elm_layout_add(box2);
   elm_layout_file_set(layout_padding3, EDJE_PATH, "padding3_layout");
   evas_object_show(layout_padding3);

   elm_box_pack_end(box2, layout_padding3);

   Evas_Object *label_view_size = label_create(layout_padding3,
                                               _("Default View Size"));
   elm_object_part_content_set(layout_padding3, "elm.swallow.content",
                               label_view_size);

   //Spacer
   rect = evas_object_rectangle_add(evas_object_evas_get(box2));
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box2, rect);

   Evas_Coord w, h;
   char w_str[5], h_str[5];
   config_view_size_get(&w, &h);
   snprintf(w_str, sizeof(w_str), "%d", w);
   snprintf(h_str, sizeof(h_str), "%d", h);

   //Entry (View Width)
   Evas_Object *entry_view_size_w = entry_create(box2);
   evas_object_size_hint_weight_set(entry_view_size_w, 0.15, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(entry_view_size_w, EVAS_HINT_FILL, EVAS_HINT_FILL);

   digits_filter_data.accepted = "0123456789";
   digits_filter_data.rejected = NULL;
   elm_entry_markup_filter_append(entry_view_size_w,
                                  elm_entry_filter_accept_set,
                                  &digits_filter_data);
   limit_filter_data.max_char_count = 4;
   limit_filter_data.max_byte_count = 0;
   elm_entry_markup_filter_append(entry_view_size_w,
                                  elm_entry_filter_limit_size,
                                  &limit_filter_data);

   elm_object_text_set(entry_view_size_w, w_str);
   elm_box_pack_end(box2, entry_view_size_w);

   //Label (X)
   Evas_Object *label_view_size_x = label_create(box2, "X");
   elm_box_pack_end(box2, label_view_size_x);

   //Entry (View Height)
   Evas_Object *entry_view_size_h = entry_create(box2);
   evas_object_size_hint_weight_set(entry_view_size_h, 0.15, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(entry_view_size_h, EVAS_HINT_FILL, EVAS_HINT_FILL);

   elm_entry_markup_filter_append(entry_view_size_h,
                                  elm_entry_filter_accept_set,
                                  &digits_filter_data);
   elm_entry_markup_filter_append(entry_view_size_h,
                                  elm_entry_filter_limit_size,
                                  &limit_filter_data);

   elm_object_text_set(entry_view_size_h, h_str);
   elm_box_pack_end(box2, entry_view_size_h);

   //Toggle (Tools)
   Evas_Object *toggle_tools =
      toggle_create(box, _("Tools"),
                    config_tools_get(),
                    _("Tools (F7)<br>"
                      "Display Tools on the top area.<br>"
                      "Tools displays the essential function <br>"
                      "toggles to edit the layout."));
   elm_box_pack_end(box, toggle_tools);

   //Toggle (Status)
   Evas_Object *toggle_status =
      toggle_create(box, _("Status"), config_stats_bar_get(),
                    _("Status (F8)<br>"
                      "Display Status bar, which shows subsidiary<br>"
                      "information  for editing in the bottom area."
                      "editing."));
   elm_box_pack_end(box, toggle_status);

   //Toggle (Console)
   Evas_Object *toggle_console =
      toggle_create(box, _("Auto Hiding Console"), config_console_get(),
                    _("Hide the console box automatically<br>"
                      "when no messages are to be shown.<br>"
                      "for example, when you have fixed all<br>"
                      "grammatical errors."));
   elm_box_pack_end(box, toggle_console);

   //Toggle (Auto Indentation)
   Evas_Object *toggle_indent =
      toggle_create(box, _("Auto Indentation"), config_auto_indent_get(),
                    _("Auto indentation (Ctrl + I)<br>"
                      "Apply automatic indentation for text editing.<br>"
                      "When wrapping the text around, Enventor<br>"
                      "inserts the line indentation automatically."));
   elm_box_pack_end(box, toggle_indent);

   //Toggle (Auto Completion)
   Evas_Object *toggle_autocomp =
      toggle_create(box, _("Auto Completion"), config_auto_complete_get(),
                    _("Display the candidate keyword popup with<br>"
                      "regards to the current editing contxt.<br>"
                      "When you type texts in the editor, the candidate<br>"
                      "popup appears. You can choose an item<br>"
                      "from the list, and a code template is inserted."));
   elm_box_pack_end(box, toggle_autocomp);

   //Toggle (Smart Undo/Redo)
   Evas_Object *toggle_smart_undo_redo =
      toggle_create(box, _("Smart Undo/Redo"), config_smart_undo_redo_get(),
                    _("Redo/Undo text by word. If disabled, redoing<br>"
                      "and undoing works by character."));
   elm_box_pack_end(box, toggle_smart_undo_redo);

   //Toggle (Red Alert)
   Evas_Object *toggle_red_alert =
      toggle_create(box, _("Error Message Red Alert"), config_red_alert_get(),
                    _("Enable error message red alert effect.<br>"
                      "When EDC compilation fails because of<br>"
                      "a grammar error, Enventor alerts<br>"
                      "you with a fading screen effect."));
   evas_object_size_hint_weight_set(toggle_red_alert, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(toggle_red_alert, EVAS_HINT_FILL, 0);
   elm_box_pack_end(box, toggle_red_alert);

   psd->box = box;
   psd->view_size_w_entry = entry_view_size_w;
   psd->view_size_h_entry = entry_view_size_h;
   psd->toggle_tools = toggle_tools;
   psd->toggle_status = toggle_status;
   psd->toggle_console = toggle_console;
   psd->toggle_indent = toggle_indent;
   psd->toggle_autocomp = toggle_autocomp;
   psd->toggle_smart_undo_redo = toggle_smart_undo_redo;
   psd->toggle_red_alert = toggle_red_alert;

   return box;
}

preference_setting_data *
preference_setting_init(void)
{
   preference_setting_data *psd = calloc(1, sizeof(preference_setting_data));
   if (!psd)
     {
        mem_fail_msg();
        return NULL;
     }
   return psd;
}

void
preference_setting_term(preference_setting_data *psd)
{
   EINA_SAFETY_ON_NULL_RETURN(psd);

   evas_object_del(psd->box);
   free(psd);
}
