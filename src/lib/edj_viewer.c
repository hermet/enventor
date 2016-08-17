#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT 1

#include <Enventor.h>
#include <Edje_Edit.h>
#include <Eio.h>
#include "enventor_private.h"

struct viewer_s
{
   Evas_Object *layout;
   Evas_Object *base;
   Evas_Object *scroller;
   Evas_Object *event_rect;
   Enventor_Object *enventor;
   Enventor_Item *it;

   Evas_Object *part_obj;
   Evas_Object *part_highlight;

   Eina_Stringshare *group_name;
   Eina_Stringshare *part_name;

   Ecore_Idler *idler;
   Ecore_Animator *animator;
   Ecore_Timer *update_img_timer;
   Ecore_Timer *update_edj_timer;
   Ecore_Timer *edj_monitor_timer;
   Eio_Monitor *edj_monitor;
   Eina_List *img_monitors;
   Eina_List *part_names;
   Eio_Monitor *img_monitor;
   Ecore_Event_Handler *edj_monitor_event;
   Ecore_Event_Handler *img_monitor_event;
   Ecore_Event_Handler *exe_del_event;
   void (*del_cb)(void *data);
   void *data;

   /* view size configured by application */
   Evas_Coord_Size view_config_size;
   double view_scale;

   //Keep the part info which state has been changed
   struct {
     Eina_Stringshare *part;
     Eina_Stringshare *desc;
     double state;
   } changed_part;

   Eina_Bool edj_reload_need : 1;
   Eina_Bool file_set_finished : 1;
   Eina_Bool activated: 1;
   Eina_Bool view_update_call_request : 1;
};

const char *PART_NAME = "part_name";


static void
view_obj_parts_callbacks_set(view_data *vd);

