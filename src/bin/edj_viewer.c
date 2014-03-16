#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT 1

#include <Elementary.h>
#include <Edje_Edit.h>
#include "common.h"

struct viewer_s
{
   Evas_Object *parent;
   Evas_Object *layout;
   Evas_Object *scroller;

   Evas_Object *part_obj;
   Evas_Object *part_highlight;

   Eina_Stringshare *group_name;
   Eina_Stringshare *part_name;

   Ecore_Idler *idler;
   Ecore_Timer *timer;

   void (*del_cb)(void *data);
   void *data;

   Eina_Bool dummy_on;
};

static void
view_obj_min_update(Evas_Object *obj)
{
   Evas_Coord w, h;
   edje_object_size_min_calc(obj, &w, &h);
   double scale = edje_object_scale_get(obj);
   evas_object_size_hint_min_set(obj, ((double)w * scale), ((double)h * scale));
}

static Eina_Bool
file_set_timer_cb(void *data)
{
   view_data *vd = data;
   if (!vd->layout)
     {
        vd->timer = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   if (edje_object_file_set(vd->layout, config_edj_path_get(),
                            vd->group_name))
     {
        vd->timer = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   view_obj_min_update(vd->layout);
   edj_mgr_reload_need_set(EINA_TRUE);

   return ECORE_CALLBACK_RENEW;
}

static void
part_obj_geom_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                 void *event_info EINA_UNUSED)
{
   view_data *vd = data;
   Evas_Object *part_highlight = vd->part_highlight;

   //Create Part Highlight Object
   if (!part_highlight && vd->part_name)
     {
        part_highlight = elm_layout_add(vd->scroller);
        evas_object_smart_member_add(part_highlight, vd->scroller);
        elm_layout_file_set(part_highlight, EDJE_PATH, "part_highlight");
        evas_object_pass_events_set(part_highlight, EINA_TRUE);
        evas_object_show(part_highlight);
     }

   Evas_Coord x, y, w , h;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_move(part_highlight, x, y);
   evas_object_resize(part_highlight, w, h);

   vd->part_highlight = part_highlight;
}

static void
part_obj_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   view_data *vd = data;
   vd->part_obj = NULL;
}

static void
layout_resize_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                 Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   if (!config_stats_bar_get()) return;

   Evas_Coord w, h;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   config_view_size_set(w, h);
   stats_view_size_update();
}

static void
rect_mouse_move_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                   Evas_Object *obj EINA_UNUSED, void *event_info)
{
   if (!config_stats_bar_get()) return;

   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x, y, w, h;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   stats_cursor_pos_update(ev->cur.canvas.x - x, ev->cur.canvas.y - y,
                           (float) (ev->cur.canvas.x - x) / (float) w,
                           (float) (ev->cur.canvas.y - y) / (float) h);
}

static Evas_Object *
view_scroller_create(Evas_Object *parent)
{
   Evas_Object *scroller = elm_scroller_add(parent);
   elm_object_focus_allow_set(scroller, EINA_FALSE);
   evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);

   return scroller;
}

static void
edje_change_file_cb(void *data, Evas_Object *obj,
                    const char *emission EINA_UNUSED,
                    const char *source EINA_UNUSED)
{
   view_data *vd = data;
   if (!edje_object_file_set(vd->layout, config_edj_path_get(),
                        vd->group_name))
     {
        vd->del_cb(vd->data);
        view_term(vd);
        return;
     }
   view_obj_min_update(obj);
   view_part_highlight_set(vd, vd->part_name);
}

static void
layout_geom_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
               void *event_info EINA_UNUSED)
{
   Evas_Coord x, y, w, h;
   Evas_Object *rect = data;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_move(rect, x, y);
   evas_object_resize(rect, w, h);
}

