#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Elementary_Cursor.h>
#include "common.h"

#define CTRL_PT_LAYER 3
#define INFO_TEXT_LAYER (CTRL_PT_LAYER+1)
#define ROUNDING(x, dig) (floor((x) * pow(10, dig) + 0.5) / pow(10, dig))

typedef enum
{
   Ctrl_Pt_Rel1 = 0,
   Ctrl_Pt_Rel2,
   Ctrl_Pt_Rel3,
   Ctrl_Pt_Rel4,
   Ctrl_Pt_Top,
   Ctrl_Pt_Bottom,
   Ctrl_Pt_Left,
   Ctrl_Pt_Right,
   Ctrl_Pt_Cnt
} Ctrl_Pt;

typedef enum
{
   Info_Text_Rel1 = 0,
   Info_Text_Rel2,
   Info_Text_Size,
   Info_Text_Cnt
} Info_Text;

typedef enum
{
   Align_Line_Top = 0,
   Align_Line_Bottom,
   Align_Line_Left,
   Align_Line_Right,
   Align_Line_Cnt
} Align_Line;

typedef struct ctxpopup_it_data_s
{
   const char *name;
   Edje_Part_Type type;
} ctxpopup_it_data;

typedef struct live_editor_s
{
   Evas_Object *ctxpopup;
   Evas_Object *layout;
   Evas_Object *live_view;
   Evas_Object *enventor;
   Evas_Object *trigger;
   Evas_Object *ctrl_pt[Ctrl_Pt_Cnt];
   Evas_Object *align_line[Align_Line_Cnt];
   Evas_Object *info_text[Info_Text_Cnt];
   Evas_Coord_Point move_delta;
   double half_ctrl_size;

   struct {
      unsigned int type;
      float rel1_x, rel1_y;
      float rel2_x, rel2_y;
   } part_info;

   Ecore_Event_Handler *key_down_handler;

   Eina_Bool on : 1;
} live_data;

static void live_edit_update_internal(live_data *ld);

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

static Evas_Object *
view_obj_get(live_data *ld)
{
   //This is a trick! we got the actual view object from the live edit.
   Evas_Object *o, *o2, *o3;
   o = elm_object_part_content_get(ld->live_view, "elm.swallow.content");
   o2 = elm_object_content_get(o);
   o3 = elm_object_part_content_get(o2, "elm.swallow.content");

   return o3;
}

static void
info_text_update(live_data *ld)
{
   //Update Text
   char buf[256];

   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->live_view, &lx, &ly, &lw, &lh);

   //Rel1
   snprintf(buf, sizeof(buf), "%.2f %.2f (%d, %d)",
            ld->part_info.rel1_x, ld->part_info.rel1_y,
            (int) round(ld->part_info.rel1_x * (double) lw),
            (int) round(ld->part_info.rel1_y * (double) lh));
  evas_object_text_text_set(ld->info_text[Info_Text_Rel1], buf);

   //Rel2
   snprintf(buf, sizeof(buf), "%.2f %.2f (%d, %d)",
            ld->part_info.rel2_x, ld->part_info.rel2_y,
            (int) round(ld->part_info.rel2_x * (double) lw),
            (int) round(ld->part_info.rel2_y * (double) lh));
   evas_object_text_text_set(ld->info_text[Info_Text_Rel2], buf);

   //Size
   Evas_Coord layout_x, layout_y, layout_w, layout_h;
   evas_object_geometry_get(ld->layout, &layout_x, &layout_y, &layout_w,
                            &layout_h);
   snprintf(buf, sizeof(buf), "[%d x %d]", layout_w, layout_h);
   evas_object_text_text_set(ld->info_text[Info_Text_Size], buf);

   //Update Position
   Evas_Coord x, y, w, h;
   Evas_Coord rx, ry, rw, rh;

   //Rel1
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], &rx, &ry, &rw, &rh);
   evas_object_geometry_get(ld->info_text[Info_Text_Rel1], NULL, NULL, &w, &h);
   x = (rx + rw);
   y = ry - h;
   if ((x + w) > (lx + lw)) x = (rx - w);
   if (y < ly) y = (ry + rh);
   evas_object_move(ld->info_text[Info_Text_Rel1], x, y);

   //Rel2
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], &rx, &ry, &rw, &rh);
   evas_object_geometry_get(ld->info_text[Info_Text_Rel2], NULL, NULL, &w, &h);
   x = (rx - w);
   y = (ry + rh);
   if (x < lx) x = (rx + rw);
   if ((y + h) > (ly + lh)) y = (ry - h);
   evas_object_move(ld->info_text[Info_Text_Rel2], x, y);

   //Size
   evas_object_geometry_get(ld->info_text[Info_Text_Size], NULL, NULL, &w, &h);
   x = (layout_x + (layout_w/2)) - (w/2);
   y = (layout_y + (layout_h/2)) - (h/2);
   if (x < lx) x = lx;
   if (y < lx) y = ly;
   if ((x + w) > (lx + lw)) x = ((lx + lw) - w);
   if ((y + h) > (ly + lh)) y = ((ly + lh) - h);
   evas_object_move(ld->info_text[Info_Text_Size], x, y);
}

