#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Elementary_Cursor.h>
#include "common.h"

typedef struct ctxpopup_it_data_s
{
   const char *name;
   int type;
} ctxpopup_it_data;

typedef struct live_editor_s
{
   Evas_Object *ctxpopup;
   Evas_Object *layout;
   Evas_Object *enventor;
   Evas_Object *trigger;

   struct {
      unsigned int type;
      float rel1_x, rel1_y;
      float rel2_x, rel2_y;
      Evas_Coord x, y, w, h;
   } cur_part_data;

   Ecore_Event_Handler *key_down_handler;

   Eina_Bool on : 1;
} live_data;

static const ctxpopup_it_data CTXPOPUP_ITEMS[] =
{
     {"RECT", EDJE_PART_TYPE_RECTANGLE},
     {"TEXT", EDJE_PART_TYPE_TEXT},
     {"IMAGE", EDJE_PART_TYPE_IMAGE},
     {"SWALLOW", EDJE_PART_TYPE_SWALLOW},
     {"TEXTBLOCK", EDJE_PART_TYPE_TEXTBLOCK},
     {"SPACER", EDJE_PART_TYPE_SPACER}
};

static live_data *g_ld = NULL;

#define LIVE_EDIT_NEW_PART_DATA_MAX_LEN 80
static const char *LIVE_EDIT_NEW_PART_DATA_STR =
                   "    %s<br/>"
                   "    X: %5d  Y: %5d<br/>"
                   "    W: %5d H: %5d";

#define LIVE_EDIT_NEW_PART_REL_STR_MAX_LEN 16
static const char *LIVE_EDIT_NEW_PART_REL_STR = "%.2f %.2f";

static void
cur_part_value_update(live_data *ld, Evas_Object *edje)
{
   Evas_Coord x, y, w, h;
   Evas_Coord view_w, view_h;

   config_view_size_get(&view_w, &view_h);
   edje_object_part_geometry_get(edje, "elm.swallow.symbol", &x, &y, &w, &h);

   ld->cur_part_data.rel1_x = ((float) x) / ((float) view_w);
   ld->cur_part_data.rel1_y = ((float) y) / ((float) view_h);
   ld->cur_part_data.rel2_x = ((float) (x + w)) / ((float) view_w);
   ld->cur_part_data.rel2_y = ((float) (y + h)) / ((float) view_h);
   ld->cur_part_data.x = x;
   ld->cur_part_data.y = y;
   ld->cur_part_data.w = w;
   ld->cur_part_data.h = h;
}

static void
part_info_update(live_data *ld)
{
   Evas_Object *layout = elm_layout_edje_get(ld->layout);

   cur_part_value_update(ld, layout);

   char part_info[LIVE_EDIT_NEW_PART_DATA_MAX_LEN];

   snprintf(part_info,
            LIVE_EDIT_NEW_PART_DATA_MAX_LEN, LIVE_EDIT_NEW_PART_DATA_STR,
            CTXPOPUP_ITEMS[ld->cur_part_data.type].name,
            ld->cur_part_data.x, ld->cur_part_data.y,
            ld->cur_part_data.w, ld->cur_part_data.h);
//   edje_object_part_text_set(layout,
//                             "elm.text.info", part_info);
   snprintf(part_info,
            LIVE_EDIT_NEW_PART_REL_STR_MAX_LEN, LIVE_EDIT_NEW_PART_REL_STR,
            ld->cur_part_data.rel1_x, ld->cur_part_data.rel1_y);
   edje_object_part_text_set(layout, "elm.text.rel1", part_info);
   snprintf(part_info,
            LIVE_EDIT_NEW_PART_REL_STR_MAX_LEN, LIVE_EDIT_NEW_PART_REL_STR,
            ld->cur_part_data.rel2_x, ld->cur_part_data.rel2_y);
   edje_object_part_text_set(layout, "elm.text.rel2", part_info);
}

static void
dragable_geometry_changed_cb(void *data,
                                   Evas_Object *obj EINA_UNUSED,
                                   const char *emission EINA_UNUSED,
                                   const char *source EINA_UNUSED)
{
   //TODO: recalc on viewport size changed
   live_data *ld = data;
   part_info_update(ld);
}

