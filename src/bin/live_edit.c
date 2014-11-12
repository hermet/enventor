#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define ENVENTOR_BETA_API_SUPPORT 1

#include <Elementary_Cursor.h>
#include <Enventor.h>
#include "common.h"

typedef struct menu_data_s
{
   const char *name;
   int type;
} menu_data;

typedef struct cur_part_data_s
{
   unsigned int type;
   float rel1_x, rel1_y, rel2_x, rel2_y;
   Evas_Coord x, y, w, h;
} cur_part_data;

typedef struct live_editor_s
{
   Evas_Object *menu;
   Evas_Object *layout;
   Evas_Object *enventor;
   cur_part_data *cur_part_data;

   Ecore_Event_Handler *key_down_handler;
} live_data;

const int MENU_ITEMS_NUM = 6;

static const menu_data MENU_ITEMS[] =
{
     {"RECT", EDJE_PART_TYPE_RECTANGLE},
     {"IMAGE", EDJE_PART_TYPE_IMAGE},
     {"SPACER", EDJE_PART_TYPE_SPACER},
     {"SWALLOW", EDJE_PART_TYPE_SWALLOW},
     {"TEXT", EDJE_PART_TYPE_TEXT},
     {"TEXTBLOCK", EDJE_PART_TYPE_TEXTBLOCK}
};

static live_data *g_ld = NULL;

#define LIVE_EDIT_NEW_PART_DATA_MAX_LEN 80
static const char *LIVE_EDIT_NEW_PART_DATA_STR =
                   "    %s<br/>"
                   "    X: %5d  Y: %5d<br/>"
                   "    W: %5d H: %5d";

#define LIVE_EDIT_NEW_PART_REL_STR_MAX_LEN 16
static const char *LIVE_EDIT_NEW_PART_REL_STR = "   %.2f %.2f";

static void
cur_part_value_update(live_data *ld, Evas_Object *edje)
{
   Evas_Coord x, y, w, h;
   Evas_Coord view_w, view_h;

   config_view_size_get(&view_w, &view_h);
   edje_object_part_geometry_get(edje, "elm.swallow.symbol", &x, &y, &w, &h);

   ld->cur_part_data->rel1_x = ((float) x) / ((float) view_w);
   ld->cur_part_data->rel1_y = ((float) y) / ((float) view_h);
   ld->cur_part_data->rel2_x = ((float) (x + w)) / ((float) view_w);
   ld->cur_part_data->rel2_y = ((float) (y + h)) / ((float) view_h);
   ld->cur_part_data->x = x;
   ld->cur_part_data->y = y;
   ld->cur_part_data->w = w;
   ld->cur_part_data->h = h;
}

static Evas_Object *
create_live_edit_layout(live_data *ld)
{
   Evas_Object *live_view = enventor_object_live_view_get(ld->enventor);
   Evas_Object *layout = elm_layout_add(live_view);
   elm_layout_file_set(layout, EDJE_PATH,  "live_edit_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(live_view, "elm.swallow.live_edit", layout);
   return layout;
}

static void
part_info_update(live_data *ld)
{
   Evas_Object *layout = elm_layout_edje_get(ld->layout);

   cur_part_value_update(ld, layout);

   char part_info[LIVE_EDIT_NEW_PART_DATA_MAX_LEN];

   snprintf(part_info,
            LIVE_EDIT_NEW_PART_DATA_MAX_LEN, LIVE_EDIT_NEW_PART_DATA_STR,
            MENU_ITEMS[ld->cur_part_data->type].name,
            ld->cur_part_data->x, ld->cur_part_data->y,
            ld->cur_part_data->w, ld->cur_part_data->h);
   edje_object_part_text_set(layout,
                             "elm.text.info", part_info);
   snprintf(part_info,
            LIVE_EDIT_NEW_PART_REL_STR_MAX_LEN, LIVE_EDIT_NEW_PART_REL_STR,
            ld->cur_part_data->rel1_x, ld->cur_part_data->rel1_y);
   edje_object_part_text_set(layout, "elm.text.rel1", part_info);
   snprintf(part_info,
            LIVE_EDIT_NEW_PART_REL_STR_MAX_LEN, LIVE_EDIT_NEW_PART_REL_STR,
            ld->cur_part_data->rel2_x, ld->cur_part_data->rel2_y);
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
   int cur_x = ld->cur_part_data->x + ev->cur.canvas.x - ev->prev.canvas.x;
   int cur_y = ld->cur_part_data->y + ev->cur.canvas.y - ev->prev.canvas.y;

   if ((cur_x >= 0) && (cur_y >= 0) &&
       (cur_x <= view_w - ld->cur_part_data->w) &&
       (cur_y <= view_h - ld->cur_part_data->h))
     {
        double dx = ((float) cur_x / (float) view_w) -
           ld->cur_part_data->rel1_x;
        double dy = ((float) cur_y / (float) view_h) -
           ld->cur_part_data->rel1_y;
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
            MENU_ITEMS[ld->cur_part_data->type].name);
   Evas_Object *bg_layout = elm_layout_add(ld->layout);
   elm_layout_file_set(bg_layout, EDJE_PATH, buf);
   elm_object_part_content_set(ld->layout, "elm.swallow.symbol", bg_layout);
}

static void
live_edit_reset(live_data *ld)
{
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
                                             MENU_ITEMS[ld->cur_part_data->type].type,
                                             ENVENTOR_TEMPLATE_INSERT_LIVE_EDIT,
                                             ld->cur_part_data->rel1_x,
                                             ld->cur_part_data->rel1_y,
                                             ld->cur_part_data->rel2_x,
                                             ld->cur_part_data->rel2_y,
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
   Evas_Object *layout = create_live_edit_layout(ld);

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
menu_it_selected_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   live_data *ld = data;
   const Elm_Object_Item *it = event_info;
   ld->cur_part_data->type = elm_menu_item_index_get(it);
   live_edit_layer_set(ld);
   evas_object_del(ld->menu);
   ld->menu = NULL;
}

static void
menu_dismissed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   live_data *ld = data;
   evas_object_del(obj);
   ld->menu = NULL;
}