static void
live_edit_symbol_set(live_data *ld)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s_bg", CTXPOPUP_ITEMS[ld->part_info.type].name);
   Evas_Object *layout_symbol = elm_layout_add(ld->layout);
   elm_layout_file_set(layout_symbol, EDJE_PATH, buf);
   elm_object_scale_set(layout_symbol, config_view_scale_get());
   elm_object_part_content_set(ld->layout, "elm.swallow.symbol", layout_symbol);
}

static void
live_edit_insert(live_data *ld)
{
   int type = CTXPOPUP_ITEMS[ld->part_info.type].type;
   enventor_object_template_part_insert(ld->enventor,
                                        type,
                                        ENVENTOR_TEMPLATE_INSERT_LIVE_EDIT,
                                        ld->part_info.rel1_x,
                                        ld->part_info.rel1_y,
                                        ld->part_info.rel2_x,
                                        ld->part_info.rel2_y,
                                        NULL, 0);
   enventor_object_save(ld->enventor, config_edc_path_get());
}

static Eina_Bool
key_down_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   live_data *ld = data;

   if (!strcmp(event->key, "Return")) live_edit_insert(ld);
   else if (strcmp(event->key, "Delete") &&
            strcmp(event->key, "BackSpace")) return EINA_TRUE;

   live_edit_cancel();
   return EINA_TRUE;
}

static void
ctrl_pt_update(live_data *ld)
{
   //Init Control Point Positions
   Evas_Coord dx, dy, dw, dh;
   evas_object_geometry_get(ld->layout, &dx, &dy, &dw, &dh);

   double x = dx;
   double y = dy;
   double w = dw;
   double h = dh;

   int half_ctrl_size = ld->half_ctrl_size;

   //Rel1
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Rel1],
                    round((x - half_ctrl_size)), round((y - half_ctrl_size)));

   //Rel2
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Rel2],
                    round((x + w) - half_ctrl_size),
                    round((y + h) - half_ctrl_size));

   //Rel3
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Rel3],
                    round((x + w) - half_ctrl_size),
                    round(y - half_ctrl_size));

   //Rel4
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Rel4],
                    round(x - half_ctrl_size),
                    round((y + h) - half_ctrl_size));

   //Top
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Top],
                    round((x + (w/2)) - half_ctrl_size),
                    round(y - half_ctrl_size));

   //Bottom
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Bottom],
                    round((x + (w/2)) - half_ctrl_size),
                    round((y + h) - half_ctrl_size));

   //Left
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Left],
                    round(x - half_ctrl_size),
                    round((y + (h/2)) - half_ctrl_size));

   //Right
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Right],
                    round((x + w) - half_ctrl_size),
                    round((y + (h/2)) - half_ctrl_size));
}

static void
cp_top_mouse_move_cb(void *data, Evas *e EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED,
                     void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord y = ev->cur.canvas.y;

   Evas_Object *view = view_obj_get(ld);
   Evas_Coord vx, vy, vw, vh;
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   Evas_Coord rel2_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], NULL, &rel2_y,
                            NULL, NULL);
   if (vy > y) y = vy;
   if ((y - ld->half_ctrl_size) > rel2_y) y = (rel2_y + ld->half_ctrl_size);

   ld->part_info.rel1_y = ROUNDING(((double) (y - vy) / (double) vh), 2);

   elm_object_signal_emit(ld->align_line[Align_Line_Top], "elm,state,show", "");
}