static void
new_part_mouse_move_cb(void *data, Evas *e EINA_UNUSED,
                 Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   live_data *ld = data;

   Evas_Coord view_w, view_h;
   config_view_size_get(&view_w, &view_h);
   int cur_x = ld->cur_part_data.x + ev->cur.canvas.x - ev->prev.canvas.x;
   int cur_y = ld->cur_part_data.y + ev->cur.canvas.y - ev->prev.canvas.y;

   if ((cur_x >= 0) && (cur_y >= 0) &&
       (cur_x <= view_w - ld->cur_part_data.w) &&
       (cur_y <= view_h - ld->cur_part_data.h))
     {
        double dx = ((float) cur_x / (float) view_w) -
           ld->cur_part_data.rel1_x;
        double dy = ((float) cur_y / (float) view_h) -
           ld->cur_part_data.rel1_y;
        edje_object_part_drag_step(elm_layout_edje_get(ld->layout),
                                   "rel1_dragable", dx, dy);
        edje_object_part_drag_step(elm_layout_edje_get(ld->layout),
                                   "rel2_dragable", dx, dy);
        part_info_update(ld);
     }
}

static void
new_part_mouse_down_cb(void *data,
                       Evas_Object *obj,
                       const char *emission EINA_UNUSED,
                       const char *source EINA_UNUSED)
{
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                  new_part_mouse_move_cb, data);
}

static void
new_part_mouse_up_cb(void *data EINA_UNUSED,
                      Evas_Object *obj,
                      const char *emission EINA_UNUSED,
                      const char *source EINA_UNUSED)
{
   evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                  new_part_mouse_move_cb);
}

static void
symbol_set(live_data *ld)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s_bg",
            CTXPOPUP_ITEMS[ld->cur_part_data.type].name);
   Evas_Object *bg_layout = elm_layout_add(ld->layout);
   elm_layout_file_set(bg_layout, EDJE_PATH, buf);
   elm_object_part_content_set(ld->layout, "elm.swallow.symbol", bg_layout);
}

static void
live_edit_reset(live_data *ld)
{
   if (ld->ctxpopup) elm_ctxpopup_dismiss(ld->ctxpopup);

   ecore_event_handler_del(ld->key_down_handler);
   ld->key_down_handler = NULL;

   evas_object_del(ld->layout);
   ld->layout = NULL;
}

static Eina_Bool
key_down_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   live_data *ld = data;

   if (!strcmp(event->key, "Return"))
     {
        enventor_object_template_part_insert(ld->enventor,
                                             CTXPOPUP_ITEMS[ld->cur_part_data.type].type,
                                             ENVENTOR_TEMPLATE_INSERT_LIVE_EDIT,
                                             ld->cur_part_data.rel1_x,
                                             ld->cur_part_data.rel1_y,
                                             ld->cur_part_data.rel2_x,
                                             ld->cur_part_data.rel2_y,
                                             NULL, 0);
        enventor_object_save(ld->enventor, config_edc_path_get());
     }
   else if (strcmp(event->key, "Delete")) return EINA_TRUE;

   live_edit_reset(ld);
   return EINA_TRUE;
}

static void
live_edit_layer_set(live_data *ld)
{
   ld->key_down_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                                  key_down_cb,
                                                  ld);
   //Create Live View Layout
   Evas_Object *live_view = enventor_object_live_view_get(ld->enventor);
   Evas_Object *layout = elm_layout_add(live_view);
   elm_layout_file_set(layout, EDJE_PATH,  "live_edit_layout");
   elm_object_part_content_set(live_view, "elm.swallow.live_edit", layout);

   edje_object_signal_callback_add(elm_layout_edje_get(layout),
                                   "drag", "rel1_dragable",
                                   dragable_geometry_changed_cb, ld);
   edje_object_signal_callback_add(elm_layout_edje_get(layout),
                                   "drag", "rel2_dragable",
                                   dragable_geometry_changed_cb, ld);
   edje_object_signal_callback_add(elm_layout_edje_get(layout),
                                   "mouse,down,1", "elm.swallow.symbol",
                                   new_part_mouse_down_cb, ld);
   edje_object_signal_callback_add(elm_layout_edje_get(layout),
                                   "mouse,up,1", "elm.swallow.symbol",
                                   new_part_mouse_up_cb, ld);
   elm_layout_part_cursor_set(layout, "elm.swallow.symbol",
                              ELM_CURSOR_FLEUR);
   elm_layout_part_cursor_set(layout, "rel1_dragable",
                              ELM_CURSOR_TOP_LEFT_CORNER);
   elm_layout_part_cursor_set(layout, "rel2_dragable",
                              ELM_CURSOR_BOTTOM_RIGHT_CORNER);
   ld->layout = layout;
   symbol_set(ld);
   part_info_update(ld);
}

