#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

typedef struct help_data_s
{
   Evas_Object *list;
   Evas_Object *box;
   Evas_Object *button;
} help_data;

static help_data *g_hd = NULL;
static Evas_Coord win_x = -1;
static Evas_Coord win_y = -1;
static Evas_Coord win_w = DEFAULT_HELP_WIN_W;
static Evas_Coord win_h = DEFAULT_HELP_WIN_H;
static Evas_Object *g_win = NULL;

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
   Evas_Object *label;
   Evas_Object *entry;
   char buf[PATH_MAX];
   help_data *hd = g_hd;
   char *item = data;

   elm_box_clear(hd->box);

   //Label
   label =  elm_label_add(hd->box);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(hd->box, label);
   evas_object_show(label);

   //Entry
   entry = elm_entry_add(hd->box);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_line_wrap_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_FALSE);
   elm_entry_line_wrap_set(entry, ELM_WRAP_MIXED);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(hd->box, entry);
   evas_object_show(entry);

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

   //Back Button
   evas_object_show(hd->button);
}

static void
backbutton_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
   help_data *hd = g_hd;
   Evas_Object *box = data;
   elm_box_clear(box);

   elm_list_item_selected_set(elm_list_selected_item_get(hd->list), EINA_FALSE);
   evas_object_hide(hd->button);
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

   help_data *hd = malloc(sizeof(help_data));
   if (!hd)
   {
      mem_fail_msg();
      return ;
   }
   g_hd = hd;

   char buf[PATH_MAX];

   //Win
   win = elm_win_add(base_win_get(), _("Enventor Help"), ELM_WIN_BASIC);
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   snprintf(buf, sizeof(buf), "About Enventor v%s", PACKAGE_VERSION);
   elm_win_title_set(win, buf);
   evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb,
                                  NULL);
   evas_object_smart_callback_add(win, "moved", win_moved_cb, NULL);

   //Bg
   Evas_Object *bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);
   elm_win_resize_object_add(win, bg);

   //Box
   Evas_Object *box = elm_box_add(win);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, box);
   evas_object_show(box);

   //Title Bg
   Evas_Object *title_bg = elm_image_add(box);
   elm_image_file_set(title_bg, EDJE_PATH, "about");
   evas_object_size_hint_align_set(title_bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(title_bg, 430, 100);
   elm_box_pack_end(box, title_bg);
   evas_object_show(title_bg);

   //Entry Box
   Evas_Object *entry_box = elm_box_add(win);
   evas_object_size_hint_weight_set(entry_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, entry_box);
   evas_object_show(entry_box);
   hd->box = entry_box;

   //List
   Evas_Object *list = elm_list_add(box);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_tree_focus_allow_set(list, EINA_FALSE);
   elm_list_select_mode_set(list, ELM_OBJECT_SELECT_MODE_DEFAULT);
   evas_object_show(list);
   elm_box_pack_end(box, list);
   hd->list = list;

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

   // Back Button Box
   Evas_Object *button_box = elm_box_add(win);
   evas_object_size_hint_weight_set(button_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, button_box);
   evas_object_show(button_box);

   //Back Button
   Evas_Object *button = elm_button_add(button_box);
   evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(button, 0.95, 0.95);
   evas_object_size_hint_min_set(button, 60, 30);
   elm_object_text_set(button, "Back");
   evas_object_smart_callback_add(button, "clicked", backbutton_clicked_cb, entry_box);
   elm_box_pack_end(button_box, button);
   evas_object_hide(button);
   hd->button = button;

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
   free(g_hd);

   //Save last state
   evas_object_geometry_get(win, NULL, NULL, &win_w, &win_h);
   elm_win_screen_position_get(win, &win_x, &win_y);
   evas_object_del(win);
   g_win = NULL;
}