static void
layout_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   Evas_Object *rect = data;
   evas_object_del(rect);
}
static void
event_layer_set(view_data *vd)
{
   Evas *e = evas_object_evas_get(vd->layout);
   Evas_Object *rect = evas_object_rectangle_add(e);
   evas_object_repeat_events_set(rect, EINA_TRUE);
   evas_object_color_set(rect, 0, 0, 0, 0);
   evas_object_show(rect);

   evas_object_event_callback_add(vd->layout, EVAS_CALLBACK_RESIZE,
                                  layout_geom_cb, rect);
   evas_object_event_callback_add(vd->layout, EVAS_CALLBACK_MOVE,
                                  layout_geom_cb, rect);
   evas_object_event_callback_add(vd->layout, EVAS_CALLBACK_DEL,
                                  layout_del_cb, rect);
   evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_MOVE,
                                  rect_mouse_move_cb, vd);
}

static Evas_Object *
view_obj_create(view_data *vd, const char *file_path, const char *group)
{
   Evas *e = evas_object_evas_get(vd->scroller);
   Evas_Object *layout = edje_edit_object_add(e);
   if (!edje_object_file_set(layout, file_path, group))
     {
        //FIXME: more optimized way?
        vd->timer = ecore_timer_add(1, file_set_timer_cb, vd);
     }
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_event_callback_add(layout, EVAS_CALLBACK_RESIZE,
                                  layout_resize_cb, vd);
   edje_object_signal_callback_add(layout,
                                   "edje,change,file", "edje",
                                   edje_change_file_cb, vd);
   view_obj_min_update(layout);

   return layout;
}

static Eina_Bool
view_obj_idler_cb(void *data)
{
   view_data *vd = data;

   vd->layout = view_obj_create(vd, config_edj_path_get(), vd->group_name);
   view_scale_set(vd, config_view_scale_get());

   event_layer_set(vd);
   elm_object_content_set(vd->scroller, vd->layout);

   if (vd->dummy_on)
     dummy_obj_new(vd->layout);

   vd->idler = NULL;
   if (vd->part_name) view_part_highlight_set(vd, vd->part_name);

   return ECORE_CALLBACK_CANCEL;
}

void
view_dummy_toggle(view_data *vd, Eina_Bool msg)
{
   Eina_Bool dummy_on = config_dummy_swallow_get();
   if (dummy_on == vd->dummy_on) return;
   if (dummy_on)
     {
        if (msg) stats_info_msg_update("Dummy Swallow Enabled.");
        dummy_obj_new(vd->layout);
     }
   else
     {
        if (msg) stats_info_msg_update("Dummy Swallow Disabled.");
        dummy_obj_del(vd->layout);
     }

   vd->dummy_on = dummy_on;
}

view_data *
view_init(Evas_Object *parent, const char *group,
          void (*del_cb)(void *data), void *data)
{
   view_data *vd = calloc(1, sizeof(view_data));
   vd->parent = parent;
   vd->scroller = view_scroller_create(parent);
   vd->dummy_on = config_dummy_swallow_get();

   vd->group_name = eina_stringshare_add(group);
   vd->idler = ecore_idler_add(view_obj_idler_cb, vd);
   vd->del_cb = del_cb;
   vd->data = data;
   view_part_highlight_set(vd, NULL);
   return vd;
}

void
view_term(view_data *vd)
{
   if (!vd) return;

   if (vd->group_name) eina_stringshare_del(vd->group_name);
   if (vd->part_name) eina_stringshare_del(vd->part_name);

   if (vd->part_obj)
     evas_object_event_callback_del(vd->part_obj, EVAS_CALLBACK_DEL,
                                    part_obj_del_cb);
   evas_object_del(vd->scroller);

   ecore_idler_del(vd->idler);
   ecore_timer_del(vd->timer);

   free(vd);
}

Evas_Object *
view_obj_get(view_data *vd)
{
   return vd->scroller;
}