static void
ctxpopup_it_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   live_data *ld = g_ld;
   const Elm_Object_Item *it = event_info;
   ld->cur_part_data.type = (unsigned int) data;
   live_edit_layer_set(ld);
   elm_ctxpopup_dismiss(obj);

   stats_info_msg_update("Click and drag the mouse in the Live View to insert "
                         "the part.");
}

static void
ctxpopup_dismissed_cb(void *data, Evas_Object *obj,
                      void *event_info EINA_UNUSED)
{
   live_data *ld = data;
   if (!ld->layout) live_edit_cancel();
   evas_object_del(obj);
   ld->ctxpopup = NULL;
}

static Evas_Object *
ctxpopup_create(Evas_Object *parent, live_data *ld)
{
   const int CTXPOPUP_ITEMS_NUM = 6;
   int i;
   Evas_Object *ctxpopup = elm_ctxpopup_add(parent);
   elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_DOWN,
                                       ELM_CTXPOPUP_DIRECTION_RIGHT,
                                       ELM_CTXPOPUP_DIRECTION_LEFT,
                                       ELM_CTXPOPUP_DIRECTION_UP);

   for (i = 0; i < CTXPOPUP_ITEMS_NUM; i++)
     {
        Evas_Object *icon = elm_image_add(ctxpopup);
        elm_image_file_set(icon, EDJE_PATH, CTXPOPUP_ITEMS[i].name);
        elm_ctxpopup_item_append(ctxpopup, CTXPOPUP_ITEMS[i].name, icon,
                                 ctxpopup_it_selected_cb, (void *)i);
     }

   evas_object_smart_callback_add(ctxpopup, "dismissed", ctxpopup_dismissed_cb,
                                  ld);
   Evas_Coord x, y, w, h;
   evas_object_geometry_get(ld->trigger, &x, &y, &w, &h);
   evas_object_move(ctxpopup, (x + (w/2)), (y + h));
   evas_object_show(ctxpopup);

   return ctxpopup;
}

void
live_edit_toggle(void)
{
   live_data *ld = g_ld;
   ld->on = !ld->on;

   Evas_Object *live_view = enventor_object_live_view_get(ld->enventor);
   if (!live_view) return;

   enventor_object_disabled_set(ld->enventor, ld->on);

   if (ld->on)
     {
        ld->ctxpopup = ctxpopup_create(live_view, ld);
        stats_info_msg_update("Select a part to add in Live View.");
     }
   else
     {
        live_edit_reset(ld);
        stats_info_msg_update("Live View Edit Mode Disabled.");
     }
}

Eina_Bool
live_edit_get(void)
{
   live_data *ld = g_ld;
   return ld->on;
}

void
live_edit_cancel(void)
{
   live_data *ld = g_ld;
   if (!ld->on) return;
   live_edit_toggle();
}

void
live_edit_init(Evas_Object *enventor, Evas_Object *trigger)
{
   live_data *ld = calloc(1, sizeof(live_data));
   if (!ld)
     {
        EINA_LOG_ERR("Faild to allocate Memory!");
        return;
     }
   g_ld = ld;
   ld->enventor = enventor;
   ld->trigger = trigger;
}

void
live_edit_term(void)
{
   live_data *ld = g_ld;
   evas_object_del(ld->ctxpopup);
   live_edit_reset(ld);
   free(ld);
   g_ld = NULL;
}
