#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Elementary_Cursor.h>
#include "common.h"

#define ALIGN_LINES_CNT 4

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
   Evas_Object *align_line[ALIGN_LINES_CNT];
   double half_ctrl_size;

   struct {
      unsigned int type;
      float rel1_x, rel1_y;
      float rel2_x, rel2_y;
      Evas_Coord x, y, w, h;
   } part_info;

   Ecore_Event_Handler *key_down_handler;

   Eina_Bool on : 1;
} live_data;

static void live_edit_update(live_data *ld);

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
text_update(live_data *ld)
{
   Evas_Object *layout = elm_layout_edje_get(ld->layout);

   char part_info[LIVE_EDIT_NEW_PART_DATA_MAX_LEN];

   snprintf(part_info,
            LIVE_EDIT_NEW_PART_REL_STR_MAX_LEN, LIVE_EDIT_NEW_PART_REL_STR,
            ld->part_info.rel1_x, ld->part_info.rel1_y);
   edje_object_part_text_set(layout, "elm.text.rel1", part_info);
   snprintf(part_info,
            LIVE_EDIT_NEW_PART_REL_STR_MAX_LEN, LIVE_EDIT_NEW_PART_REL_STR,
            ld->part_info.rel2_x, ld->part_info.rel2_y);
   edje_object_part_text_set(layout, "elm.text.rel2", part_info);
}

static void
live_edit_symbol_set(live_data *ld)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s_bg", CTXPOPUP_ITEMS[ld->part_info.type].name);
   Evas_Object *layout_symbol = elm_layout_add(ld->layout);
   elm_layout_file_set(layout_symbol, EDJE_PATH, buf);
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
   Evas_Coord x, y, w, h;
   evas_object_geometry_get(ld->layout, &x, &y, &w, &h);

   int half_ctrl_size = ld->half_ctrl_size;

   //Rel1
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Rel1],
                    (x - half_ctrl_size), (y - half_ctrl_size));

   //Rel2
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Rel2],
                    ((x + w) - half_ctrl_size), ((y + h) - half_ctrl_size));

   //Rel3
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Rel3],
                    ((x + w) - half_ctrl_size), (y - half_ctrl_size));

   //Rel4
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Rel4],
                    (x - half_ctrl_size), ((y + h) - half_ctrl_size));

   //Top
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Top],
                    ((x + (w/2)) - half_ctrl_size), (y - half_ctrl_size));

   //Bottom
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Bottom],
                    ((x + (w/2)) - half_ctrl_size), ((y + h) - half_ctrl_size));

   //Left
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Left],
                    (x - half_ctrl_size), ((y + (h/2)) - half_ctrl_size));

   //Right
   evas_object_move(ld->ctrl_pt[Ctrl_Pt_Right],
                    ((x + w) - half_ctrl_size), ((y + (h/2)) - half_ctrl_size));
}

static void
cp_top_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                     void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord y = ev->cur.canvas.y;

   //Limit to boundary
   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->live_view, &lx, &ly, &lw, &lh);

   Evas_Coord rel2_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], NULL, &rel2_y,
                            NULL, NULL);
   if (ly > y) y = ly;
   if ((y - ld->half_ctrl_size) > rel2_y) y = (rel2_y + ld->half_ctrl_size);

   ld->part_info.rel1_y = ((double) (y - ly) / (double) lh);

   //Align Line
   evas_object_move(ld->align_line[0], lx + 1, y - 1);
   evas_object_resize(ld->align_line[0], lw - 2, 1);
   evas_object_show(ld->align_line[0]);
}

static void
cp_bottom_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                       void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord y = ev->cur.canvas.y;

   //Limit to boundary
   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->live_view, &lx, &ly, &lw, &lh);

   Evas_Coord rel1_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], NULL, &rel1_y,
                            NULL, NULL);
   if (y > (ly + lh)) y = (ly + lh);
   if (rel1_y > (y + ld->half_ctrl_size)) y = (rel1_y - ld->half_ctrl_size);

   ld->part_info.rel2_y = ((double) (y - ly) / (double) lh);

   //Align Line
   evas_object_move(ld->align_line[0], lx + 1, y - 1);
   evas_object_resize(ld->align_line[0], lw - 2, 1);
   evas_object_show(ld->align_line[0]);
}

