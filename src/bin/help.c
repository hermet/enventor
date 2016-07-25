#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

static Evas_Coord win_x = -1;
static Evas_Coord win_y = -1;
static Evas_Coord win_w = DEFAULT_HELP_WIN_W;
static Evas_Coord win_h = DEFAULT_HELP_WIN_H;
static Evas_Object *g_win = NULL;
static Evas_Object *g_layout = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
keygrabber_key_down_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                       Evas_Object *obj  EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   help_close();
}

static void
win_delete_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   help_close();
}

static void
win_moved_cb(void *data EINA_UNUSED, Evas_Object *obj,
             void *event_info EINA_UNUSED)
{
   /* Move the window with the previous remembered position when the window is
      moved by window manager first time. */
   if ((win_x != -1) || (win_y != -1)) evas_object_move(obj, win_x, win_y);
   evas_object_smart_callback_del(obj, "moved", win_moved_cb);
}

static void
list_item_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *layout;
   Evas_Object *label;
   Evas_Object *entry;
   char buf[PATH_MAX];
   char *item = data;

   layout = g_layout;
   label = elm_object_part_content_get(layout, "swallow_label");
   entry = elm_object_part_content_get(layout, "swallow_entry");
   elm_object_signal_emit(layout, "elm,state,content,show", "");

   //Read File
   if (!strcmp(item, "about"))
   {
      elm_object_text_set(label, "<font_size=11><b>About</b></font_size>");
      snprintf(buf, sizeof(buf), "%s/help/INTRO", elm_app_data_dir_get());
      elm_entry_autosave_set(entry, EINA_FALSE);
      elm_entry_file_set(entry, buf, ELM_TEXT_FORMAT_MARKUP_UTF8);
   }
   else if (!strcmp(item, "history"))
   {
      elm_object_text_set(label, "<font_size=11><b>Version History</b></font_size>");
      snprintf(buf, sizeof(buf), "%s/help/HISTORY", elm_app_data_dir_get());
      elm_entry_autosave_set(entry, EINA_FALSE);
      elm_entry_file_set(entry, buf, ELM_TEXT_FORMAT_MARKUP_UTF8);
   }
   else if (!strcmp(item, "shortcut"))
   {
      elm_object_text_set(label, "<font_size=11><b>Shortcut Keys</b></font_size>");
      snprintf(buf, sizeof(buf), "%s/help/SHORTCUT", elm_app_data_dir_get());
      elm_entry_autosave_set(entry, EINA_FALSE);
      elm_entry_file_set(entry, buf, ELM_TEXT_FORMAT_MARKUP_UTF8);
   }
   else if (!strcmp(item, "command"))
   {
      elm_object_text_set(label, "<font_size=11><b>Commandline Usage</b></font_size>");
      snprintf(buf, sizeof(buf), "%s/help/COMMAND", elm_app_data_dir_get());
      elm_entry_autosave_set(entry, EINA_FALSE);
      elm_entry_file_set(entry, buf, ELM_TEXT_FORMAT_MARKUP_UTF8);
   }
   else if (!strcmp(item, "devel"))
   {
      elm_object_text_set(label, "<font_size=11><b>Developers</b></font_size>");
      snprintf(buf, sizeof(buf), "%s/help/DEVEL", elm_app_data_dir_get());
      elm_entry_autosave_set(entry, EINA_FALSE);
      elm_entry_file_set(entry, buf, ELM_TEXT_FORMAT_MARKUP_UTF8);
   }
}

static void
button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *layout = g_layout;
   Evas_Object *list = elm_object_part_content_get(layout, "swallow_list");
   elm_list_item_selected_set(elm_list_selected_item_get(list), EINA_FALSE);
   elm_object_signal_emit(layout, "elm,state,content,hide", "");
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
help_open(void)
{
   Evas_Object *win = g_win;
   if (win)
   {
      elm_win_activate(win);
      return;
   }

   char buf[PATH_MAX];

   //Win
   win = elm_win_add(base_win_get(), _("Enventor Help"), ELM_WIN_BASIC);
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   snprintf(buf, sizeof(buf), "About Enventor v%s", PACKAGE_VERSION);
   elm_win_title_set(win, buf);
   evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb,
                                  NULL);
   evas_object_smart_callback_add(win, "moved", win_moved_cb, NULL);

   //Layout
   Evas_Object *layout = elm_layout_add(win);
   elm_layout_file_set(layout, EDJE_PATH, "help_layout");
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, layout);
   evas_object_show(layout);
   g_layout = layout;

   //List
   Evas_Object *list = elm_list_add(win);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_tree_focus_allow_set(list, EINA_FALSE);
   elm_list_select_mode_set(list, ELM_OBJECT_SELECT_MODE_DEFAULT);
   evas_object_show(list);

   elm_object_part_content_set(layout, "swallow_list", list);

   elm_list_item_append(list, "About", NULL, NULL, list_item_selected_cb,
                        "about");
   elm_list_item_append(list, "Version History", NULL, NULL, list_item_selected_cb,
                        "history");
   elm_list_item_append(list, "Shortcut Keys", NULL, NULL, list_item_selected_cb,
                        "shortcut");
   elm_list_item_append(list, "Commandline Usage", NULL, NULL, list_item_selected_cb,
                        "command");
   elm_list_item_append(list, "Developers", NULL, NULL, list_item_selected_cb,
                        "devel");

   //Label
   Evas_Object *label =  elm_label_add(win);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(label);

   elm_object_part_content_set(layout, "swallow_label", label);

   //Entry
   Evas_Object *entry = elm_entry_add(win);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_line_wrap_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_FALSE);
   elm_entry_line_wrap_set(entry, ELM_WRAP_MIXED);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(entry);

   elm_object_part_content_set(layout, "swallow_entry", entry);

   //Back Button
   Evas_Object *button = elm_button_add(win);
   evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(button, "clicked", button_clicked_cb, NULL);
   elm_object_focus_allow_set(button, EINA_FALSE);
   elm_object_style_set(button, ENVENTOR_NAME);
   evas_object_show(button);

   //Back Button Icon
   Evas_Object *back_img = elm_image_add(button);
   elm_image_file_set(back_img, EDJE_PATH, "close");
   elm_object_content_set(button, back_img);

   elm_object_part_content_set(layout, "swallow_button", button);

   //Content hide
   elm_object_signal_emit(layout, "elm,state,content,hide", "");

   //Window
   win_w = (Evas_Coord) ((double) win_w * elm_config_scale_get());
   win_h = (Evas_Coord) ((double) win_h * elm_config_scale_get());
   evas_object_resize(win, win_w, win_h);
   evas_object_show(win);

   //Keygrabber
   Evas_Object *keygrabber =
      evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_event_callback_add(keygrabber, EVAS_CALLBACK_KEY_DOWN,
                                  keygrabber_key_down_cb, NULL);
   if (!evas_object_key_grab(keygrabber, "Escape", 0, 0, EINA_TRUE))
     EINA_LOG_ERR(_("Failed to grab key - Escape"));

   g_win = win;
}

void
help_close(void)
{
   Evas_Object *win = g_win;
   if (!win) return;

   Evas_Object *layout = g_layout;
   if (!layout) return;
   evas_object_del(layout);
   g_layout = NULL;

   //Save last state
   evas_object_geometry_get(win, NULL, NULL, &win_w, &win_h);
   elm_win_screen_position_get(win, &win_x, &win_y);
   evas_object_del(win);
   g_win = NULL;
}
