#include <Elementary.h>
#include "common.h"

typedef struct search_s
{
   Evas_Object *win;
   Evas_Object *en_find;
   Evas_Object *en_replace;
   Evas_Object *entry;
   int pos;
   Eina_Bool    need_iterate : 1;
} search_data;

static search_data *g_sd = NULL;
static Evas_Object *g_entry = NULL;
static Evas_Coord win_x = -1;
static Evas_Coord win_y = -1;
static Evas_Coord win_w = DEFAULT_SEARCH_WIN_W;
static Evas_Coord win_h = DEFAULT_SEARCH_WIN_H;

static void
win_delete_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   //search_data *sd = data;
   search_close();
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
find_word(Evas_Object *entry, const char *word, Eina_Bool *found)
{
   search_data *sd = g_sd;
   char buf[256];

   *found = EINA_FALSE;

   const char *text = elm_entry_entry_get(entry);
   const char *utf8 = elm_entry_markup_to_utf8(text);

   //get the character position begun with searching.
   if (sd->pos == -1)
      {
         sd->pos = elm_entry_cursor_pos_get(entry);
         sd->need_iterate = EINA_FALSE;
      }
   else sd->pos++;

   utf8 += sd->pos;

   char *s = strstr(utf8, word);

   //No found
   if (!s)
     {
        //Need to iterate to find again?
        if (sd->need_iterate)
          {

          }
        //There are no searched words in the text
        else
          {
             snprintf(buf, sizeof(buf), "No \"%s\" in the text", word);
             stats_info_msg_update(buf);
             return;
          }
     }

   //Got you!
   int len = strlen(word);
   sd->pos += (s - utf8);
   elm_entry_select_region_set(entry, sd->pos, sd->pos + len);
   *found = EINA_TRUE;
}

static void
find_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   search_data *sd = data;
   const char *find = elm_entry_entry_get(sd->en_find);
   if (!find) return;
   Eina_Bool found;
   find_word(sd->entry, find, &found);
}

void
search_open()
{
   if (g_sd) return;

   search_data *sd = calloc(1, sizeof(search_data));
   g_sd = sd;

   //Win
   Evas_Object *win = elm_win_add(base_win_get(), "Enventor Search",
                                  ELM_WIN_DIALOG_BASIC);
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   elm_win_title_set(win, "Find/Replace");
   evas_object_resize(win, win_w, win_h);
   evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb,
                                  sd);
   evas_object_smart_callback_add(win, "moved", win_moved_cb, sd);
   evas_object_show(win);

   //Bg
   Evas_Object *bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);
   elm_win_resize_object_add(win, bg);

   //Layout
   Evas_Object *layout = elm_layout_add(win);
   elm_layout_file_set(layout, EDJE_PATH, "search");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   elm_win_resize_object_add(win, layout);

   //Entry (find)
   Evas_Object *entry_find = elm_entry_add(layout);
   elm_entry_single_line_set(entry_find, EINA_TRUE);
   elm_entry_scrollable_set(entry_find, EINA_TRUE);
   evas_object_size_hint_weight_set(entry_find, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(entry_find, EVAS_HINT_FILL, 0);
   elm_object_focus_set(entry_find, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.find_entry", entry_find);

   //Entry (replace)
   Evas_Object *entry_replace = elm_entry_add(layout);
   elm_entry_single_line_set(entry_replace, EINA_TRUE);
   elm_entry_scrollable_set(entry_replace, EINA_TRUE);
   evas_object_size_hint_weight_set(entry_replace, EVAS_HINT_EXPAND,0);
   evas_object_size_hint_align_set(entry_replace, EVAS_HINT_FILL, 0);
   elm_object_part_content_set(layout, "elm.swallow.replace_entry",
                               entry_replace);
   //Button (find)
   Evas_Object *btn_find = elm_button_add(layout);
   elm_object_text_set(btn_find, "Find");
   evas_object_smart_callback_add(btn_find, "clicked", find_clicked_cb, sd);
   elm_object_part_content_set(layout, "elm.swallow.find", btn_find);

   //Button (find/replace)
   Evas_Object *btn_replace_find = elm_button_add(layout);
   elm_object_text_set(btn_replace_find, "Find/Replace");
   elm_object_disabled_set(btn_replace_find, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.replace/find",
                               btn_replace_find);

   //Button (replace)
   Evas_Object *btn_replace = elm_button_add(layout);
   elm_object_text_set(btn_replace, "Replace");
   elm_object_disabled_set(btn_replace, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.replace", btn_replace);

   //Button (replace all)
   Evas_Object *btn_replace_all = elm_button_add(layout);
   elm_object_text_set(btn_replace_all, "Replace All");
   elm_object_disabled_set(btn_replace_all, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.replace_all",
                               btn_replace_all);
   sd->win = win;
   sd->en_find = entry_find;
   sd->en_replace = entry_replace;
   sd->entry = g_entry;
   sd->pos = -1;
   sd->need_iterate = EINA_TRUE;
}

Eina_Bool
search_is_opened()
{
   search_data *sd = g_sd;
   return (sd ? EINA_TRUE : EINA_FALSE);
}

void
search_entry_register(Evas_Object *entry)
{
   g_entry = entry;
}

void
search_close()
{
   search_data *sd = g_sd;
   if (!sd) return;
   //Save last state
   evas_object_geometry_get(sd->win, NULL, NULL, &win_w, &win_h);
   elm_win_screen_position_get(sd->win, &win_x, &win_y);
   evas_object_del(sd->win);
   free(sd);
   g_sd = NULL;
}