static void
cp_rel1_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                      void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x = ev->cur.canvas.x;
   Evas_Coord y = ev->cur.canvas.y;

   //Limit to boundary
   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->live_view, &lx, &ly, &lw, &lh);

   Evas_Coord rel2_x, rel2_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], &rel2_x, &rel2_y,
                            NULL, NULL);
   if (lx > x) x = lx;
   if (ly > y) y = ly;
   if ((x - ld->half_ctrl_size) > rel2_x) x = (rel2_x + ld->half_ctrl_size);
   if ((y - ld->half_ctrl_size) > rel2_y) y = (rel2_y + ld->half_ctrl_size);

   ld->part_info.rel1_x = ((double) (x - lx) / (double) lw);
   ld->part_info.rel1_y = ((double) (y - ly) / (double) lh);

   //Align Lines

   //Horizontal
   evas_object_move(ld->align_line[0], lx + 1, y - 1);
   evas_object_resize(ld->align_line[0], lw - 2, 1);
   evas_object_show(ld->align_line[0]);

   //Vertical
   evas_object_move(ld->align_line[1], x - 1, ly + 1);
   evas_object_resize(ld->align_line[1], 1, lh - 2);
   evas_object_show(ld->align_line[1]);
}

static void
cp_rel2_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                      void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x = ev->cur.canvas.x;
   Evas_Coord y = ev->cur.canvas.y;

   //Limit to boundary
   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->live_view, &lx, &ly, &lw, &lh);

   Evas_Coord rel1_x, rel1_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], &rel1_x, &rel1_y,
                            NULL, NULL);
   if (x > (lx + lw)) x = (lx + lw);
   if (y > (ly + lh)) y = (ly + lh);
   if (rel1_x > (x + ld->half_ctrl_size)) x = (rel1_x - ld->half_ctrl_size);
   if (rel1_y > (y + ld->half_ctrl_size)) y = (rel1_y - ld->half_ctrl_size);

   ld->part_info.rel2_x = ((double) (x - lx) / (double) lw);
   ld->part_info.rel2_y = ((double) (y - ly) / (double) lh);

   //Align Lines

   //Horizontal
   evas_object_move(ld->align_line[0], lx + 1, y - 1);
   evas_object_resize(ld->align_line[0], lw - 2, 1);
   evas_object_show(ld->align_line[0]);

   //Vertical
   evas_object_move(ld->align_line[1], x - 1, ly + 1);
   evas_object_resize(ld->align_line[1], 1, lh - 2);
   evas_object_show(ld->align_line[1]);
}

static void
cp_rel3_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                      void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x = ev->cur.canvas.x;
   Evas_Coord y = ev->cur.canvas.y;

   //Limit to boundary
   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->live_view, &lx, &ly, &lw, &lh);

   Evas_Coord rel1_x;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], &rel1_x, NULL,
                            NULL, NULL);
   if (x > (lx + lw)) x = (lx + lw);
   if (rel1_x > (x + ld->half_ctrl_size)) x = (rel1_x - ld->half_ctrl_size);

   Evas_Coord rel2_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], NULL, &rel2_y,
                            NULL, NULL);
   if (ly > y) y = ly;
   if ((y - ld->half_ctrl_size) > rel2_y) y = (rel2_y + ld->half_ctrl_size);

   ld->part_info.rel2_x = ((double) (x - lx) / (double) lw);
   ld->part_info.rel1_y = ((double) (y - ly) / (double) lh);

   //Align Lines

   //Horizontal
   evas_object_move(ld->align_line[0], lx + 1, y - 1);
   evas_object_resize(ld->align_line[0], lw - 2, 1);
   evas_object_show(ld->align_line[0]);

   //Vertical
   evas_object_move(ld->align_line[1], x - 1, ly + 1);
   evas_object_resize(ld->align_line[1], 1, lh - 2);
   evas_object_show(ld->align_line[1]);
}