static Eina_Bool
exe_del_event_cb(void *data, int type, void *even);

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static Eina_Bool
img_changed_timer_cb(void *data)
{
   view_data *vd = data;
   Eina_File *file = eina_file_open(eio_monitor_path_get(vd->img_monitor),
                                    EINA_FALSE);
   if (!file) return ECORE_CALLBACK_RENEW;
   vd->edj_reload_need = EINA_TRUE;
   vd->update_img_timer = NULL;
   vd->img_monitor = NULL;
   build_edc();
   eina_file_close(file);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
img_changed_cb(void *data, int type EINA_UNUSED, void *event)
{
   view_data *vd = data;
   Eio_Monitor_Event *ev = event;
   Eina_List *l;
   Eio_Monitor *monitor = NULL;
   EINA_LIST_FOREACH(vd->img_monitors, l, monitor)
     {
        if (ev->monitor != monitor) continue;
        vd->img_monitor = monitor;
        ecore_timer_del(vd->update_img_timer);
        //FIXME: here 0.5 was confirmed by experimental way. But we need to
        //decide the time size based on the image file size in order that
        //we could update small images quickly but large images slowly.
        vd->update_img_timer = ecore_timer_add(1, img_changed_timer_cb, vd);
        return ECORE_CALLBACK_DONE;
     }
   return ECORE_CALLBACK_PASS_ON;
}

static void
view_obj_min_update(view_data *vd)
{
   double min_w = (double) vd->view_config_size.w * vd->view_scale;
   if (1 > min_w) min_w = 1;
   double min_h = (double) vd->view_config_size.h * vd->view_scale;
   if (1 > min_h) min_h = 1;

   evas_object_size_hint_min_set(vd->layout, min_w, min_h);
   evas_object_size_hint_max_set(vd->layout, min_w, min_h);
}

static void
view_images_monitor_set(view_data *vd)
{
   Eina_List *l, *l2;
   char *path, *img;
   char buf[PATH_MAX];
   Eio_Monitor *monitor;

   //Free the previous monitors
   EINA_LIST_FREE(vd->img_monitors, monitor)
     eio_monitor_del(monitor);

   Eina_List *imgs = edje_edit_images_list_get(vd->layout);
   Eina_List *paths = build_path_get(ENVENTOR_PATH_TYPE_IMAGE);

   //List up new image pathes and add monitors
   EINA_LIST_FOREACH(imgs, l, img)
     {
        EINA_LIST_FOREACH(paths, l2, path)
          {
             if (path[strlen(path) - 1] == '/')
               snprintf(buf, sizeof(buf), "%s%s", path, img);
             else
               snprintf(buf, sizeof(buf), "%s/%s", path, img);
             if (ecore_file_exists(buf))
               {
                  monitor = eio_monitor_add(buf);
                  vd->img_monitors = eina_list_append(vd->img_monitors,
                                                      monitor);
               }
          }
     }

   edje_edit_string_list_free(imgs);
}

static void
view_obj_create_post_job(view_data *vd)
{
   vd->file_set_finished = EINA_TRUE;

   vd->exe_del_event =
      ecore_event_handler_add(ECORE_EXE_EVENT_DEL, exe_del_event_cb, vd);

   eio_monitor_del(vd->edj_monitor);
   vd->edj_monitor = eio_monitor_add(build_edj_path_get());
   if (!vd->edj_monitor) EINA_LOG_ERR("Failed to add Eio_Monitor");
   view_obj_min_update(vd);

   if (vd->part_name) view_part_highlight_set(vd, vd->part_name);

   if (enventor_obj_dummy_parts_get(vd->enventor))
     dummy_obj_new(vd->layout);

   if (enventor_obj_wireframes_get(vd->enventor))
     wireframes_obj_new(vd->layout);

   view_mirror_mode_update(vd);

   if (vd->changed_part.part)
     edje_edit_part_selected_state_set(vd->layout, vd->changed_part.part,
                                       vd->changed_part.desc,
                                       vd->changed_part.state);

   view_obj_parts_callbacks_set(vd);

   evas_object_smart_callback_call(vd->enventor, SIG_LIVE_VIEW_LOADED, vd->it);
   view_images_monitor_set(vd);
}

static Eina_Bool
file_set_animator_cb(void *data)
{
   view_data *vd = data;
   if (!vd->layout)
     {
        vd->animator = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   if (edje_object_file_set(vd->layout, build_edj_path_get(), vd->group_name))
     {
        view_obj_create_post_job(vd);
        vd->animator = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

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
   Edje_Part_Type type = edje_edit_part_type_get(vd->layout, vd->part_name);
   if (type == EDJE_PART_TYPE_SPACER)
     {
        Evas_Object *scroller_edje = elm_layout_edje_get(vd->scroller);
        // Clipper need, to clip the highlight object for the  part SPACER,
        // because position of the highlight object is calculated here,
        // not in edje. In case, when  the SPACER is placed outside of
        // scroller region view, the highlight should be hided.
        Evas_Object *clipper =
           (Evas_Object *)edje_object_part_object_get(scroller_edje,
                                                      "clipper");

        evas_object_smart_member_add(part_highlight, vd->scroller);
        edje_object_part_geometry_get(vd->layout, vd->part_name,
                                      &x, &y, &w, &h);
        Evas_Coord lx, ly;
        evas_object_geometry_get(vd->layout, &lx, &ly, NULL, NULL);

        evas_object_move(part_highlight, (x + lx), (y + ly));
        evas_object_resize(part_highlight, w, h);
        evas_object_clip_set(part_highlight, clipper);
     }
   else if (type == EDJE_PART_TYPE_TEXT)
     {
        Evas_Coord lx, ly;
        evas_object_geometry_get(vd->layout, &lx, &ly, NULL, NULL);
        edje_object_part_geometry_get(vd->layout, vd->part_name,
                                      &x, &y, &w, &h);
        evas_object_resize(part_highlight, w, h);
        evas_object_move(part_highlight, lx + x, ly + y);
     }
   else
     {
        evas_object_geometry_get(obj, &x, &y, &w, &h);
        evas_object_move(part_highlight, x, y);
        evas_object_resize(part_highlight, w, h);
     }

   vd->part_highlight = part_highlight;
}

static void
event_highlight_geom_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   view_data *vd = (view_data *) data;
   if (!vd) return;

   if (edje_edit_part_type_get(vd->layout, vd->part_name) == EDJE_PART_TYPE_SPACER)
     part_obj_geom_cb(vd, evas_object_evas_get(vd->layout), vd->part_obj, NULL);
}

static void
part_obj_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,

                void *event_info EINA_UNUSED)
{
   view_data *vd = data;
   vd->part_obj = NULL;
}

static void
rect_mouse_move_cb(void *data, Evas *e EINA_UNUSED,
                   Evas_Object *obj EINA_UNUSED, void *event_info)
{
   static Enventor_Live_View_Cursor cursor;
   view_data *vd = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x, y, w, h;
   evas_object_geometry_get(obj, &x, &y, &w, &h);

   cursor.relx = (float) ((ev->cur.canvas.x - x) / (float) w);
   cursor.rely = (float) ((ev->cur.canvas.y - y) / (float) h);

   if (vd->view_config_size.w > 0)
     cursor.x = (((double)vd->view_config_size.w) * cursor.relx);
   else
     cursor.x = (ev->cur.canvas.x - x);

   if (vd->view_config_size.h > 0)
     cursor.y = (((double)vd->view_config_size.h) * cursor.rely);
   else
     cursor.y = (ev->cur.canvas.y - y);

   evas_object_smart_callback_call(vd->enventor, SIG_LIVE_VIEW_CURSOR_MOVED,
                                   &cursor);
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
edje_part_clicked(void *data, Evas *e EINA_UNUSED,
                  Evas_Object *obj  EINA_UNUSED,
                  void *ei EINA_UNUSED)
{
   view_data *vd = (view_data *)data;
   char *part_name = evas_object_data_get(obj, PART_NAME);
   evas_object_smart_callback_call(vd->enventor, "part,clicked", part_name);
}

inline static void
view_obj_parts_names_free(view_data *vd)
{
   Eina_Stringshare *part_name = NULL;

   EINA_LIST_FREE(vd->part_names, part_name)
      eina_stringshare_del(part_name);

   vd->part_names = NULL;
}

static void
view_obj_parts_callbacks_set(view_data *vd)
{
   if (vd->part_names)
      view_obj_parts_names_free(vd);

   Eina_List *l = NULL;
   Eina_Stringshare *part_name = NULL;
   Eina_List *parts = edje_edit_parts_list_get(vd->layout);

   EINA_LIST_FOREACH(parts, l, part_name)
     {
        Evas_Object *edje_part =
               (Evas_Object *)edje_object_part_object_get(vd->layout, part_name);
        if (edje_part)
          {
             Eina_Stringshare *name = eina_stringshare_add(part_name);
             vd->part_names = eina_list_append(vd->part_names, name);
             evas_object_data_set(edje_part, PART_NAME, name);
             evas_object_event_callback_add(edje_part, EVAS_CALLBACK_MOUSE_DOWN,
                                            edje_part_clicked, vd);
          }
     }
   edje_edit_string_list_free(parts);
}

static void
update_view(view_data *vd)
{
   view_images_monitor_set(vd);
   view_obj_min_update(vd);
   view_part_highlight_set(vd, vd->part_name);
   dummy_obj_update(vd->layout);
   wireframes_obj_update(vd->layout);
   view_mirror_mode_update(vd);

   if (vd->changed_part.part)
   edje_edit_part_selected_state_set(vd->layout, vd->changed_part.part,
                                     vd->changed_part.desc,
                                     vd->changed_part.state);

   view_obj_parts_callbacks_set(vd);
   wireframes_obj_callbacks_set(vd->layout);

   if (vd->view_update_call_request)
     {
        evas_object_smart_callback_call(vd->enventor,
                                        SIG_LIVE_VIEW_UPDATED, vd->it);
        vd->view_update_call_request = EINA_FALSE;
     }
}

static void
update_edj_file_internal(view_data *vd)
{
   vd->view_update_call_request = EINA_TRUE;
   vd->edj_reload_need = EINA_FALSE;
   vd->file_set_finished = EINA_TRUE;

   if (!vd->activated) return;

   update_view(vd);
}

static Eina_Bool
update_edj_file(void *data)
{
   view_data *vd = data;

   if (!vd->edj_reload_need)
     {
        vd->update_edj_timer = NULL;
        return ECORE_CALLBACK_DONE;
     }

   //wait for whether edj is generated completely.
   Eina_File *file = eina_file_open(build_edj_path_get(), EINA_FALSE);
   if (!file) return ECORE_CALLBACK_RENEW;
   eina_file_close(file);

   //Failed to load edj? I have no idea. Try again.
   if (!edje_object_file_set(vd->layout, build_edj_path_get(), vd->group_name))
     return ECORE_CALLBACK_RENEW;

   update_edj_file_internal(vd);
   vd->update_edj_timer = NULL;

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
exe_del_event_cb(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   view_data *vd = data;

   if (!vd->edj_reload_need) return ECORE_CALLBACK_PASS_ON;

   //Failed to load edj? I have no idea. Try again.
   if (!edje_object_file_set(vd->layout, build_edj_path_get(), vd->group_name))
     {
        if (vd->activated)
          {
             dummy_obj_update(vd->layout);
             wireframes_obj_update(vd->layout);
          }
        ecore_timer_del(vd->update_edj_timer);
        vd->file_set_finished = EINA_FALSE;
        vd->update_edj_timer = ecore_timer_add(0.25, update_edj_file, vd);
        return ECORE_CALLBACK_PASS_ON;
     }

   update_edj_file_internal(vd);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
edj_monitor_timer_cb(void *data)
{
   view_data *vd = data;

   Eina_File *file = eina_file_open(build_edj_path_get(), EINA_FALSE);
   if (!file) return ECORE_CALLBACK_PASS_ON;
   eina_file_close(file);
   vd->edj_monitor = eio_monitor_add(build_edj_path_get());
   if (!vd->edj_monitor)
     {
        EINA_LOG_ERR("Failed to add Eio_Monitor!");
        return ECORE_CALLBACK_PASS_ON;
     }
   vd->edj_monitor_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
edj_changed_cb(void *data, int type EINA_UNUSED, void *event)
{
   view_data *vd = data;
   Eio_Monitor_Event *ev = event;

   if (vd->edj_monitor != ev->monitor) return ECORE_CALLBACK_PASS_ON;
   view_obj_parts_names_free(vd);

   //FIXME: why it need to add monitor again??
   eio_monitor_del(vd->edj_monitor);

   //Exceptional case. Try again.
   Eina_File *file = eina_file_open(build_edj_path_get(), EINA_FALSE);
   if (!file)
     {
        ecore_timer_del(vd->edj_monitor_timer);
        vd->edj_monitor_timer = ecore_timer_add(0.25, edj_monitor_timer_cb, vd);
        return ECORE_CALLBACK_DONE;
     }
   eina_file_close(file);

   vd->edj_monitor = eio_monitor_add(build_edj_path_get());
   if (!vd->edj_monitor) EINA_LOG_ERR("Failed to add Eio_Monitor!");

   vd->edj_reload_need = EINA_TRUE;

   return ECORE_CALLBACK_DONE;
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

static Evas_Object *
base_create(Evas_Object *parent)
{
   Evas_Object *base = elm_layout_add(parent);
   elm_layout_file_set(base, EDJE_PATH, "viewer_layout_bg");
   evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(base, 0.5, 0.5);

   return base;
}

static void
dummy_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED, void *ei)
{
   char *part_name = (char *)ei;
   view_data *vd = (view_data *)data;
   evas_object_smart_callback_call(vd->enventor, "part,clicked", part_name);
}

static void
view_obj_create(view_data *vd)
{
   Evas *e = evas_object_evas_get(vd->base);
   vd->layout = edje_edit_object_add(e);
   if (!edje_object_file_set(vd->layout, build_edj_path_get(), vd->group_name))
     vd->animator = ecore_animator_add(file_set_animator_cb, vd);
   else
     view_obj_create_post_job(vd);

   evas_object_size_hint_weight_set(vd->layout, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(vd->layout, "dummy,clicked",
                                  dummy_clicked_cb, vd);
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
   evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_WHEEL,
                                  event_highlight_geom_cb, vd);
   vd->event_rect = rect;
}

static Eina_Bool
view_obj_idler_cb(void *data)
{
   view_data *vd = data;

   vd->base = base_create(vd->scroller);

   view_obj_create(vd);

   event_layer_set(vd);

   elm_object_part_content_set(vd->base, "elm.swallow.content",
                               vd->layout);
   elm_object_content_set(vd->scroller, vd->base);

   vd->idler = NULL;

   return ECORE_CALLBACK_CANCEL;
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
view_dummy_set(view_data *vd, Eina_Bool dummy_parts)
{
   if (!vd) return;
   //Does view have dummy object?
   if (dummy_parts) dummy_obj_new(vd->layout);
   else dummy_obj_del(vd->layout);
}

void
view_wireframes_set(view_data *vd, Eina_Bool wireframes)
{
   if (!vd) return;
   if (wireframes) wireframes_obj_new(vd->layout);
   else wireframes_obj_del(vd->layout);
}

view_data *
view_init(Enventor_Object *enventor, Enventor_Item *it, const char *group,
          void (*del_cb)(void *data), void *data)
{
   view_data *vd = calloc(1, sizeof(view_data));
   if (!vd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }
   vd->enventor = enventor;
   vd->it = it;
   vd->scroller = view_scroller_create(enventor);

   vd->group_name = eina_stringshare_add(group);
   vd->idler = ecore_idler_add(view_obj_idler_cb, vd);
   vd->del_cb = del_cb;
   vd->data = data;
   view_part_highlight_set(vd, NULL);

   vd->edj_monitor_event =
      ecore_event_handler_add(EIO_MONITOR_FILE_MODIFIED, edj_changed_cb, vd);
   vd->img_monitor_event =
      ecore_event_handler_add(EIO_MONITOR_FILE_MODIFIED, img_changed_cb, vd);

   vd->view_config_size.w = 0;
   vd->view_config_size.h = 0;
   vd->view_scale = 1;

   return vd;
}

void
view_term(view_data *vd)
{
   if (!vd) return;

   eina_stringshare_del(vd->group_name);
   eina_stringshare_del(vd->part_name);
   eina_stringshare_del(vd->changed_part.part);
   eina_stringshare_del(vd->changed_part.desc);
   view_obj_parts_names_free(vd);

   if (vd->part_obj)
     evas_object_event_callback_del(vd->part_obj, EVAS_CALLBACK_DEL,
                                    part_obj_del_cb);
   evas_object_del(vd->scroller);
   ecore_idler_del(vd->idler);
   ecore_animator_del(vd->animator);
   ecore_timer_del(vd->update_img_timer);
   ecore_timer_del(vd->update_edj_timer);
   ecore_timer_del(vd->edj_monitor_timer);
   eio_monitor_del(vd->edj_monitor);

   Eio_Monitor *monitor;
   EINA_LIST_FREE(vd->img_monitors, monitor)
      eio_monitor_del(monitor);

   ecore_event_handler_del(vd->edj_monitor_event);
   ecore_event_handler_del(vd->img_monitor_event);
   ecore_event_handler_del(vd->exe_del_event);

   free(vd);
}

Evas_Object *
view_obj_get(view_data *vd)
{
   if (!vd) return NULL;
   return vd->scroller;
}

void
view_obj_need_reload_set(view_data *vd)
{
   vd->edj_reload_need = EINA_TRUE;
}

void
view_programs_stop(view_data *vd)
{
   if (!vd || !vd->layout) return;
   if (!vd->file_set_finished) return;
   edje_edit_program_stop_all(vd->layout);
}

void
view_program_run(view_data *vd, const char *program)
{
   if (!vd) return;
   if (!program || !vd->layout) return;
   if (!vd->file_set_finished) return;
   edje_edit_program_run(vd->layout, program);
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

   evas_object_event_callback_del(vd->layout, EVAS_CALLBACK_RESIZE,
                                  event_highlight_geom_cb);
   evas_object_event_callback_del(vd->layout, EVAS_CALLBACK_MOVE,
                                  event_highlight_geom_cb);

   if (!part_obj)
     {
        evas_object_event_callback_add(vd->layout, EVAS_CALLBACK_RESIZE,
                                       event_highlight_geom_cb, vd);
        evas_object_event_callback_add(vd->layout, EVAS_CALLBACK_MOVE,
                                       event_highlight_geom_cb, vd);
     }

   vd->part_obj = part_obj;
   eina_stringshare_replace(&vd->part_name, part_name);
   part_obj_geom_cb(vd, evas_object_evas_get(vd->layout), part_obj, NULL);
}

Eina_Stringshare *
view_group_name_get(view_data *vd)
{
   if (!vd) return NULL;
   return vd->group_name;
}

void *
view_data_get(view_data *vd)
{
   if (!vd) return NULL;
   return vd->data;
}

double
view_scale_get(view_data *vd)
{
   if (!vd) return 1.0;
   return vd->view_scale;
}

void
view_scale_set(view_data *vd, double scale)
{
   if (!vd || !vd->layout) return;
   if (scale == edje_object_scale_get(vd->layout)) return;

   vd->view_scale = scale;

   int pminw, pminh;
   evas_object_size_hint_min_get(vd->layout, &pminw, &pminh);
   Evas_Coord sx, sy, sw, sh;
   elm_scroller_region_get(vd->scroller, &sx, &sy, &sw, &sh);

   edje_object_scale_set(vd->layout, scale);

   view_obj_min_update(vd);

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
}

void
view_size_set(view_data *vd, Evas_Coord w, Evas_Coord h)
{
   static Enventor_Live_View_Size size;

   if (!vd) return;

   int prev_w = vd->view_config_size.w;
   int prev_h = vd->view_config_size.h;

   vd->view_config_size.w = w;
   vd->view_config_size.h = h;
   view_obj_min_update(vd);

   if ((prev_w == w) && (prev_h == h)) return;

   view_size_get(vd, &size.w, &size.h);
   evas_object_smart_callback_call(vd->enventor, SIG_LIVE_VIEW_RESIZED, &size);
}

void
view_size_get(view_data *vd, Evas_Coord *w, Evas_Coord *h)
{
   if (!w || !h) return;
   if (!vd)
     {
        *w = 0;
        *h = 0;
        return;
     }

   evas_object_geometry_get(vd->layout, NULL , NULL, w, h);

   if (vd->view_config_size.w > 0)
     *w = vd->view_config_size.w;

   if (vd->view_config_size.h > 0)
     *h = vd->view_config_size.h;
}

Eina_List *
view_parts_list_get(view_data *vd)
{
   if (!vd || !vd->file_set_finished) return NULL;
   return edje_edit_parts_list_get(vd->layout);
}

Eina_List *
view_images_list_get(view_data *vd)
{
   if (!vd || !vd->file_set_finished) return NULL;
   return edje_edit_images_list_get(vd->layout);
}

Eina_List *
view_programs_list_get(view_data *vd)
{
   if (!vd || !vd->file_set_finished) return NULL;
   return edje_edit_programs_list_get(vd->layout);
}

Edje_Part_Type
view_part_type_get(view_data *vd, const char *part)
{
   if (!vd || !vd->file_set_finished) return EDJE_PART_TYPE_NONE;
   return edje_edit_part_type_get(vd->layout, part);
}

Eina_List *
view_part_states_list_get(view_data *vd, const char *part)
{
   if (!vd || !vd->file_set_finished) return NULL;
   return edje_edit_part_states_list_get(vd->layout, part);
}

Eina_List *
view_program_targets_get(view_data *vd, const char *prog)
{
   if (!vd || !vd->file_set_finished) return NULL;
   return edje_edit_program_targets_get(vd->layout, prog);
}

void
view_string_list_free(Eina_List *list)
{
   edje_edit_string_list_free(list);
}

void
view_part_state_set(view_data *vd, Eina_Stringshare *part,
                    Eina_Stringshare *desc, double state)
{
   if (!vd) return;
   if (!part && !vd->changed_part.part) return;
   if (!vd->file_set_finished) return;

   //reset previous part?
   if (part != vd->changed_part.part)
     {
        view_part_state_set(vd, vd->changed_part.part, "default", 0.0);
        eina_stringshare_del(vd->changed_part.part);
        eina_stringshare_del(vd->changed_part.desc);
     }

   edje_edit_part_selected_state_set(vd->layout, part, desc, state);
   vd->changed_part.part = eina_stringshare_add(part);
   vd->changed_part.desc = eina_stringshare_add(desc);
   vd->changed_part.state = state;
}

void
view_mirror_mode_update(view_data *vd)
{
   if (!vd || !vd->layout) return;
   edje_object_mirrored_set(vd->layout,
                            enventor_obj_mirror_mode_get(vd->enventor));
   dummy_obj_update(vd->layout);
   part_obj_geom_cb(vd, evas_object_evas_get(vd->layout), vd->part_obj, NULL);
}

Enventor_Item *
view_item_get(view_data *vd)
{
   if (!vd) return NULL;
   return vd->it;
}

void
view_activated_set(view_data *vd, Eina_Bool activated)
{
   if (!vd) return;
   activated = !!activated;
   if (activated == vd->activated) return;
   vd->activated = activated;
   if (!activated) return;

   update_view(vd);
}

