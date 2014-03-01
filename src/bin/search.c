#include <Elementary.h>
#include "common.h"

typedef struct search_s
{
   Evas_Object *win;
   int order;
} search_data;

static search_data *g_sd = NULL;
static Evas_Coord win_x = -1;
static Evas_Coord win_y = -1;
static Evas_Coord win_w = 285;
static Evas_Coord win_h = 90;

static void
win_delete_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   //search_data *sd = data;
   search_close();
}

static void
win_moved_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   /* Move the window with the previous remembered position when the window is
      moved by window manager first time. */
   if ((win_x != -1) || (win_y != -1)) evas_object_move(obj, win_x, win_y);
   evas_object_smart_callback_del(obj, "moved", win_moved_cb);
}

void
search_open()
{
   if (g_sd) return;

   search_data *sd = calloc(1, sizeof(search_data));
   g_sd = sd;

   //Win
   Evas_Object *win = elm_win_add(base_win_get(), "Enventor Search",
                                  ELM_WIN_UTILITY);
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
   evas_object_show(entry_find);
   elm_object_part_content_set(layout, "elm.swallow.find_entry", entry_find);

   //Entry (replace)
   Evas_Object *entry_replace = elm_entry_add(layout);
   elm_entry_single_line_set(entry_replace, EINA_TRUE);
   elm_entry_scrollable_set(entry_replace, EINA_TRUE);
   evas_object_size_hint_weight_set(entry_replace, EVAS_HINT_EXPAND,0);
   evas_object_size_hint_align_set(entry_replace, EVAS_HINT_FILL, 0);
   evas_object_show(entry_replace);
   elm_object_part_content_set(layout, "elm.swallow.replace_entry",
                               entry_replace);
   //Button (find)
   Evas_Object *btn_find = elm_button_add(layout);
   elm_object_text_set(btn_find, "Find");
   evas_object_show(btn_find);
   elm_object_part_content_set(layout, "elm.swallow.find", btn_find);

   //Button (find/replace)
   Evas_Object *btn_replace_find = elm_button_add(layout);
   elm_object_text_set(btn_replace_find, "Find/Replace");
   evas_object_show(btn_replace_find);
   elm_object_part_content_set(layout, "elm.swallow.replace/find",
                               btn_replace_find);

   //Button (replace)
   Evas_Object *btn_replace = elm_button_add(layout);
   elm_object_text_set(btn_replace, "Replace");
   evas_object_show(btn_replace);
   elm_object_part_content_set(layout, "elm.swallow.replace", btn_replace);

   sd->win = win;


   sd->win = win;
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

void
search_word(Evas_Object *entry, const char *word, Eina_Bool *found)
{
   search_data *sd = g_sd;

   *found = EINA_FALSE;

   if (!word) return;

   const char *text = elm_entry_entry_get(entry);
   const char *utf8 = elm_entry_markup_to_utf8(text);

   //There is no word in the text
   char *s = strstr(utf8, word);
   if (!s)
     {
        free(sd);
        return;
     }

   int order = sd->order;

   //No more next word found
   if ((order > 0) && (strlen(s) <= 1)) return;

   while (order > 0)
     {
        s++;
        s = strstr(s, word);
        if (!s) return;
        order--;
     }

   //Got you!
   int len = strlen(word);
   elm_entry_select_region_set(entry, (s - utf8), (s - utf8) + len);
   sd->order++;
   *found = EINA_TRUE;

   return;
}
