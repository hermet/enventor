#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

typedef struct search_s
{
   Evas_Object *win;
   Evas_Object *layout;
   Evas_Object *en_find;
   Evas_Object *en_replace;
   int pos;
   int len;
   int syntax_color;
   Enventor_Item *it;
   Eina_Bool forward : 1;
} search_data;

static search_data *g_sd = NULL;
static Evas_Coord win_x = -1;
static Evas_Coord win_y = -1;
static Evas_Coord win_w = DEFAULT_SEARCH_WIN_W;
static Evas_Coord win_h = DEFAULT_SEARCH_WIN_H;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
syntax_color_context_clear(search_data *sd)
{
   enventor_item_select_none(sd->it);
   while (sd->syntax_color > 0)
     {
        enventor_item_syntax_color_partial_apply(sd->it, -1);
        sd->syntax_color--;
     }
}

static void
win_delete_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
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
replace_all_proc(search_data *sd)
{
   const char *find = elm_entry_entry_get(sd->en_find);
   if (!find) return;
   int find_len = strlen(find);

   const char *replace = elm_entry_entry_get(sd->en_replace);
   int replace_len = 0;

   //Same word. no need to replace
   if (replace)
     {
        replace_len = strlen(replace);
        if (!strcmp(find, replace) && (find_len == replace_len))
          return;
     }

   char buf[256];
   int replace_cnt = 0;

   const char *text = enventor_item_text_get(sd->it);
   char *utf8 = elm_entry_markup_to_utf8(text);

   char *s = utf8;
   int pos;
   int delta = replace_len - find_len;

   while ((s = strstr(s, find)))
     {
        pos = s + (delta * replace_cnt) - utf8;
        enventor_item_select_region_set(sd->it, pos, (pos + find_len));
        enventor_item_text_insert(sd->it, replace);
        enventor_item_select_none(sd->it);
        replace_cnt++;
        s++;
     }

   snprintf(buf, sizeof(buf), _("%d matches replaced"), replace_cnt);
   stats_info_msg_update(buf);

   free(utf8);
}

/* FIXME: selection_region_set() won't be worked all just right after entry
   changes, no idea why? so use the animator. */
static Eina_Bool
selection_region_anim_cb(void *data)
{
   search_data *sd = data;
   enventor_item_select_region_set(sd->it, sd->pos,
                                   (sd->pos + sd->len));

   //Move search position to the end of the word if search type is forward
   if (sd->forward)
     sd->pos += sd->len;

   return ECORE_CALLBACK_CANCEL;
}

static void
find_forward_proc(search_data *sd)
{
   const char *find = elm_entry_entry_get(sd->en_find);
   if (!find) return;

   char buf[256];
   Eina_Bool need_iterate = EINA_TRUE;

   const char *text = enventor_item_text_get(sd->it);
   if (!text) return;
   char *utf8 = elm_entry_markup_to_utf8(text);

   //get the character position begun with searching.
   if (sd->pos == -1) sd->pos = enventor_item_cursor_pos_get(sd->it);
   else if (sd->pos == 0) need_iterate = EINA_FALSE;
   else sd->pos++;

   char *s = strstr((utf8 + sd->pos), find);

   //No found
   if (!s)
     {
        //Need to iterate finding?
        if (need_iterate)
          {
             sd->pos = 0;
             find_forward_proc(sd);
          }
        //There are no searched words in the text
        else
          {
             snprintf(buf, sizeof(buf), _("No \"%s\" in the text"), find);
             stats_info_msg_update(buf);
             sd->pos = -1;
          }
        free(utf8);
        return;
     }

   //Got you!
   sd->len = strlen(find);
   sd->pos = s - utf8;
   ecore_animator_add(selection_region_anim_cb, sd);
   free(utf8);
}

static void
find_backward_proc(search_data *sd)
{
   const char *find = elm_entry_entry_get(sd->en_find);
   if (!find) return;

   char buf[256];
   Eina_Bool need_iterate = EINA_TRUE;
   int len = 0;

   const char *text = enventor_item_text_get(sd->it);
   if (!text) return;
   char *utf8 = elm_entry_markup_to_utf8(text);

   //get the character position begun with searching.
   if (sd->pos == -1)
      {
        sd->pos = enventor_item_cursor_pos_get(sd->it);
      }
   else
      {
         len = strlen(utf8);
         if (sd->pos == len) need_iterate = EINA_FALSE;
      }

   char *prev = NULL;
   char *s = utf8;

   while ((s = strstr(s, find)))
     {
        if ((s - utf8) >= sd->pos) break;
        prev = s;
        s++;
     }

   //No found. Need to iterate finding?
   if (!prev)
     {
        if (need_iterate)
          {
             sd->pos = len;
             find_backward_proc(sd);
          }
        else
          {
             snprintf(buf, sizeof(buf), _("No \"%s\" in the text"), find);
             stats_info_msg_update(buf);
             sd->pos = -1;
          }
        free(utf8);
        return;
     }

   //Got you!
   sd->pos = prev - utf8;
   sd->len = strlen(find);
   ecore_animator_add(selection_region_anim_cb, sd);

   free(utf8);
}