static void
cp_rel4_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                      void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x = ev->cur.canvas.x;
   Evas_Coord y = ev->cur.canvas.y;

   //Limit to boundary
   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->live_view, &lx, &ly, &lw, &lh);

   Evas_Coord rel2_x;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], &rel2_x, NULL,
                            NULL, NULL);
   if (lx > x) x = lx;
   if ((x - ld->half_ctrl_size) > rel2_x) x = (rel2_x + ld->half_ctrl_size);

   Evas_Coord rel1_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], NULL, &rel1_y,
                            NULL, NULL);
   if (y > (ly + lh)) y = (ly + lh);
   if (rel1_y > (y + ld->half_ctrl_size)) y = (rel1_y - ld->half_ctrl_size);

   ld->part_info.rel1_x = ((double) (x - lx) / (double) lw);
   ld->part_info.rel2_y = ((double) (y - ly) / (double) lh);

   //Align Lines

   //Horizontal
   evas_object_move(ld->align_line[0], lx + 1, y - 1);
   evas_object_resize(ld->align_line[0], lw - 2, 1);
   evas_object_show(ld->align_line[0]);

   //Vertical
   evas_object_move(ld->align_line[1], x - 1, ly + 1);
   evas_object_resize(ld->align_line[1], 1, lh - 2);
   evas_object_show(ld->align_line[1]);
}

static void
cp_left_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                      void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x = ev->cur.canvas.x;

   //Limit to boundary
   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->live_view, &lx, &ly, &lw, &lh);

   Evas_Coord rel2_x;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], &rel2_x, NULL,
                            NULL, NULL);
   if (lx > x) x = lx;
   if ((x - ld->half_ctrl_size) > rel2_x) x = (rel2_x + ld->half_ctrl_size);

   ld->part_info.rel1_x = ((double) (x - lx) / (double) lw);

   //Align Line
   evas_object_move(ld->align_line[0], x - 1, ly + 1);
   evas_object_resize(ld->align_line[0], 1, lh - 2);
   evas_object_show(ld->align_line[0]);
}

static void
cp_right_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                       void *event_info)
{
   live_data *ld = data;
   Evas_Event_Mouse_Move *ev = event_info;

   Evas_Coord x = ev->cur.canvas.x;

   //Limit to boundary
   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->live_view, &lx, &ly, &lw, &lh);

   Evas_Coord rel1_x;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], &rel1_x, NULL,
                            NULL, NULL);
   if (x > (lx + lw)) x = (lx + lw);
   if (rel1_x > (x + ld->half_ctrl_size)) x = (rel1_x - ld->half_ctrl_size);

   ld->part_info.rel2_x = ((double) (x - lx) / (double) lw);

   //Align Line
   evas_object_move(ld->align_line[0], x - 1, ly + 1);
   evas_object_resize(ld->align_line[0], 1, lh - 2);
   evas_object_show(ld->align_line[0]);
}

static void
cp_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
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
     }
   live_edit_update(ld);
}

static void
align_lines_hide(live_data *ld)
{
   int i;
   for (i = 0; i < ALIGN_LINES_CNT; i++)
     evas_object_hide(ld->align_line[i]);
}

static void
cp_mouse_up_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
               void *event_info EINA_UNUSED)
{
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
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                  cp_mouse_move_cb, data);
   evas_object_layer_set(obj, EVAS_LAYER_MAX);

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
   ctrl_size *= elm_config_scale_get();
   ld->half_ctrl_size = ctrl_size * 0.5;

   //Create Control Points
   int i;
   for (i = 0; i < Ctrl_Pt_Cnt; i++)
     {
        Evas_Object *layout = elm_layout_add(ld->layout);
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

   evas_object_geometry_get(ld->live_view, &x, &y, &w, &h);
   Evas_Coord x2 = (w * ld->part_info.rel1_x);
   Evas_Coord y2 = (h * ld->part_info.rel1_y);
   evas_object_move(ld->layout, (x + x2), (y + y2));
   Evas_Coord w2 = (w * (ld->part_info.rel2_x)) - x2;
   Evas_Coord h2 = (h * (ld->part_info.rel2_y)) - y2;
   evas_object_resize(ld->layout, w2, h2);
}