static Evas_Object *
menu_create(Evas_Object *parent, live_data *ld)
{
   int i;
   Evas_Object* icon;
   Elm_Object_Item *it;
   Evas_Object *menu = elm_menu_add(parent);
   evas_object_smart_callback_add(menu, "dismissed", menu_dismissed_cb, ld);

   for (i = 0; i < MENU_ITEMS_NUM; i++)
     {
        it = elm_menu_item_add(menu, NULL, NULL, MENU_ITEMS[i].name,
                               menu_it_selected_cb, ld);
        icon = elm_image_add(menu);
        elm_image_file_set(icon, EDJE_PATH, MENU_ITEMS[i].name);
        elm_object_item_part_content_set(it, NULL, icon);
     }

   return menu;
}

static void
layout_mouse_up_cb(void *data, Evas *e EINA_UNUSED,
                   Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   live_data *ld = data;

   // Check if the right button is pressed
   if (ev->button != 3) return;

   if (!ld->menu) ld->menu = menu_create(obj, ld);

   elm_menu_move(ld->menu, ev->canvas.x, ev->canvas.y);
   evas_object_show(ld->menu);
}

void
live_edit_toggle(void)
{
   live_data *ld = g_ld;
   Eina_Bool on = !config_live_edit_get();
   Evas_Object *event_obj = enventor_object_live_view_get(ld->enventor);
   if (!event_obj) return;

   if (on)
     {
        evas_object_event_callback_add(event_obj, EVAS_CALLBACK_MOUSE_UP,
                                       layout_mouse_up_cb, ld);
     }
   else
     {
        evas_object_event_callback_del(event_obj, EVAS_CALLBACK_MOUSE_UP,
                                       layout_mouse_up_cb);
        live_edit_reset(ld);
     }
   enventor_object_disabled_set(ld->enventor, on);

   if (on) stats_info_msg_update("Live View Edit Mode Enabled.");
   else stats_info_msg_update("Live View Edit Mode Disabled.");

   config_live_edit_set(on);
}

void
live_edit_cancel(void)
{
   if (!config_live_edit_get()) return;
   live_edit_toggle();
}

void
live_edit_init(Evas_Object *enventor)
{
   live_data *ld = calloc(1, sizeof(live_data));
   if (!ld)
     {
        EINA_LOG_ERR("Faild to allocate Memory!");
        return;
     }
   g_ld = ld;

   ld->cur_part_data = calloc(1, sizeof(cur_part_data));
   if (!ld->cur_part_data)
     {
        EINA_LOG_ERR("Faild to allocate Memory!");
        return;
     }

   ld->enventor = enventor;

   ld->menu = NULL;
   ld->layout = NULL;
   ld->key_down_handler = NULL;
}

void
live_edit_term()
{
   live_data *ld = g_ld;
   evas_object_del(ld->menu);
   live_edit_reset(ld);
   free(ld->cur_part_data);
   free(ld);
   g_ld = NULL;
}