void
view_program_run(view_data *vd, const char *program)
{
   if (!vd) return;
   if (!program || !vd->layout) return;
   edje_edit_program_run(vd->layout, program);
   char buf[256];
   snprintf(buf, sizeof(buf), "Program Run: \"%s\"", program);
   stats_info_msg_update(buf);
}

void
view_part_highlight_set(view_data *vd, const char *part_name)
{
   if (!vd) return;

   if (!vd->layout)
     {
        if (vd->idler) vd->part_name = eina_stringshare_add(part_name);
        return;
     }

   if (!part_name)
     {
        if (vd->part_highlight)
          {
             evas_object_del(vd->part_highlight);
             vd->part_highlight = NULL;
          }
        if (vd->part_name)
          {
             eina_stringshare_del(vd->part_name);
             vd->part_name = NULL;
          }
        return;
     }
   if (vd->part_obj && (vd->part_name == part_name)) return;

   Evas_Object *part_obj =
      (Evas_Object *) edje_object_part_object_get(vd->layout, part_name);
   if (!part_obj) return;

   //Delete the previous part callbacks
   if (vd->part_obj)
     {
        evas_object_event_callback_del(vd->part_obj, EVAS_CALLBACK_RESIZE,
                                       part_obj_geom_cb);
        evas_object_event_callback_del(vd->part_obj, EVAS_CALLBACK_MOVE,
                                       part_obj_geom_cb);
        evas_object_event_callback_del(vd->part_obj, EVAS_CALLBACK_DEL,
                                       part_obj_del_cb);
     }
   evas_object_event_callback_add(part_obj, EVAS_CALLBACK_RESIZE,
                                  part_obj_geom_cb, vd);
   evas_object_event_callback_add(part_obj, EVAS_CALLBACK_MOVE,
                                  part_obj_geom_cb, vd);
   evas_object_event_callback_add(part_obj, EVAS_CALLBACK_DEL, part_obj_del_cb,
                                  vd);

   vd->part_obj = part_obj;
   eina_stringshare_replace(&vd->part_name, part_name);
   part_obj_geom_cb(vd, evas_object_evas_get(vd->layout), part_obj, NULL);
}

Eina_Stringshare *
view_group_name_get(view_data *vd)
{
   return vd->group_name;
}

void *
view_data_get(view_data *vd)
{
   return vd->data;
}

void
view_scale_set(view_data *vd, double scale)
{
   if (!vd->layout) return;
   if (scale == edje_object_scale_get(vd->layout)) return;

   int pminw, pminh;
   evas_object_size_hint_min_get(vd->layout, &pminw, &pminh);
   Evas_Coord sx, sy, sw, sh;
   elm_scroller_region_get(vd->scroller, &sx, &sy, &sw, &sh);

   edje_object_scale_set(vd->layout, scale);
   view_obj_min_update(vd->layout);

   //adjust scroller position according to the scale change.
   int minw, minh;
   evas_object_size_hint_min_get(vd->layout, &minw, &minh);

   //a. center position of the scroller
   double cx = ((double)sx + ((double)sw * 0.5));
   double cy = ((double)sy + ((double)sh * 0.5));

   //b. multiply scale value
   cx *= (1 + (((double)(minw - pminw))/pminw));
   cy *= (1 + (((double)(minh - pminh))/pminh));

   elm_scroller_region_show(vd->scroller, ((Evas_Coord) cx) - (sw / 2),
                            ((Evas_Coord) cy) - (sh / 2), sw, sh);

   //FIXME: Update the size for weird text ellipsis.
   Evas_Coord w, h;
   evas_object_geometry_get(vd->layout, NULL, NULL, &w, &h);
   evas_object_resize(vd->layout, 0, 0);
   evas_object_resize(vd->layout, w, h);
}

Eina_List *
view_parts_list_get(view_data *vd)
{
   return edje_edit_parts_list_get(vd->layout);
}

void
view_parts_list_free(Eina_List *list)
{
   edje_edit_string_list_free(list);
}