static Eina_Bool
replace_proc(search_data *sd)
{
   const char *find = elm_entry_entry_get(sd->en_find);
   const char *selection = enventor_item_selection_get(sd->it);
   if (!find || !selection) return EINA_FALSE;
   char *utf8 = elm_entry_markup_to_utf8(selection);
   if (strcmp(find, utf8)) return EINA_FALSE;
   const char *replace = elm_entry_entry_get(sd->en_replace);
   enventor_item_text_insert(sd->it, replace);
   enventor_item_select_none(sd->it);
   free(utf8);
   return EINA_TRUE;
}

static void
backward_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   search_data *sd = data;
   find_backward_proc(sd);
   sd->forward = EINA_FALSE;
   elm_object_part_text_set(sd->layout, "elm.text.dir", _("Previous"));
}

static void
replace_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   search_data *sd = data;
   Eina_Bool next;
   next = replace_proc(sd);
   enventor_item_syntax_color_full_apply(sd->it, EINA_TRUE);
   if (!next) return;
   if (sd->forward) find_forward_proc(sd);
   else find_backward_proc(sd);
}

static void
replace_all_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   search_data *sd = data;
   replace_all_proc(sd);
   enventor_item_syntax_color_full_apply(sd->it, EINA_TRUE);
}

static void
forward_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   search_data *sd = data;
   find_forward_proc(sd);
   sd->forward = EINA_TRUE;
   elm_object_part_text_set(sd->layout, "elm.text.dir", _("Next"));
}

static void
find_activated_cb(void *data, Evas_Object *obj EINA_UNUSED,
                  void* event_info EINA_UNUSED)
{
   search_data *sd = data;
   if (sd->forward) find_forward_proc(sd);
   else find_backward_proc(sd);
}

static void
replace_activated_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void* event_info EINA_UNUSED)
{
   search_data *sd = data;
   Eina_Bool next;
   next = replace_proc(sd);
   enventor_item_syntax_color_full_apply(sd->it, EINA_TRUE);
   if (!next) return;
   if (sd->forward) find_forward_proc(sd);
   else find_backward_proc(sd);
}

static void
win_focused_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   search_data *sd = g_sd;
   enventor_item_syntax_color_full_apply(sd->it, EINA_FALSE);
   sd->syntax_color++;
   /* FIXME: reset position because search requests syntax color partial apply
      when it's window is unfocused. the selection region would be dismissed.
      we can remove this once selection region could be recovered just right
      after syntax color is applied. */
   sd->pos = -1;
}

static void
win_unfocused_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   search_data *sd = g_sd;
   enventor_item_syntax_color_partial_apply(sd->it, -1);
   sd->syntax_color--;
}