static void
cp_bottom_mouse_move_cb(void *data, Evas *e EINA_UNUSED,
                        Evas_Object *obj EINA_UNUSED,
                        void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord y = ev->cur.canvas.y;

   Evas_Object *view = view_obj_get(ld);
   Evas_Coord vx, vy, vw, vh;
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   Evas_Coord rel1_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], NULL, &rel1_y,
                            NULL, NULL);
   if (y > (vy + vh)) y = (vy + vh);
   if (rel1_y > (y + ld->half_ctrl_size)) y = (rel1_y + ld->half_ctrl_size);

   ld->part_info.rel2_y = ROUNDING(((double) (y - vy) / (double) vh), 2);

   elm_object_signal_emit(ld->align_line[Align_Line_Bottom], "elm,state,show",
                          "");
}

static void
align_line_update(live_data *ld)
{
   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->live_view, &lx, &ly, &lw, &lh);

   Evas_Coord vw, vh;
   config_view_size_get(&vw, &vh);
   vw *= config_view_scale_get();
   vh *= config_view_scale_get();

   lx = (lx + (lw * 0.5)) - (vw * 0.5);
   ly = (ly + (lh * 0.5)) - (vh * 0.5);
   lw = vw;
   lh = vh;

   int x, y;

   //Top
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Top], NULL, &y, NULL, NULL);
   y = round(((double) y) + ld->half_ctrl_size);
   evas_object_move(ld->align_line[Align_Line_Top], (lx + 1), y);
   evas_object_resize(ld->align_line[Align_Line_Top], (lw - 2),
                      (ELM_SCALE_SIZE(1)));

   //Bottom
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Bottom], NULL, &y, NULL, NULL);
   y = round(((double) y) + ld->half_ctrl_size);
   evas_object_move(ld->align_line[Align_Line_Bottom], (lx + 1), (y - 1));
   evas_object_resize(ld->align_line[Align_Line_Bottom], (lw - 2),
                      ELM_SCALE_SIZE(1));
   //Left
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Left], &x, NULL, NULL, NULL);
   x = round(((double) x) + ld->half_ctrl_size);
   evas_object_move(ld->align_line[Align_Line_Left], x, (ly + 1));
   evas_object_resize(ld->align_line[Align_Line_Left],
                      ELM_SCALE_SIZE(1), (lh - 2));
   //Right
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Right], &x, NULL, NULL, NULL);
   x = round(((double) x) + ld->half_ctrl_size);
   evas_object_move(ld->align_line[Align_Line_Right], (x - 1), (ly + 1));
   evas_object_resize(ld->align_line[Align_Line_Right],
                      ELM_SCALE_SIZE(1), (lh - 2));
}

static void
cp_rel1_mouse_move_cb(void *data, Evas *e EINA_UNUSED,
                      Evas_Object *obj EINA_UNUSED,
                      void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x = ev->cur.canvas.x;
   Evas_Coord y = ev->cur.canvas.y;

   Evas_Object *view = view_obj_get(ld);
   Evas_Coord vx, vy, vw, vh;
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   Evas_Coord rel2_x, rel2_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], &rel2_x, &rel2_y,
                            NULL, NULL);
   if (vx > x) x = vx;
   if (vy > y) y = vy;
   if ((x - ld->half_ctrl_size) > rel2_x) x = (rel2_x + ld->half_ctrl_size);
   if ((y - ld->half_ctrl_size) > rel2_y) y = (rel2_y + ld->half_ctrl_size);

   ld->part_info.rel1_x = ROUNDING(((double) (x - vx) / (double) vw), 2);
   ld->part_info.rel1_y = ROUNDING(((double) (y - vy) / (double) vh), 2);

   elm_object_signal_emit(ld->align_line[Align_Line_Left], "elm,state,show",
                          "");
   elm_object_signal_emit(ld->align_line[Align_Line_Top], "elm,state,show", "");
}

