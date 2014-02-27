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
static Evas_Coord win_w = 300;
static Evas_Coord win_h = 100;

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
   search_data *sd = data;
   search_close();
}

void
search_open()
{
   search_data *sd = calloc(1, sizeof(search_data));
   g_sd = sd;

   //Win
   Evas_Object *win = elm_win_add(base_win_get(), "Enventor Search",
                                  ELM_WIN_DIALOG_BASIC);
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   elm_win_title_set(win, "Find/Replace");
   //FIXME: doesn't moved
   if ((win_x == -1) && (win_y == -1)) evas_object_move(win, win_x, win_y);
   evas_object_resize(win, win_w, win_h);
   evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb,
                                  sd);
   evas_object_show(win);

   //BG
   Evas_Object *bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   elm_win_resize_object_add(win, bg);

   //Box
   Evas_Object *box = elm_box_add(win);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(box);
   elm_win_resize_object_add(win, box);

   //Box 2
   Evas_Object *box2 = elm_box_add(box);
   evas_object_size_hint_align_set(box2, 0, 0);
   evas_object_show(box2);
   elm_box_pack_end(box, box2);

   //Label (find)
   Evas_Object *label_find;
   label_find = elm_label_add(box2);
   evas_object_size_hint_align_set(label_find, 0, 0);
   elm_object_text_set(label_find, "Find:");
   evas_object_show(label_find);
   elm_box_pack_end(box2, label_find);

   //Label (find)
   Evas_Object *label_replace;
   label_replace = elm_label_add(box2);
   evas_object_size_hint_align_set(label_replace, 0, 0);
   elm_object_text_set(label_replace, "Replace with:");
   evas_object_show(label_replace);
   elm_box_pack_end(box2, label_replace);

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
   printf("%d %d\n", win_x, win_y);
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