static void
keygrabber_key_down_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                       Evas_Object *obj  EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   search_close();
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
search_open(void)
{
   search_data *sd = g_sd;

   if (sd)
   {
      elm_win_activate(sd->win);
      return;
   }

   goto_close();

   Enventor_Item *it = file_mgr_focused_item_get();

   sd = calloc(1, sizeof(search_data));
   if (!sd)
     {
        mem_fail_msg();
        return;
     }
   g_sd = sd;

   //Win
   Evas_Object *win = elm_win_add(base_win_get(), _("Enventor Search"),
                                  ELM_WIN_DIALOG_BASIC);
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   elm_win_title_set(win, _("Find/Replace"));
   win_w = (Evas_Coord) ((double) win_w * elm_config_scale_get());
   win_h = (Evas_Coord) ((double) win_h * elm_config_scale_get());
   evas_object_resize(win, win_w, win_h);
   evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb,
                                  sd);
   evas_object_smart_callback_add(win, "moved", win_moved_cb, sd);
   evas_object_smart_callback_add(win, "focused", win_focused_cb, sd);
   evas_object_smart_callback_add(win, "unfocused", win_unfocused_cb, sd);

   //Bg
   Evas_Object *bg = elm_bg_add(win);
   evas_object_show(bg);
   elm_win_resize_object_add(win, bg);

   //Layout
   Evas_Object *layout = elm_layout_add(win);
   elm_layout_file_set(layout, EDJE_PATH, "search_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   elm_win_resize_object_add(win, layout);

   //Entry (find)
   Evas_Object *entry_find = elm_entry_add(layout);
   elm_entry_single_line_set(entry_find, EINA_TRUE);
   elm_entry_scrollable_set(entry_find, EINA_TRUE);
   evas_object_smart_callback_add(entry_find, "activated", find_activated_cb,
                                  sd);
   evas_object_size_hint_weight_set(entry_find, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(entry_find, EVAS_HINT_FILL, 0);
   evas_object_show(entry_find);
   elm_object_focus_set(entry_find, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.find_entry", entry_find);

   //Entry (replace)
   Evas_Object *entry_replace = elm_entry_add(layout);
   elm_entry_single_line_set(entry_replace, EINA_TRUE);
   elm_entry_scrollable_set(entry_replace, EINA_TRUE);
   evas_object_smart_callback_add(entry_replace, "activated",
                                  replace_activated_cb, sd);
   evas_object_size_hint_weight_set(entry_replace, EVAS_HINT_EXPAND,0);
   evas_object_size_hint_align_set(entry_replace, EVAS_HINT_FILL, 0);
   elm_object_part_content_set(layout, "elm.swallow.replace_entry",
                               entry_replace);
   //Button (backward)
   Evas_Object *btn_backward = elm_button_add(layout);
   elm_object_text_set(btn_backward, _("Previous"));
   evas_object_smart_callback_add(btn_backward, "clicked",
                                  backward_clicked_cb, sd);
   elm_object_part_content_set(layout, "elm.swallow.backward",
                               btn_backward);
   //Button (forward)
   Evas_Object *btn_forward = elm_button_add(layout);
   elm_object_text_set(btn_forward, _("Next"));
   evas_object_smart_callback_add(btn_forward, "clicked", forward_clicked_cb,
                                  sd);
   elm_object_part_content_set(layout, "elm.swallow.forward", btn_forward);

   //Button (replace)
   Evas_Object *btn_replace = elm_button_add(layout);
   elm_object_text_set(btn_replace, _("Replace"));
   evas_object_smart_callback_add(btn_replace, "clicked",
                                  replace_clicked_cb, sd);
   elm_object_part_content_set(layout, "elm.swallow.replace", btn_replace);

   //Button (replace all)
   Evas_Object *btn_replace_all = elm_button_add(layout);
   elm_object_text_set(btn_replace_all, _("Replace All"));
   evas_object_smart_callback_add(btn_replace_all, "clicked",
                                  replace_all_clicked_cb, sd);
   elm_object_part_content_set(layout, "elm.swallow.replace_all",
                               btn_replace_all);
   evas_object_show(win);

   tools_search_update();

   //Keygrabber
   Evas_Object *keygrabber =
      evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_event_callback_add(keygrabber, EVAS_CALLBACK_KEY_DOWN,
                                  keygrabber_key_down_cb, sd);
   if (!evas_object_key_grab(keygrabber, "Escape", 0, 0, EINA_TRUE))
     EINA_LOG_ERR(_("Failed to grab key - Escape"));

   sd->win = win;
   sd->layout = layout;
   sd->en_find = entry_find;
   sd->en_replace = entry_replace;
   sd->pos = -1;
   sd->forward = EINA_TRUE;
   sd->it = it;
}

Eina_Bool
search_is_opened(void)
{
   search_data *sd = g_sd;
   return (sd ? EINA_TRUE : EINA_FALSE);
}

Eina_Bool
search_close(void)
{
   search_data *sd = g_sd;
   if (!sd) return EINA_FALSE;

   syntax_color_context_clear(sd);

   //Save last state
   evas_object_geometry_get(sd->win, NULL, NULL, &win_w, &win_h);
   elm_win_screen_position_get(sd->win, &win_x, &win_y);
   evas_object_del(sd->win);
   free(sd);
   g_sd = NULL;

   tools_search_update();

   return EINA_TRUE;
}

void
search_reset(void)
{
   search_data *sd = g_sd;
   if (!sd) return;

   syntax_color_context_clear(sd);

   sd->it = file_mgr_focused_item_get();
   sd->pos = -1;
   sd->forward = EINA_TRUE;
   sd->len = 0;
}