static void
cp_rel2_mouse_move_cb(void *data, Evas *e EINA_UNUSED,
                      Evas_Object *obj EINA_UNUSED,
                      void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x = ev->cur.canvas.x;
   Evas_Coord y = ev->cur.canvas.y;

   Evas_Object *view = view_obj_get(ld);
   Evas_Coord vx, vy, vw, vh;
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   Evas_Coord rel1_x, rel1_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], &rel1_x, &rel1_y,
                            NULL, NULL);
   if (x > (vx + vw)) x = (vx + vw);
   if (y > (vy + vh)) y = (vy + vh);
   if (rel1_x > (x + ld->half_ctrl_size)) x = (rel1_x + ld->half_ctrl_size);
   if (rel1_y > (y + ld->half_ctrl_size)) y = (rel1_y + ld->half_ctrl_size);

   ld->part_info.rel2_x = ROUNDING(((double) (x - vx) / (double) vw), 2);
   ld->part_info.rel2_y = ROUNDING(((double) (y - vy) / (double) vh), 2);

   elm_object_signal_emit(ld->align_line[Align_Line_Right], "elm,state,show",
                          "");
   elm_object_signal_emit(ld->align_line[Align_Line_Bottom], "elm,state,show",
                          "");
}

static void
cp_rel3_mouse_move_cb(void *data, Evas *e EINA_UNUSED,
                      Evas_Object *obj EINA_UNUSED,
                      void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x = ev->cur.canvas.x;
   Evas_Coord y = ev->cur.canvas.y;

   Evas_Object *view = view_obj_get(ld);
   Evas_Coord vx, vy, vw, vh;
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   Evas_Coord rel1_x;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], &rel1_x, NULL,
                            NULL, NULL);
   if (x > (vx + vw)) x = (vx + vw);
   if (rel1_x > (x + ld->half_ctrl_size)) x = (rel1_x - ld->half_ctrl_size);

   Evas_Coord rel2_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], NULL, &rel2_y,
                            NULL, NULL);
   if (vy > y) y = vy;
   if ((y - ld->half_ctrl_size) > rel2_y) y = (rel2_y + ld->half_ctrl_size);

   ld->part_info.rel2_x = ROUNDING(((double) (x - vx) / (double) vw), 2);
   ld->part_info.rel1_y = ROUNDING(((double) (y - vy) / (double) vh), 2);

   elm_object_signal_emit(ld->align_line[Align_Line_Right], "elm,state,show",
                          "");
   elm_object_signal_emit(ld->align_line[Align_Line_Top], "elm,state,show", "");
}

static void
cp_rel4_mouse_move_cb(void *data, Evas *e EINA_UNUSED,
                      Evas_Object *obj EINA_UNUSED,
                      void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x = ev->cur.canvas.x;
   Evas_Coord y = ev->cur.canvas.y;

   Evas_Object *view = view_obj_get(ld);
   Evas_Coord vx, vy, vw, vh;
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   Evas_Coord rel2_x;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], &rel2_x, NULL,
                            NULL, NULL);
   if (vx > x) x = vx;
   if ((x - ld->half_ctrl_size) > rel2_x) x = (rel2_x + ld->half_ctrl_size);

   Evas_Coord rel1_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], NULL, &rel1_y,
                            NULL, NULL);
   if (y > (vy + vh)) y = (vy + vh);
   if (rel1_y > (y + ld->half_ctrl_size)) y = (rel1_y + ld->half_ctrl_size);

   ld->part_info.rel1_x = ROUNDING(((double) (x - vx) / (double) vw), 2);
   ld->part_info.rel2_y = ROUNDING(((double) (y - vy) / (double) vh), 2);

   elm_object_signal_emit(ld->align_line[Align_Line_Left], "elm,state,show",
                          "");
   elm_object_signal_emit(ld->align_line[Align_Line_Bottom], "elm,state,show",
                          "");
}

static void
cp_left_mouse_move_cb(void *data, Evas *e EINA_UNUSED,
                      Evas_Object *obj EINA_UNUSED,
                      void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x = ev->cur.canvas.x;

   Evas_Object *view = view_obj_get(ld);
   Evas_Coord vx, vy, vw, vh;
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   Evas_Coord rel2_x;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], &rel2_x, NULL,
                            NULL, NULL);
   if (vx > x) x = vx;
   if ((x - ld->half_ctrl_size) > rel2_x) x = (rel2_x + ld->half_ctrl_size);

   ld->part_info.rel1_x = ROUNDING(((double) (x - vx) / (double) vw), 2);

   elm_object_signal_emit(ld->align_line[Align_Line_Left], "elm,state,show",
                          "");
}

