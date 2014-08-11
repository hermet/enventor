#include <Elementary.h>
#include "common.h"

typedef struct menu_data_s
{
   const char *name;
   int type;
} menu_data;

typedef struct live_editor_s
{
   Evas_Object *menu;
   edit_data *ed;
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

static void
menu_it_selected_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   live_data *ld = data;
   const Elm_Object_Item *it = event_info;
   unsigned int idx = elm_menu_item_index_get(it);
   template_live_edit_part_insert(ld->ed, MENU_ITEMS[idx].type,
                                  view_group_name_get(VIEW_DATA));
   evas_object_del(ld->menu);
   ld->menu = NULL;
}

static void
menu_dismissed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   live_data *ld = data;
   evas_object_del(ld->menu);
   ld->menu = NULL;
}

static Evas_Object *
menu_create(Evas_Object *parent, live_data *ld)
{
   int i;
   Evas_Object* icon;
   Elm_Object_Item *it;
   Evas_Object *menu = elm_menu_add(parent);

   for (i = 0; i < MENU_ITEMS_NUM; i++)
     {
        it = elm_menu_item_add(menu, NULL, NULL, MENU_ITEMS[i].name,
                               menu_it_selected_cb, ld);
        icon = elm_image_add(menu);
        elm_image_file_set(icon, EDJE_PATH, MENU_ITEMS[i].name);
        elm_object_item_part_content_set(it, NULL, icon);
     }

   evas_object_smart_callback_add(menu, "dismissed", menu_dismissed_cb, ld);

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

   ld->menu = menu_create(obj, ld);

   elm_menu_move(ld->menu, ev->canvas.x, ev->canvas.y);
   evas_object_show(ld->menu);
}

void
live_edit_toggle(void)
{
   live_data *ld = g_ld;
   Eina_Bool on = !config_live_edit_get();

   Evas_Object *event_obj = view_obj_get(VIEW_DATA);
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
     }

   edit_disabled_set(ld->ed, on);

   if (on) stats_info_msg_update("Live View Edit Mode Enabled.");
   else stats_info_msg_update("Live View Edit Mode Disabled.");

   config_live_edit_set(on);
}

void
live_edit_init(edit_data *ed)
{
   live_data *ld = calloc(1, sizeof(live_data));
   if (!ld)
     {
        EINA_LOG_ERR("Faild to allocate Memory!");
        return;
     }
   g_ld = ld;

   ld->ed = ed;
}

void
live_edit_term()
{
   live_data *ld = g_ld;
   if (ld->menu) evas_object_del(ld->menu);
   free(ld);
   g_ld = NULL;
}
