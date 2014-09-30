#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define ENVENTOR_BETA_API_SUPPORT 1

#include <Enventor.h>
#include "common.h"

typedef struct goto_s
{
   Evas_Object *win;
   Evas_Object *layout;
   Evas_Object *entry;
   Evas_Object *btn;
   Evas_Object *enventor;
} goto_data;

static goto_data *g_gd = NULL;
static Evas_Coord win_x = -1;
static Evas_Coord win_y = -1;
static Evas_Coord win_w = DEFAULT_GOTO_WIN_W;
static Evas_Coord win_h = DEFAULT_GOTO_WIN_H;

static void
win_delete_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   goto_close();
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
goto_line(goto_data *gd)
{
  const char *txt = elm_entry_entry_get(gd->entry);
  int line = atoi(txt);
  enventor_object_line_goto(gd->enventor, line);
  goto_close();
}

static void
entry_activated_cb(void *data, Evas_Object *obj EINA_UNUSED,
                   void* event_info EINA_UNUSED)
{
   goto_data *gd = data;
   if (elm_object_disabled_get(gd->btn)) return;
   goto_line(gd);
}

static void
entry_changed_cb(void *data, Evas_Object *obj, void* event_info EINA_UNUSED)
{
   goto_data *gd = data;
   const char *txt = elm_entry_entry_get(obj);
   if (txt[0] == 0) return;

   int line = atoi(txt);

   if ((line < 1) || (line > enventor_object_max_line_get(gd->enventor)))
     {
        elm_object_part_text_set(gd->layout, "elm.text.msg",
                                 "Invalid line number");
        elm_object_disabled_set(gd->btn, EINA_TRUE);
     }
   else
     {
        elm_object_part_text_set(gd->layout, "elm.text.msg", "");
        elm_object_disabled_set(gd->btn, EINA_FALSE);
     }
}

static void
btn_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   goto_data *gd = data;
   goto_line(gd);
}

void
goto_open(Evas_Object *enventor)
{
   static Elm_Entry_Filter_Accept_Set digits_filter_data;
   goto_data *gd = g_gd;

   if (gd)
   {
      elm_win_activate(gd->win);
      return;
   }

   search_close();

   gd = calloc(1, sizeof(goto_data));
   if (!gd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }
   g_gd = gd;

   //Win
   Evas_Object *win = elm_win_add(base_win_get(), "Enventor Goto Line",
                                  ELM_WIN_DIALOG_BASIC);
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);

   elm_win_title_set(win, "Go to Line");
   win_w = (Evas_Coord) ((double) win_w * elm_config_scale_get());
   win_h = (Evas_Coord) ((double) win_h * elm_config_scale_get());
   evas_object_resize(win, win_w, win_h);
   evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb,
                                  gd);
   evas_object_smart_callback_add(win, "moved", win_moved_cb, gd);

   //Bg
   Evas_Object *bg = elm_bg_add(win);
   evas_object_show(bg);
   elm_win_resize_object_add(win, bg);

   //Layout
   Evas_Object *layout = elm_layout_add(win);
   elm_layout_file_set(layout, EDJE_PATH, "goto_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);
   elm_win_resize_object_add(win, layout);

   char  buf[256];
   snprintf(buf, sizeof(buf), "Enter line number [1..%d]:",
            enventor_object_max_line_get(enventor));
   elm_object_part_text_set(layout, "elm.text.goto", buf);

   //Entry (line)
   Evas_Object *entry = elm_entry_add(layout);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_scrollable_set(entry, EINA_TRUE);

   digits_filter_data.accepted = "0123456789";
   digits_filter_data.rejected = NULL;
   elm_entry_markup_filter_append(entry, elm_entry_filter_accept_set,
                                  &digits_filter_data);
   evas_object_smart_callback_add(entry, "activated", entry_activated_cb,
                                  gd);
   evas_object_smart_callback_add(entry, "changed,user", entry_changed_cb, gd);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, 0);
   evas_object_show(entry);
   elm_object_focus_set(entry, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.entry", entry);

   //Button (ok)
   Evas_Object *btn = elm_button_add(layout);
   elm_object_text_set(btn, "Ok");
   evas_object_smart_callback_add(btn, "clicked", btn_clicked_cb, gd);
   elm_object_part_content_set(layout, "elm.swallow.btn",
                               btn);
   evas_object_show(win);

   gd->win = win;
   gd->layout = layout;
   gd->entry = entry;
   gd->btn = btn;
   gd->enventor = enventor;
}

Eina_Bool
goto_is_opened(void)
{
   goto_data *gd = g_gd;
   return (gd ? EINA_TRUE : EINA_FALSE);
}

void
goto_close(void)
{
   goto_data *gd = g_gd;

   if (!gd) return;
   //Save last state
   evas_object_geometry_get(gd->win, NULL, NULL, &win_w, &win_h);
   elm_win_screen_position_get(gd->win, &win_x, &win_y);
   evas_object_del(gd->win);
   free(gd);
   g_gd = NULL;
}