static void
cp_right_mouse_move_cb(void *data, Evas *e EINA_UNUSED,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x = ev->cur.canvas.x;

   Evas_Object *view = view_obj_get(ld);
   Evas_Coord vx, vy, vw, vh;
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   Evas_Coord rel1_x;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], &rel1_x, NULL,
                            NULL, NULL);
   if (x > (vx + vw)) x = (vx + vw);
   if (rel1_x > (x + ld->half_ctrl_size)) x = (rel1_x + ld->half_ctrl_size);

   ld->part_info.rel2_x = ROUNDING(((double) (x - vx) / (double) vw), 2);

   elm_object_signal_emit(ld->align_line[Align_Line_Right], "elm,state,show",
                          "");
}

static void
cp_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;

   //Dispatch to actual mouse move call
   Ctrl_Pt cp = (Ctrl_Pt) evas_object_data_get(obj, "index");

   //Show Control Point
   live_data *ld = data;
   elm_object_signal_emit(ld->ctrl_pt[cp], "elm,state,show", "");

   switch (cp)
     {
        case Ctrl_Pt_Rel1:
          cp_rel1_mouse_move_cb(data, e, obj, event_info);
          break;
        case Ctrl_Pt_Rel2:
          cp_rel2_mouse_move_cb(data, e, obj, event_info);
          break;
        case Ctrl_Pt_Rel3:
          cp_rel3_mouse_move_cb(data, e, obj, event_info);
          break;
        case Ctrl_Pt_Rel4:
          cp_rel4_mouse_move_cb(data, e, obj, event_info);
          break;
        case Ctrl_Pt_Top:
          cp_top_mouse_move_cb(data, e, obj, event_info);
          break;
        case Ctrl_Pt_Bottom:
          cp_bottom_mouse_move_cb(data, e, obj, event_info);
          break;
        case Ctrl_Pt_Left:
          cp_left_mouse_move_cb(data, e, obj, event_info);
          break;
        case Ctrl_Pt_Right:
          cp_right_mouse_move_cb(data, e, obj, event_info);
          break;
        case Ctrl_Pt_Cnt: //for avoiding compiler warning.
          break;
     }
   live_edit_update_internal(ld);
}

static void
align_lines_hide(live_data *ld)
{
   int i;
   for (i = 0; i < Align_Line_Cnt; i++)
     elm_object_signal_emit(ld->align_line[i], "elm,state,hide", "");
}

static void
cp_mouse_up_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
               void *event_info EINA_UNUSED)
{
   Evas_Event_Mouse_Up *ev = event_info;
   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;

   evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                  cp_mouse_move_cb);
   live_data *ld = data;
   align_lines_hide(ld);

   //Show All Control Points
   int i;
   for (i = 0; i < Ctrl_Pt_Cnt; i++)
     elm_object_signal_emit(ld->ctrl_pt[i], "elm,state,show", "");
}

static void
cp_mouse_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                 void *event_info EINA_UNUSED)
{
   Evas_Event_Mouse_Down *ev = event_info;
   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;

   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                  cp_mouse_move_cb, data);
   evas_object_layer_set(obj, CTRL_PT_LAYER);

   //Hide All Control Points
   live_data *ld = data;
   int i;
   for (i = 0; i < Ctrl_Pt_Cnt; i++)
     elm_object_signal_emit(ld->ctrl_pt[i], "elm,state,hide", "");
}

