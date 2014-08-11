#include <Elementary.h>
#include "common.h"

struct live_editor_s
{
   Evas_Object *menu;
   edit_data *ed;
};

#define LIVE_EDIT_ITEMS_NUM 6
static const char *LIVE_EDIT_MENU_LABELS[LIVE_EDIT_ITEMS_NUM] =
{
   "RECT",
   "IMAGE",
   "SPACER",
   "SWALLOW",
   "TEXT",
   "TEXTBLOCK"
};

static Eina_Bool
live_edit_part_insert(live_edit_data *led, unsigned int part_type,
                      Eina_Stringshare *group_name)
{
   Edje_Part_Type type;
   switch (part_type)
     {
      case 0:
        {
           type = EDJE_PART_TYPE_RECTANGLE;
           break;
        }
      case 1:
        {
           type = EDJE_PART_TYPE_IMAGE;
           break;
        }
      case 2:
        {
           type = EDJE_PART_TYPE_SPACER;
           break;
        }
      case 3:
        {
           type = EDJE_PART_TYPE_SWALLOW;
           break;
        }
      case 4:
        {
           type = EDJE_PART_TYPE_TEXT;
           break;
        }
      case 5:
        {
           type = EDJE_PART_TYPE_TEXTBLOCK;
           break;
        }
   }
   template_live_edit_part_insert(led->ed, type, group_name);
   return EINA_TRUE;
}

live_edit_data*
live_edit_init(edit_data *ed)
{
   live_edit_data *led = calloc(1, sizeof(live_edit_data));
   if (!led)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }
   led->ed = ed;

   return led;
}

static void
live_edit_menu_item_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   live_edit_data *led = data;
   const Elm_Object_Item *it = event_info;
   unsigned int part_type = elm_menu_item_index_get(it);
   live_edit_part_insert(led, part_type, view_group_name_get(VIEW_DATA));
}

static Evas_Object *
live_edit_menu_create(Evas_Object *parent, live_edit_data *led)
{
   long int i;
   Evas_Object* icon;
   Elm_Object_Item *it;

   Evas_Object *menu = elm_menu_add(parent);

   for (i = 0; i < LIVE_EDIT_ITEMS_NUM; i++)
     {
        it = elm_menu_item_add(menu, NULL, NULL,
                               LIVE_EDIT_MENU_LABELS[i],
                               live_edit_menu_item_selected_cb, led);
        icon = elm_image_add(menu);
        elm_image_file_set(icon, EDJE_PATH, LIVE_EDIT_MENU_LABELS[i]);
        elm_object_item_part_content_set(it, NULL, icon);
     }
   
   return menu;
}


static void
live_edit_menu_show_cb(void *data, Evas *e EINA_UNUSED,
                       Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   live_edit_data *led = data;

   // Check if the right button is pressed
   if (ev->button != 3) return;

   if (!led->menu)
     led->menu = live_edit_menu_create(obj, led);

   elm_menu_move(led->menu, ev->canvas.x, ev->canvas.y);
   evas_object_show(led->menu);
}

void
live_edit_toggle(live_edit_data* led, Eina_Bool is_on)
{
   if (VIEW_DATA)
     {
        Evas_Object *view_layout = view_layout_get(VIEW_DATA);

        if (is_on)
          evas_object_event_callback_add(view_layout, EVAS_CALLBACK_MOUSE_UP,
                                         live_edit_menu_show_cb, led);
        else
          evas_object_event_callback_del(view_layout, EVAS_CALLBACK_MOUSE_UP,
                                         live_edit_menu_show_cb);
     }

   edit_disabled_set(led->ed, is_on);
   if (is_on)
     stats_info_msg_update("Enabled Live View Edit Mode");
   else
     stats_info_msg_update("Disabled Live View Edit Mode");
}

void
live_edit_term(live_edit_data* led)
{
   if (led->menu)
     evas_object_del(led->menu);
   free(led);
}

#undef LIVE_EDIT_ITEMS_NUM
