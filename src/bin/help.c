#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

static Evas_Coord win_x = -1;
static Evas_Coord win_y = -1;
static Evas_Coord win_w = DEFAULT_HELP_WIN_W;
static Evas_Coord win_h = DEFAULT_HELP_WIN_H;
static Evas_Object *g_win = NULL;

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

void
help_open(void)
{
   Evas_Object *win = g_win;
   if (win)
   {
      elm_win_activate(win);
      return;
   }

   Evas_Object *entry;
   char buf[PATH_MAX];

   //Win
   win = elm_win_add(base_win_get(), _("Enventor Help"), ELM_WIN_BASIC);
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   elm_win_title_set(win, _("Help"));
   evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb,
                                  NULL);
   evas_object_smart_callback_add(win, "moved", win_moved_cb, NULL);

   //Bg
   Evas_Object *bg = elm_bg_add(win);
   evas_object_show(bg);
   elm_win_resize_object_add(win, bg);

   //Scroller
   Evas_Object *scroller = elm_scroller_add(win);
   evas_object_show(scroller);
   elm_win_resize_object_add(win, scroller);

   //Box
   Evas_Object *box = elm_box_add(win);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);
   elm_object_content_set(scroller, box);

   //Frame1: Introduction
   Evas_Object *intro_frame = elm_frame_add(box);
   elm_frame_autocollapse_set(intro_frame, EINA_TRUE);
   elm_object_text_set(intro_frame, "About");
   evas_object_size_hint_weight_set(intro_frame, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(intro_frame, EVAS_HINT_FILL, 0);
   evas_object_show(intro_frame);
   elm_box_pack_end(box, intro_frame);

   //Entry
   entry = elm_entry_add(intro_frame);
   elm_entry_scrollable_set(entry, EINA_FALSE);
   elm_entry_line_wrap_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_FALSE);
   elm_entry_line_wrap_set(entry, ELM_WRAP_MIXED);
   elm_object_content_set(intro_frame, entry);

   //Read File
   snprintf(buf, sizeof(buf), "%s/help/INTRO", elm_app_data_dir_get());
   elm_entry_autosave_set(entry, EINA_FALSE);
   elm_entry_file_set(entry, buf, ELM_TEXT_FORMAT_MARKUP_UTF8);


   //Frame2: Version History
   Evas_Object *version_frame = elm_frame_add(box);
   elm_frame_autocollapse_set(version_frame, EINA_TRUE);
   elm_frame_collapse_set(version_frame, EINA_TRUE);
   elm_object_text_set(version_frame, "Version History");
   evas_object_size_hint_weight_set(version_frame, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(version_frame, EVAS_HINT_FILL,
                                   0);
   evas_object_show(version_frame);
   elm_box_pack_end(box, version_frame);

   //Entry
   entry = elm_entry_add(version_frame);
   elm_entry_scrollable_set(entry, EINA_FALSE);
   elm_entry_line_wrap_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_FALSE);
   elm_entry_line_wrap_set(entry, ELM_WRAP_MIXED);
   elm_object_content_set(version_frame, entry);

   //Read File
   snprintf(buf, sizeof(buf), "%s/help/HISTORY", elm_app_data_dir_get());
   elm_entry_autosave_set(entry, EINA_FALSE);
   elm_entry_file_set(entry, buf, ELM_TEXT_FORMAT_MARKUP_UTF8);


   //Frame3: Shortcut Keys
   Evas_Object *key_frame = elm_frame_add(box);
   elm_frame_autocollapse_set(key_frame, EINA_TRUE);
   elm_frame_collapse_set(key_frame, EINA_TRUE);
   elm_object_text_set(key_frame, "Shortcut keys");
   evas_object_size_hint_weight_set(key_frame, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(key_frame, EVAS_HINT_FILL, 0);
   evas_object_show(key_frame);
   elm_box_pack_end(box, key_frame);

   //Entry
   entry = elm_entry_add(key_frame);
   elm_entry_scrollable_set(entry, EINA_FALSE);
   elm_entry_line_wrap_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_FALSE);
   elm_entry_line_wrap_set(entry, ELM_WRAP_MIXED);
   elm_object_content_set(key_frame, entry);

   //Read File
   snprintf(buf, sizeof(buf), "%s/help/SHORTCUT", elm_app_data_dir_get());
   elm_entry_autosave_set(entry, EINA_FALSE);
   elm_entry_file_set(entry, buf, ELM_TEXT_FORMAT_MARKUP_UTF8);


   //Frame4: Commandline
   Evas_Object *command_frame = elm_frame_add(box);
   elm_frame_autocollapse_set(command_frame, EINA_TRUE);
   elm_frame_collapse_set(command_frame, EINA_TRUE);
   elm_object_text_set(command_frame, "Commandline Usage");
   evas_object_size_hint_weight_set(command_frame, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(command_frame, EVAS_HINT_FILL, 0);
   evas_object_show(command_frame);
   elm_box_pack_end(box, command_frame);

   //Entry
   entry = elm_entry_add(command_frame);
   elm_entry_scrollable_set(entry, EINA_FALSE);
   elm_entry_line_wrap_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_FALSE);
   elm_entry_line_wrap_set(entry, ELM_WRAP_MIXED);
   elm_object_content_set(command_frame, entry);

   //Read File
   snprintf(buf, sizeof(buf), "%s/help/COMMAND", elm_app_data_dir_get());
   elm_entry_autosave_set(entry, EINA_FALSE);
   elm_entry_file_set(entry, buf, ELM_TEXT_FORMAT_MARKUP_UTF8);

   //Frame5: EDC References
   Evas_Object *refer_frame = elm_frame_add(box);
   elm_object_text_set(refer_frame, "EDC References (Not support yet)");
   evas_object_size_hint_weight_set(refer_frame, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(refer_frame, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   evas_object_show(refer_frame);
   elm_box_pack_end(box, refer_frame);

   //Frame6:Developers
   Evas_Object *devel_frame = elm_frame_add(box);
   elm_frame_autocollapse_set(devel_frame, EINA_TRUE);
   elm_frame_collapse_set(devel_frame, EINA_TRUE);
   elm_object_text_set(devel_frame, "Developers");
   evas_object_size_hint_weight_set(devel_frame, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(devel_frame, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   evas_object_show(devel_frame);
   elm_box_pack_end(box, devel_frame);

   //Entry
   entry = elm_entry_add(devel_frame);
   elm_entry_scrollable_set(entry, EINA_FALSE);
   elm_entry_line_wrap_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_FALSE);
   elm_entry_line_wrap_set(entry, ELM_WRAP_MIXED);
   elm_object_content_set(devel_frame, entry);

   //Read File
   snprintf(buf, sizeof(buf), "%s/help/DEVEL", elm_app_data_dir_get());
   elm_entry_autosave_set(entry, EINA_FALSE);
   elm_entry_file_set(entry, buf, ELM_TEXT_FORMAT_MARKUP_UTF8);


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

   //Save last state
   evas_object_geometry_get(win, NULL, NULL, &win_w, &win_h);
   elm_win_screen_position_get(win, &win_x, &win_y);
   evas_object_del(win);
   g_win = NULL;
}