static void
ctrl_pt_init(live_data *ld)
{
   //Ctrl Point Size
   Evas_Object *edje = elm_layout_edje_get(ld->layout);
   double ctrl_size = atof(edje_object_data_get(edje, "ctrl_size"));
   ctrl_size = ELM_SCALE_SIZE(ctrl_size);
   ld->half_ctrl_size = ctrl_size * 0.5;

   //Create Control Points
   int i;
   for (i = 0; i < Ctrl_Pt_Cnt; i++)
     {
        Evas_Object *layout = elm_layout_add(ld->layout);
        evas_object_smart_member_add(layout, ld->live_view);
        elm_layout_file_set(layout, EDJE_PATH,  "ctrl_pt");
        evas_object_resize(layout, ctrl_size, ctrl_size);
        evas_object_show(layout);
        evas_object_event_callback_add(layout,
                                       EVAS_CALLBACK_MOUSE_DOWN,
                                       cp_mouse_down_cb, ld);
        evas_object_event_callback_add(layout,
                                       EVAS_CALLBACK_MOUSE_UP,
                                       cp_mouse_up_cb, ld);
        evas_object_data_set(layout, "index", (void *) i);

        ld->ctrl_pt[i] = layout;
     }

   //Set Mouse Cursors
   elm_object_cursor_set(ld->ctrl_pt[Ctrl_Pt_Rel1],
                         ELM_CURSOR_TOP_LEFT_CORNER);
   elm_object_cursor_set(ld->ctrl_pt[Ctrl_Pt_Rel2],
                         ELM_CURSOR_BOTTOM_RIGHT_CORNER);
   elm_object_cursor_set(ld->ctrl_pt[Ctrl_Pt_Rel3],
                         ELM_CURSOR_TOP_RIGHT_CORNER);
   elm_object_cursor_set(ld->ctrl_pt[Ctrl_Pt_Rel4],
                         ELM_CURSOR_BOTTOM_LEFT_CORNER);
   elm_object_cursor_set(ld->ctrl_pt[Ctrl_Pt_Top], ELM_CURSOR_TOP_SIDE);
   elm_object_cursor_set(ld->ctrl_pt[Ctrl_Pt_Bottom], ELM_CURSOR_BOTTOM_SIDE);
   elm_object_cursor_set(ld->ctrl_pt[Ctrl_Pt_Left], ELM_CURSOR_LEFT_SIDE);
   elm_object_cursor_set(ld->ctrl_pt[Ctrl_Pt_Right], ELM_CURSOR_RIGHT_SIDE);

   ctrl_pt_update(ld);
}

static void
layout_update(live_data *ld)
{
   Evas_Coord x, y, w, h;
   Evas_Object *view = view_obj_get(ld);
   evas_object_geometry_get(view, &x, &y, &w, &h);

   double x2 = round(w * ld->part_info.rel1_x);
   double y2 = round(h * ld->part_info.rel1_y);
   evas_object_move(ld->layout, (x + x2), (y + y2));
   double w2 =
     round(((double) w * (ld->part_info.rel2_x - ld->part_info.rel1_x)));
   double h2 =
     round(((double) h * (ld->part_info.rel2_y - ld->part_info.rel1_y)));
   evas_object_resize(ld->layout, w2, h2);
}

static void
live_edit_update_internal(live_data *ld)
{
   evas_norender(evas_object_evas_get(ld->layout));
   layout_update(ld);
   ctrl_pt_update(ld);
   align_line_update(ld);
   info_text_update(ld);
}

static void
live_view_geom_cb(void *data, Evas *e EINA_UNUSED,
                  Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   live_data *ld = data;
   live_edit_update_internal(ld);
}

static void
layout_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                     void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;

   live_data *ld = data;

   Evas_Coord vx, vy, vw, vh;  //layout geometry
   Evas_Object *view = view_obj_get(ld);
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   //only affect when cursor is inside of the part
   Evas_Coord x, y, w, h;
   evas_object_geometry_get(obj, &x, &y, &w, &h);

   if (ev->cur.canvas.x > (x + w)) return;
   if (x > ev->cur.canvas.x) return;
   if (ev->cur.canvas.y > (y + h)) return;
   if (y > ev->cur.canvas.y) return;

   x = ev->cur.canvas.x - ld->move_delta.x;
   y = ev->cur.canvas.y - ld->move_delta.y;

   //limit to live view boundary
   if (vx > x) x = vx;
   if ((x + w) > (vx + vw)) x = (vx + vw) - w;
   if (vy > y) y = vy;
   if ((y + h) > (vy + vh)) y -= ((y + h) - (vy + vh));

   double orig_rel1_x = ld->part_info.rel1_x;
   double orig_rel1_y = ld->part_info.rel1_y;
   ld->part_info.rel1_x = ROUNDING(((double) (x - vx) / vw), 2);
   ld->part_info.rel1_y = ROUNDING(((double) (y - vy) / vh), 2);
   ld->part_info.rel2_x += ROUNDING((ld->part_info.rel1_x - orig_rel1_x), 2);
   ld->part_info.rel2_y += ROUNDING((ld->part_info.rel1_y - orig_rel1_y), 2);

   evas_object_move(obj, x, y);

   elm_object_signal_emit(ld->align_line[Align_Line_Top], "elm,state,show", "");
   elm_object_signal_emit(ld->align_line[Align_Line_Bottom], "elm,state,show",
                          "");
   elm_object_signal_emit(ld->align_line[Align_Line_Left], "elm,state,show",
                          "");
   elm_object_signal_emit(ld->align_line[Align_Line_Right], "elm,state,show",
                          "");

   ctrl_pt_update(ld);
   info_text_update(ld);
   align_line_update(ld);
}