static void
live_edit_update(live_data *ld)
{
   layout_update(ld);
   ctrl_pt_update(ld);
   text_update(ld);
}

static void
live_view_geom_cb(void *data, Evas *e EINA_UNUSED,
                  Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   live_data *ld = data;
   live_edit_update(ld);
}

static void
layout_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                     void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   live_data *ld = data;

   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->live_view, &lx, &ly, &lw, &lh);

   Evas_Coord x, y, w, h;
   evas_object_geometry_get(obj, &x, &y, &w, &h);

   //only affect when cursor is inside of the part
   if (ev->cur.canvas.x > (x + w)) return;
   if (x > ev->cur.canvas.x) return;
   if (ev->cur.canvas.y > (y + h)) return;
   if (y > ev->cur.canvas.y) return;

   x = ((double) x) + ((double) (ev->cur.canvas.x - ev->prev.canvas.x) * 0.5);
   y = ((double) y) + ((double) (ev->cur.canvas.y - ev->prev.canvas.y) * 0.5);

   //limit to live view boundary
   if (lx > x) x = lx;
   if ((x + w) > (lx + lw)) x -= ((x + w) - (lx + lw));
   if (ly > y) y = ly;
   if ((y + h) > (ly + lh)) y -= ((y + h) - (ly + lh));

   double orig_rel1_x = ld->part_info.rel1_x;
   double orig_rel1_y = ld->part_info.rel1_y;
   ld->part_info.rel1_x = ((double) (x - lx) / lw);
   ld->part_info.rel1_y = ((double) (y - ly) / lh);
   ld->part_info.rel2_x += (ld->part_info.rel1_x - orig_rel1_x);
   ld->part_info.rel2_y += (ld->part_info.rel1_y - orig_rel1_y);

   evas_object_move(obj, x, y);

   text_update(ld);
   ctrl_pt_update(ld);

   //Align Lines

   //Rel1 Horizontal
   evas_object_move(ld->align_line[0], lx + 1, y);
   evas_object_resize(ld->align_line[0], lw - 2, 1);
   evas_object_show(ld->align_line[0]);

   //Rel1 Vertical
   evas_object_move(ld->align_line[1], x, ly + 1);
   evas_object_resize(ld->align_line[1], 1, lh - 2);
   evas_object_show(ld->align_line[1]);

   //Rel2 Horizontal
   evas_object_move(ld->align_line[2], lx + 1, y + h);
   evas_object_resize(ld->align_line[2], lw - 2, 1);
   evas_object_show(ld->align_line[2]);

   //Rel2 Vertical
   evas_object_move(ld->align_line[3], x + w, ly + 1);
   evas_object_resize(ld->align_line[3], 1, lh - 2);
   evas_object_show(ld->align_line[3]);
}

static void
layout_mouse_up_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                   void *event_info EINA_UNUSED)
{
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

   //insert part on double click
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     {
        live_edit_insert(ld);
        live_edit_cancel();
        return;
     }

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
   for (i = 0; i < ALIGN_LINES_CNT; i++)
     {
        Evas_Object *layout = elm_layout_add(ld->layout);
        elm_layout_file_set(layout, EDJE_PATH,  "ctrl_pt");
        ld->align_line[i] = layout;
     }
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

   live_edit_update(ld);
   live_edit_symbol_set(ld);
   ctrl_pt_init(ld);
   align_line_init(ld);
}

static void
ctxpopup_it_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   live_data *ld = g_ld;
   const Elm_Object_Item *it = event_info;
   ld->part_info.type = (unsigned int) data;
   live_edit_layer_set(ld);
   elm_ctxpopup_dismiss(obj);

   stats_info_msg_update("Click and drag the mouse in the Live View.");
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
     }
   else
     {
        live_edit_cancel();
        stats_info_msg_update("Live View Edit Mode Disabled.");
     }

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
   for (i = 0; i < ALIGN_LINES_CNT; i++)
     {
        evas_object_del(ld->align_line[i]);
        ld->align_line[i] = NULL;
     }

   ld->on = EINA_FALSE;
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