static void
layout_mouse_up_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                   void *event_info EINA_UNUSED)
{
   Evas_Event_Mouse_Up *ev = event_info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;

   evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                  layout_mouse_move_cb);
   live_data *ld = data;
   align_lines_hide(ld);

   //Show hidden control points
   elm_object_signal_emit(ld->ctrl_pt[Ctrl_Pt_Top], "elm,state,show", "");
   elm_object_signal_emit(ld->ctrl_pt[Ctrl_Pt_Bottom], "elm,state,show", "");
   elm_object_signal_emit(ld->ctrl_pt[Ctrl_Pt_Left], "elm,state,show", "");
   elm_object_signal_emit(ld->ctrl_pt[Ctrl_Pt_Right], "elm,state,show", "");
}

static void
layout_mouse_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                     void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   live_data *ld = data;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;

   //insert part on double click
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     {
        live_edit_insert(ld);
        live_edit_cancel();
        return;
     }

   /* Store (cursor - obj position) distance.
      And keep this distance while obj is moving. */
   Evas_Coord x, y;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   ld->move_delta.x = ev->canvas.x - x;
   ld->move_delta.y = ev->canvas.y - y;

   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                  layout_mouse_move_cb, data);

   //Hide unnecessary control points
   elm_object_signal_emit(ld->ctrl_pt[Ctrl_Pt_Top], "elm,state,hide", "");
   elm_object_signal_emit(ld->ctrl_pt[Ctrl_Pt_Bottom], "elm,state,hide", "");
   elm_object_signal_emit(ld->ctrl_pt[Ctrl_Pt_Left], "elm,state,hide", "");
   elm_object_signal_emit(ld->ctrl_pt[Ctrl_Pt_Right], "elm,state,hide", "");
}

static void
align_line_init(live_data *ld)
{
   //Create Align Lines
   int i;
   for (i = 0; i < Align_Line_Cnt; i++)
     {
        Evas_Object *layout = elm_layout_add(ld->layout);
        evas_object_smart_member_add(layout, ld->layout);
        elm_layout_file_set(layout, EDJE_PATH,  "ctrl_pt");
        evas_object_show(layout);
        elm_object_signal_emit(layout, "elm,state,hide,instance", "");
        ld->align_line[i] = layout;
     }
}

static void
info_text_init(live_data *ld)
{
   //Create Info Texts
   int i;
   Evas *e = evas_object_evas_get(ld->layout);
   double scale = elm_config_scale_get();
   for (i = 0; i < Info_Text_Cnt; i++)
     {
        Evas_Object *text = evas_object_text_add(e);
        evas_object_smart_member_add(text, ld->live_view);
        evas_object_pass_events_set(text, EINA_TRUE);
        evas_object_layer_set(text, INFO_TEXT_LAYER);
        evas_object_text_font_set(text, LIVE_EDIT_FONT,
                                  ( LIVE_EDIT_FONT_SIZE * scale));
        evas_object_text_style_set(text, EVAS_TEXT_STYLE_OUTLINE);
        evas_object_text_outline_color_set(text, 0, 0, 0, 255);
        evas_object_show(text);
        ld->info_text[i] = text;
     }

   info_text_update(ld);
}

static void
live_edit_layer_set(live_data *ld)
{
   ld->key_down_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                                  key_down_cb,
                                                  ld);
   evas_object_event_callback_add(ld->live_view, EVAS_CALLBACK_RESIZE,
                                  live_view_geom_cb, ld);
   evas_object_event_callback_add(ld->live_view, EVAS_CALLBACK_MOVE,
                                  live_view_geom_cb, ld);

   //Create Live View Layout
   Evas_Object *layout = elm_layout_add(ld->live_view);
   Evas_Object *view_obj = view_obj_get(ld);
   evas_object_smart_member_add(layout, view_obj);
   elm_layout_file_set(layout, EDJE_PATH,  "live_edit_layout");
   evas_object_event_callback_add(layout, EVAS_CALLBACK_MOUSE_DOWN,
                                  layout_mouse_down_cb, ld);
   evas_object_event_callback_add(layout, EVAS_CALLBACK_MOUSE_UP,
                                  layout_mouse_up_cb, ld);
   elm_layout_part_cursor_set(layout, "cursor_body", ELM_CURSOR_FLEUR);
   evas_object_show(layout);

   ld->layout = layout;

   //Initial Layout Geometry
   ld->part_info.rel1_x = LIVE_EDIT_REL1;
   ld->part_info.rel1_y = LIVE_EDIT_REL1;
   ld->part_info.rel2_x = LIVE_EDIT_REL2;
   ld->part_info.rel2_y = LIVE_EDIT_REL2;

   live_edit_symbol_set(ld);
   ctrl_pt_init(ld);
   align_line_init(ld);
   live_edit_update_internal(ld);
   info_text_init(ld);
}

static void
ctxpopup_it_selected_cb(void *data, Evas_Object *obj,
                        void *event_info EINA_UNUSED)
{
   live_data *ld = g_ld;
   ld->part_info.type = (unsigned int) data;
   live_edit_layer_set(ld);

   elm_ctxpopup_dismiss(obj);

   stats_info_msg_update("Double click the part to confirm.");
}

static void
ctxpopup_dismissed_cb(void *data, Evas_Object *obj,
                      void *event_info EINA_UNUSED)
{
   live_data *ld = data;
   if (!ld->layout) live_edit_cancel();
   evas_object_focus_set(ld->live_view, EINA_TRUE);
   evas_object_del(obj);
   ld->ctxpopup = NULL;
}

static Evas_Object *
ctxpopup_create(live_data *ld)
{
   const int CTXPOPUP_ITEMS_NUM = 6;
   int i;
   Evas_Object *ctxpopup = elm_ctxpopup_add(ld->live_view);
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
live_edit_update(void)
{
   if (!live_edit_get()) return;

   live_data *ld = g_ld;

   //Scale up/down of the symbol object.
   Evas_Object *layout_symbol =
      elm_object_part_content_get(ld->layout, "elm.swallow.symbol");
   if (layout_symbol)
     elm_object_scale_set(layout_symbol, config_view_scale_get());

   live_edit_update_internal(ld);
}

void
live_edit_toggle(void)
{
   live_data *ld = g_ld;
   Eina_Bool on = !ld->on;

   if (on)
     {
        enventor_object_disabled_set(ld->enventor, EINA_TRUE);
        ld->live_view = enventor_object_live_view_get(ld->enventor);
        ld->ctxpopup = ctxpopup_create(ld);
        stats_info_msg_update("Select a part to add in Live View.");
        tools_live_update(EINA_TRUE);
     }
   else
     live_edit_cancel();

   ld->on = on;
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

   if (ld->ctxpopup) elm_ctxpopup_dismiss(ld->ctxpopup);

   enventor_object_disabled_set(ld->enventor, EINA_FALSE);

   ecore_event_handler_del(ld->key_down_handler);
   ld->key_down_handler = NULL;

   evas_object_event_callback_del(ld->live_view, EVAS_CALLBACK_RESIZE,
                                  live_view_geom_cb);
   evas_object_event_callback_del(ld->live_view, EVAS_CALLBACK_MOVE,
                                  live_view_geom_cb);
   ld->live_view = NULL;

   evas_object_del(ld->layout);
   ld->layout = NULL;

   //Delete Control Points
   int i;
   for (i = 0; i < Ctrl_Pt_Cnt; i++)
     {
        evas_object_del(ld->ctrl_pt[i]);
        ld->ctrl_pt[i] = NULL;
     }

   //Delete Align Lines
   for (i = 0; i < Align_Line_Cnt; i++)
     {
        evas_object_del(ld->align_line[i]);
        ld->align_line[i] = NULL;
     }

   //Delete Info Texts
   for (i = 0; i < Info_Text_Cnt; i++)
     {
        evas_object_del(ld->info_text[i]);
        ld->info_text[i] = NULL;
     }

   ld->on = EINA_FALSE;

   tools_live_update(EINA_FALSE);
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
   ld->ctxpopup = NULL;
   live_edit_cancel();
   free(ld);
   g_ld = NULL;
}
