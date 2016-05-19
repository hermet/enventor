#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Elementary_Cursor.h>
#include "common.h"

#define CTRL_PT_LAYER 3
#define INFO_TEXT_LAYER (CTRL_PT_LAYER+1)
#define PART_NAME_MAX 1024

typedef struct livedit_item_s
{
   const char *name;
   Edje_Part_Type type;
} liveedit_item;

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

typedef struct live_editor_s
{
   Evas_Object *toolbox;
   Evas_Object *layout;
   Evas_Object *live_view;
   Evas_Object *ctrl_pt[Ctrl_Pt_Cnt];
   Evas_Object *align_line[Align_Line_Cnt];
   Evas_Object *info_text[Info_Text_Cnt];
   Evas_Coord_Point move_delta;
   double half_ctrl_size;

   unsigned int type;

   //Live edit item information
   struct {
      float rel1_x, rel1_y;
      float rel2_x, rel2_y;
   } rel_info;

   //Relative to information for live edit item
   struct {
      char *rel1_x_to;
      char *rel1_y_to;
      char *rel2_x_to;
      char *rel2_y_to;
      float align_x, align_y;
      float rel1_to_x, rel1_to_y;
      float rel2_to_x, rel2_to_y;
   } rel_to_info;

   Evas_Object *keygrabber;
   Eina_Array *auto_align_array;
   Ctrl_Pt last_cp;
   unsigned int auto_align_dist;

   //Relative setting properties
   Evas_Object *fixed_ctxpopup;
   Evas_Object *rel_to_ctxpopup;
   float rel1_x, rel1_y;
   float rel2_x, rel2_y;
   Eina_Bool fixed_w : 1;
   Eina_Bool fixed_h : 1;

   Eina_Bool on : 1;
} live_data;

typedef struct auto_align_data_s
{
   char part_name[PART_NAME_MAX];
   Evas_Coord_Point pt1;
   Evas_Coord_Point pt2;
} auto_align_data;

typedef struct rel_to_data_s
{
  char part_name[PART_NAME_MAX];
  int is_rel_to_x, is_rel_to_y;
  float rel_x, rel_y;
  Evas_Coord_Point pt1, pt2;
  live_data *ld;
} rel_to_data;

static void live_edit_update_internal(live_data *ld);

#define LIVEEDIT_ITEMS_NUM 6

static live_data *g_ld = NULL;

static const liveedit_item LIVEEDIT_ITEMS[] =
{
     {"Rect", EDJE_PART_TYPE_RECTANGLE},
     {"Text", EDJE_PART_TYPE_TEXT},
     {"Image", EDJE_PART_TYPE_IMAGE},
     {"Swallow", EDJE_PART_TYPE_SWALLOW},
     {"Textblock", EDJE_PART_TYPE_TEXTBLOCK},
     {"Spacer", EDJE_PART_TYPE_SPACER} //Please leave spacer at last
};

static Evas_Object *
view_scroller_get(live_data *ld)
{
   //This is a trick! we got the actual view object from the live edit.
   if (!ld->live_view) return NULL;
   return elm_object_part_content_get(ld->live_view,
                                      "elm.swallow.content");
}

static Evas_Object *
view_obj_get(live_data *ld)
{
   //This is a trick! we got the actual view object from the live edit.
   Evas_Object *o = view_scroller_get(ld);
   Evas_Object *o2 = elm_object_content_get(o);
   return elm_object_part_content_get(o2, "elm.swallow.content");
}

static void
view_scroll_cb(void *data, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   live_data *ld = data;
   live_edit_update_internal(ld);
}

static void
info_text_update(live_data *ld)
{
   //Update Text
   char buf[256];

   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->live_view, &lx, &ly, &lw, &lh);

   Enventor_Object *enventor = base_enventor_get();

   //reverse coordinates if mirror mode is enabled.
   double ox = ld->rel_info.rel1_x;
   double ox2 = ld->rel_info.rel2_x;
   double ow = (ld->rel_info.rel1_x * (double) lw);
   double ow2 = (ld->rel_info.rel2_x * (double) lw);

   if (enventor_object_mirror_mode_get(enventor))
     {
        ox = 1 - ox;
        ox2 = 1 - ox2;
        ow = lw - ow;
        ow2 = lw - ow2;
     }

   //Rel1
   snprintf(buf, sizeof(buf), "%.2f %.2f (%d, %d)",
            ox, ld->rel_info.rel1_y,
            (int) round(ow),
            (int) round(ld->rel_info.rel1_y * (double) lh));
   evas_object_text_text_set(ld->info_text[Info_Text_Rel1], buf);

   //Rel2
   snprintf(buf, sizeof(buf), "%.2f %.2f (%d, %d)",
            ox2, ld->rel_info.rel2_y,
            (int) round(ow2),
            (int) round(ld->rel_info.rel2_y * (double) lh));
   evas_object_text_text_set(ld->info_text[Info_Text_Rel2], buf);

   //Size
   Evas_Coord vw, vh;
   config_view_size_get(&vw, &vh);

   vw = (Evas_Coord) (((double) vw) *
                      (ld->rel_info.rel2_x - ld->rel_info.rel1_x));
   vh = (Evas_Coord) (((double) vh) *
                      (ld->rel_info.rel2_y - ld->rel_info.rel1_y));
   snprintf(buf, sizeof(buf), "[%d x %d]", vw, vh);
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
   Evas_Coord layout_x, layout_y, layout_w, layout_h;
   evas_object_geometry_get(ld->layout, &layout_x, &layout_y, &layout_w,
                            &layout_h);
   evas_object_geometry_get(ld->info_text[Info_Text_Size], NULL, NULL, &w, &h);
   x = (layout_x + (layout_w/2)) - (w/2);
   y = (layout_y + (layout_h/2)) - (h/2);
   if (x < lx) x = lx;
   if (y < ly) y = ly;
   if ((x + w) > (lx + lw)) x = ((lx + lw) - w);
   if ((y + h) > (ly + lh)) y = ((ly + lh) - h);
   evas_object_move(ld->info_text[Info_Text_Size], x, y);
}

static void
live_edit_symbol_set(live_data *ld)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s_bg", LIVEEDIT_ITEMS[ld->type].name);
   Evas_Object *layout_symbol = elm_layout_add(ld->layout);
   elm_layout_file_set(layout_symbol, EDJE_PATH, buf);
   elm_object_scale_set(layout_symbol, config_view_scale_get());
   elm_object_part_content_set(ld->layout, "elm.swallow.symbol", layout_symbol);
}

static void
live_edit_insert(live_data *ld)
{
   int type = LIVEEDIT_ITEMS[ld->type].type;

   //Set relative_to values
   if (!ld->rel_to_info.rel1_x_to)
     ld->rel_to_info.rel1_to_x = ld->rel_info.rel1_x;
   if (!ld->rel_to_info.rel1_y_to)
     ld->rel_to_info.rel1_to_y = ld->rel_info.rel1_y;
   if (!ld->rel_to_info.rel2_x_to)
     ld->rel_to_info.rel2_to_x = ld->rel_info.rel2_x;
   if (!ld->rel_to_info.rel2_y_to)
     ld->rel_to_info.rel2_to_y = ld->rel_info.rel2_y;

   //Calculate min size
   Evas_Coord vw, vh;
   config_view_size_get(&vw, &vh);

   Evas_Coord min_w = (Evas_Coord) (((double) vw) *
                      (ld->rel_info.rel2_x - ld->rel_info.rel1_x));
   Evas_Coord min_h = (Evas_Coord) (((double) vh) *
                      (ld->rel_info.rel2_y - ld->rel_info.rel1_y));

   enventor_object_template_part_insert(base_enventor_get(),
                                        type,
                                        ENVENTOR_TEMPLATE_INSERT_LIVE_EDIT,
                                        ld->fixed_w,
                                        ld->fixed_h,
                                        ld->rel_to_info.rel1_x_to,
                                        ld->rel_to_info.rel1_y_to,
                                        ld->rel_to_info.rel2_x_to,
                                        ld->rel_to_info.rel2_y_to,
                                        ld->rel_to_info.align_x,
                                        ld->rel_to_info.align_y,
                                        min_w,
                                        min_h,
                                        ld->rel_to_info.rel1_to_x,
                                        ld->rel_to_info.rel1_to_y,
                                        ld->rel_to_info.rel2_to_x,
                                        ld->rel_to_info.rel2_to_y,
                                        NULL, 0);
   enventor_object_save(base_enventor_get(), config_input_path_get());

   if (ld->rel_to_info.rel1_x_to)
     free(ld->rel_to_info.rel1_x_to);
   if (ld->rel_to_info.rel1_y_to)
     free(ld->rel_to_info.rel1_y_to);
   if (ld->rel_to_info.rel2_x_to)
     free(ld->rel_to_info.rel2_x_to);
   if (ld->rel_to_info.rel2_y_to)
     free(ld->rel_to_info.rel2_y_to);
}

static void
keygrabber_key_down_cb(void *data, Evas *e EINA_UNUSED,
                       Evas_Object *obj EINA_UNUSED, void *event_info)
{
   live_data *ld = data;
   Evas_Event_Key_Down *ev = event_info;

   if (!strcmp(ev->key, "Return")) live_edit_insert(ld);
   else if (strcmp(ev->key, "Delete") &&
            strcmp(ev->key, "BackSpace")) return;

   live_edit_cancel();
}

Evas_Coord_Point
calc_ctrl_pt_auto_align_pos(live_data *ld, int cursor_x, int cursor_y)
{
   int res_x, res_y;
   unsigned int nx, ny;
   res_x = res_y = -1;
   nx = ny = LIVE_EDIT_MAX_DIST;
   unsigned int dist = ld->auto_align_dist;

   // This loop finds the closest position of part to control point
   // And then return the position
   unsigned int i;
   auto_align_data *al_pos;
   Eina_Array_Iterator iter;
   EINA_ARRAY_ITER_NEXT(ld->auto_align_array, i, al_pos, iter)
     {
        unsigned int dx, dy;

        dx = abs(al_pos->pt1.x - cursor_x);
        dy = abs(al_pos->pt1.y - cursor_y);

        if ((dx < dist) && (dx < nx) && (cursor_y >= al_pos->pt1.y) &&
            (cursor_y <= al_pos->pt2.y))
          {
             nx = dx;
             res_x = al_pos->pt1.x;
          }
        if ((dy < dist) && (dy < ny) && (cursor_x >= al_pos->pt1.x) &&
            (cursor_x <= al_pos->pt2.x))
          {
             ny = dy;
             res_y = al_pos->pt1.y;
          }

        dx = abs(al_pos->pt2.x - cursor_x);
        dy = abs(al_pos->pt2.y - cursor_y);

        if ((dx < dist) && (dx < nx) && (cursor_y >= al_pos->pt1.y) &&
            (cursor_y <= al_pos->pt2.y))
          {
             nx = dx;
             res_x = al_pos->pt2.x;
          }
        if ((dy < dist) && (dy < ny) && (cursor_x >= al_pos->pt1.x) &&
            (cursor_x <= al_pos->pt2.x))
          {
             ny = dy;
             res_y = al_pos->pt2.y;
          }
     }

   Evas_Coord_Point pt;

   if (res_x != -1)
     pt.x = res_x;
   else
     pt.x = cursor_x;

   if (res_y != -1)
     pt.y = res_y;
   else
     pt.y = cursor_y;

   return pt;
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


   Evas_Coord x = ev->cur.canvas.x;
   Evas_Coord y = ev->cur.canvas.y;

   Evas_Object *view = view_obj_get(ld);
   Evas_Coord vx, vy, vw, vh;
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   Evas_Coord rel2_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], NULL, &rel2_y,
                            NULL, NULL);


   Evas_Coord_Point pt = calc_ctrl_pt_auto_align_pos(ld, x, y);
   y = pt.y;

   if (vy > y) y = vy;
   if ((y - ld->half_ctrl_size) > rel2_y) y = (rel2_y + ld->half_ctrl_size);

   ld->rel_info.rel1_y = ROUNDING(((double) (y - vy) / (double) vh), 2);

   elm_object_signal_emit(ld->align_line[Align_Line_Top], "elm,state,show", "");
}

static void
cp_bottom_mouse_move_cb(void *data, Evas *e EINA_UNUSED,
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

   Evas_Coord rel1_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], NULL, &rel1_y,
                            NULL, NULL);

   Evas_Coord_Point pt = calc_ctrl_pt_auto_align_pos(ld, x, y);
   y = pt.y;


   if (y > (vy + vh)) y = (vy + vh);
   if (rel1_y > (y + ld->half_ctrl_size)) y = (rel1_y + ld->half_ctrl_size);

   ld->rel_info.rel2_y = ROUNDING(((double) (y - vy) / (double) vh), 2);

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
keygrabber_direction_key_down_cb(void *data, Evas *e EINA_UNUSED,
                       Evas_Object *obj EINA_UNUSED, void *event_info)
{
   live_data *ld = data;
   Evas_Event_Key_Down *ev = event_info;

   Evas_Coord x, y, w, h;
   evas_object_geometry_get(ld->layout, &x, &y, &w, &h);

   Evas_Coord vx, vy, vw, vh;
   Evas_Object *view = view_obj_get(ld);
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   //Move the live item by 1 pixel to set detailed
   //Move up
   if (!strcmp(ev->key, "Up"))
     y -= 1;
   //Move down
   else if (!strcmp(ev->key, "Down"))
     y += 1;
   //Move left
   else if (!strcmp(ev->key, "Left"))
     x -= 1;
   //Move Right
   else if (!strcmp(ev->key, "Right"))
     x += 1;

   //Check live view boundary
   if (vx > x) x = vx;
   if ((x + w) > (vx + vw)) x = (vx + vw) - w;
   if (vy > y) y = vy;
   if ((y + h) > (vy + vh)) y -= ((y + h) - (vy + vh));

   evas_object_move(ld->layout, x, y);

   //Calculate the relative value of live view item to 4 places of decimals
   double orig_rel1_x = ld->rel_info.rel1_x;
   double orig_rel1_y = ld->rel_info.rel1_y;
   ld->rel_info.rel1_x = ROUNDING(((double) (x - vx) / vw), 4);
   ld->rel_info.rel1_y = ROUNDING(((double) (y - vy) / vh), 4);
   ld->rel_info.rel2_x += ROUNDING((ld->rel_info.rel1_x - orig_rel1_x), 4);
   ld->rel_info.rel2_y += ROUNDING((ld->rel_info.rel1_y - orig_rel1_y), 4);

   ctrl_pt_update(ld);
   info_text_update(ld);
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

   Evas_Coord_Point pt = calc_ctrl_pt_auto_align_pos(ld, x, y);
   x = pt.x;
   y = pt.y;


   if (vx > x) x = vx;
   if (vy > y) y = vy;
   if ((x - ld->half_ctrl_size) > rel2_x) x = (rel2_x + ld->half_ctrl_size);
   if ((y - ld->half_ctrl_size) > rel2_y) y = (rel2_y + ld->half_ctrl_size);

   ld->rel_info.rel1_x = ROUNDING(((double) (x - vx) / (double) vw), 2);
   ld->rel_info.rel1_y = ROUNDING(((double) (y - vy) / (double) vh), 2);

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

   Evas_Coord_Point pt = calc_ctrl_pt_auto_align_pos(ld, x, y);
   x = pt.x;
   y = pt.y;

   if (x > (vx + vw)) x = (vx + vw);
   if (y > (vy + vh)) y = (vy + vh);
   if (rel1_x > (x + ld->half_ctrl_size)) x = (rel1_x + ld->half_ctrl_size);
   if (rel1_y > (y + ld->half_ctrl_size)) y = (rel1_y + ld->half_ctrl_size);

   ld->rel_info.rel2_x = ROUNDING(((double) (x - vx) / (double) vw), 2);
   ld->rel_info.rel2_y = ROUNDING(((double) (y - vy) / (double) vh), 2);

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

   Evas_Coord_Point pt = calc_ctrl_pt_auto_align_pos(ld, x, y);
   x = pt.x;
   y = pt.y;

   if (x > (vx + vw)) x = (vx + vw);
   if (rel1_x > (x + ld->half_ctrl_size)) x = (rel1_x - ld->half_ctrl_size);

   Evas_Coord rel2_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], NULL, &rel2_y,
                            NULL, NULL);
   if (vy > y) y = vy;
   if ((y - ld->half_ctrl_size) > rel2_y) y = (rel2_y + ld->half_ctrl_size);

   ld->rel_info.rel2_x = ROUNDING(((double) (x - vx) / (double) vw), 2);
   ld->rel_info.rel1_y = ROUNDING(((double) (y - vy) / (double) vh), 2);

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

   Evas_Coord_Point pt = calc_ctrl_pt_auto_align_pos(ld, x, y);
   x = pt.x;
   y = pt.y;

   if (vx > x) x = vx;
   if ((x - ld->half_ctrl_size) > rel2_x) x = (rel2_x + ld->half_ctrl_size);

   Evas_Coord rel1_y;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], NULL, &rel1_y,
                            NULL, NULL);
   if (y > (vy + vh)) y = (vy + vh);
   if (rel1_y > (y + ld->half_ctrl_size)) y = (rel1_y + ld->half_ctrl_size);

   ld->rel_info.rel1_x = ROUNDING(((double) (x - vx) / (double) vw), 2);
   ld->rel_info.rel2_y = ROUNDING(((double) (y - vy) / (double) vh), 2);

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
   Evas_Coord y = ev->cur.canvas.y;

   Evas_Object *view = view_obj_get(ld);
   Evas_Coord vx, vy, vw, vh;
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   Evas_Coord rel2_x;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel2], &rel2_x, NULL,
                            NULL, NULL);

   Evas_Coord_Point pt = calc_ctrl_pt_auto_align_pos(ld, x, y);
   x = pt.x;


   if (vx > x) x = vx;
   if ((x - ld->half_ctrl_size) > rel2_x) x = (rel2_x + ld->half_ctrl_size);

   ld->rel_info.rel1_x = ROUNDING(((double) (x - vx) / (double) vw), 2);

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
   Evas_Coord y = ev->cur.canvas.y;

   Evas_Object *view = view_obj_get(ld);
   Evas_Coord vx, vy, vw, vh;
   evas_object_geometry_get(view, &vx, &vy, &vw, &vh);

   Evas_Coord rel1_x;
   evas_object_geometry_get(ld->ctrl_pt[Ctrl_Pt_Rel1], &rel1_x, NULL,
                            NULL, NULL);


   Evas_Coord_Point pt = calc_ctrl_pt_auto_align_pos(ld, x, y);
   x = pt.x;


   if (x > (vx + vw)) x = (vx + vw);
   if (rel1_x > (x + ld->half_ctrl_size)) x = (rel1_x + ld->half_ctrl_size);

   ld->rel_info.rel2_x = ROUNDING(((double) (x - vx) / (double) vw), 2);

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
   if (cp == Ctrl_Pt_Cnt) return; //not to use Ctrl_Pt_Cnt as index.

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
        default:
          break;
     }

   //Memorize last selected control point for setting relative_to
   ld->last_cp = cp;
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
rel_to_ctxpopup_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   rel_to_data *rel_data = data;
   live_data *ld = rel_data->ld;

   Evas_Coord lx, ly, lw, lh;
   evas_object_geometry_get(ld->layout, &lx, &ly, &lw, &lh);

   //Set relative_to properties according to the user input value
   //Case 1: width and height are relative
   if (!ld->fixed_w && !ld->fixed_h)
     {
        if (rel_data->is_rel_to_x)
          {
             if ((rel_data->rel_x == 0.0) || (rel_data->rel_x == 1.0))
               {
                  if ((ld->last_cp == Ctrl_Pt_Rel1) ||
                      (ld->last_cp == Ctrl_Pt_Rel4) ||
                      (ld->last_cp == Ctrl_Pt_Left))
                    {
                       ld->rel_to_info.rel1_to_x = rel_data->rel_x;
                       if (ld->rel_to_info.rel1_x_to)
                         free(ld->rel_to_info.rel1_x_to);
                       ld->rel_to_info.rel1_x_to =
                          strndup(rel_data->part_name,
                                  strlen(rel_data->part_name));
                    }
                  else if ((ld->last_cp == Ctrl_Pt_Rel2) ||
                           (ld->last_cp == Ctrl_Pt_Rel3) ||
                           (ld->last_cp == Ctrl_Pt_Right))
                    {
                       ld->rel_to_info.rel2_to_x = rel_data->rel_x;
                       if (ld->rel_to_info.rel2_x_to)
                         free(ld->rel_to_info.rel2_x_to);
                       ld->rel_to_info.rel2_x_to =
                          strndup(rel_data->part_name,
                                  strlen(rel_data->part_name));
                    }
                }
          }
        if (rel_data->is_rel_to_y)
          {
             if ((rel_data->rel_y == 0.0) || (rel_data->rel_y == 1.0))
               {
                  if ((ld->last_cp == Ctrl_Pt_Rel1) ||
                      (ld->last_cp == Ctrl_Pt_Rel3) ||
                      (ld->last_cp == Ctrl_Pt_Top))
                    {
                       ld->rel_to_info.rel1_to_y = rel_data->rel_y;
                       if (ld->rel_to_info.rel1_y_to)
                         free(ld->rel_to_info.rel1_y_to);
                       ld->rel_to_info.rel1_y_to =
                          strndup(rel_data->part_name,
                                  strlen(rel_data->part_name));
                    }
                  else if ((ld->last_cp == Ctrl_Pt_Rel2) ||
                           (ld->last_cp == Ctrl_Pt_Rel4) ||
                           (ld->last_cp == Ctrl_Pt_Bottom))
                    {
                       ld->rel_to_info.rel2_to_y = rel_data->rel_y;
                       if (ld->rel_to_info.rel2_y_to)
                         free(ld->rel_to_info.rel2_y_to);
                       ld->rel_to_info.rel2_y_to =
                          strndup(rel_data->part_name,
                                  strlen(rel_data->part_name));
                    }
               }
          }
     }
   //Case 2: width is fixed
   else if(ld->fixed_w && !ld->fixed_h)
     {
        if (rel_data->is_rel_to_x)
          {
             if ((ld->last_cp == Ctrl_Pt_Rel1) ||
                 (ld->last_cp == Ctrl_Pt_Rel4) ||
                 (ld->last_cp == Ctrl_Pt_Left))
               {
                  ld->rel_to_info.align_x = 0.0;
                  ld->rel_to_info.align_y = 0.5;
                  ld->rel_to_info.rel1_to_x = rel_data->rel_x;
                  ld->rel_to_info.rel2_to_x = rel_data->rel_x;
                  if (ld->rel_to_info.rel1_x_to)
                    free(ld->rel_to_info.rel1_x_to);
                  if (ld->rel_to_info.rel2_x_to)
                    free(ld->rel_to_info.rel2_x_to);
                  ld->rel_to_info.rel1_x_to =
                     strndup(rel_data->part_name,
                             strlen(rel_data->part_name));
                  ld->rel_to_info.rel2_x_to =
                     strndup(rel_data->part_name,
                             strlen(rel_data->part_name));
               }
             if ((ld->last_cp == Ctrl_Pt_Rel2) ||
                 (ld->last_cp == Ctrl_Pt_Rel2) ||
                 (ld->last_cp == Ctrl_Pt_Right))
               {
                  ld->rel_to_info.align_x = 1.0;
                  ld->rel_to_info.align_y = 0.5;
                  ld->rel_to_info.rel1_to_x = rel_data->rel_x;
                  ld->rel_to_info.rel2_to_x = rel_data->rel_x;
                  if (ld->rel_to_info.rel1_x_to)
                    free(ld->rel_to_info.rel1_x_to);
                  if (ld->rel_to_info.rel2_x_to)
                    free(ld->rel_to_info.rel2_x_to);
                  ld->rel_to_info.rel1_x_to =
                     strndup(rel_data->part_name, strlen(rel_data->part_name));
                  ld->rel_to_info.rel2_x_to =
                     strndup(rel_data->part_name, strlen(rel_data->part_name));
               }
          }
        if (rel_data->is_rel_to_y)
          {
             if ((rel_data->rel_y == 0.0) || (rel_data->rel_y == 1.0))
               {
                  if (ld->last_cp == Ctrl_Pt_Rel1 ||
                      ld->last_cp == Ctrl_Pt_Rel3 ||
                      ld->last_cp == Ctrl_Pt_Top)
                    {
                       ld->rel_to_info.rel1_to_y = rel_data->rel_y;
                       if (ld->rel_to_info.rel1_y_to)
                         free(ld->rel_to_info.rel1_y_to);
                       ld->rel_to_info.rel1_y_to =
                          strndup(rel_data->part_name,
                                  strlen(rel_data->part_name));
                    }
                  else if ((ld->last_cp == Ctrl_Pt_Rel2) ||
                           (ld->last_cp == Ctrl_Pt_Rel4) ||
                           (ld->last_cp == Ctrl_Pt_Bottom))
                    {
                       ld->rel_to_info.rel2_to_y = rel_data->rel_y;
                       if (ld->rel_to_info.rel2_y_to)
                         free(ld->rel_to_info.rel2_y_to);
                       ld->rel_to_info.rel2_y_to =
                          strndup(rel_data->part_name,
                                  strlen(rel_data->part_name));
                    }
               }
          }
     }
   //Case 3: height is fixed
   else if(!ld->fixed_w && ld->fixed_h)
     {
        if (rel_data->is_rel_to_y)
          {
             if ((ld->last_cp == Ctrl_Pt_Rel1) ||
                 (ld->last_cp == Ctrl_Pt_Rel3) ||
                 (ld->last_cp == Ctrl_Pt_Top))
               {
                  ld->rel_to_info.align_x = 0.5;
                  ld->rel_to_info.align_y = 0.0;
                  ld->rel_to_info.rel1_to_y = rel_data->rel_y;
                  ld->rel_to_info.rel2_to_y = rel_data->rel_y;
                  if (ld->rel_to_info.rel1_y_to)
                    free(ld->rel_to_info.rel1_y_to);
                  if (ld->rel_to_info.rel2_y_to)
                    free(ld->rel_to_info.rel2_y_to);
                  ld->rel_to_info.rel1_y_to =
                     strndup(rel_data->part_name, strlen(rel_data->part_name));
                  ld->rel_to_info.rel2_y_to =
                     strndup(rel_data->part_name, strlen(rel_data->part_name));
               }
             if ((ld->last_cp == Ctrl_Pt_Rel2) ||
                 (ld->last_cp == Ctrl_Pt_Rel4) ||
                 (ld->last_cp == Ctrl_Pt_Bottom))
               {
                  ld->rel_to_info.align_x = 0.5;
                  ld->rel_to_info.align_y = 1.0;
                  ld->rel_to_info.rel1_to_y = rel_data->rel_y;
                  ld->rel_to_info.rel2_to_y = rel_data->rel_y;
                  if (ld->rel_to_info.rel1_y_to)
                    free(ld->rel_to_info.rel1_y_to);
                  if (ld->rel_to_info.rel2_y_to)
                    free(ld->rel_to_info.rel2_y_to);
                  ld->rel_to_info.rel1_y_to =
                     strndup(rel_data->part_name, strlen(rel_data->part_name));
                  ld->rel_to_info.rel2_y_to =
                     strndup(rel_data->part_name, strlen(rel_data->part_name));
               }
          }
        if (rel_data->is_rel_to_x)
          {
             if ((rel_data->rel_x == 0.0) || (rel_data->rel_x == 1.0))
               {
                  if ((ld->last_cp == Ctrl_Pt_Rel1) ||
                      (ld->last_cp == Ctrl_Pt_Rel4) ||
                      (ld->last_cp == Ctrl_Pt_Left))
                    {
                       ld->rel_to_info.rel1_to_x = rel_data->rel_x;
                       if (ld->rel_to_info.rel1_x_to)
                         free(ld->rel_to_info.rel1_x_to);
                       ld->rel_to_info.rel1_x_to =
                          strndup(rel_data->part_name,
                                  strlen(rel_data->part_name));
                    }
                  else if ((ld->last_cp == Ctrl_Pt_Rel2) ||
                           (ld->last_cp == Ctrl_Pt_Rel3) ||
                           (ld->last_cp == Ctrl_Pt_Right))
                    {
                       ld->rel_to_info.rel2_to_x = rel_data->rel_x;
                       if (ld->rel_to_info.rel2_x_to)
                         free(ld->rel_to_info.rel2_x_to);
                       ld->rel_to_info.rel2_x_to =
                          strndup(rel_data->part_name,
                                  strlen(rel_data->part_name));
                    }
               }
          }
     }
   //Case 4: width and height are fixed
   else if(ld->fixed_w && ld->fixed_h)
     {
        double x_add, y_add;
        x_add = y_add = 0;

       if (ld->last_cp == Ctrl_Pt_Rel1)
         {
            ld->rel_to_info.align_x = 0.0;
            ld->rel_to_info.align_y = 0.0;
         }
       else if (ld->last_cp == Ctrl_Pt_Rel2)
         {
            x_add = lw;
            y_add = lh;
            ld->rel_to_info.align_x = 1.0;
            ld->rel_to_info.align_y = 1.0;
         }
       else if (ld->last_cp == Ctrl_Pt_Rel3)
         {
            x_add = lw;
            ld->rel_to_info.align_x = 1.0;
            ld->rel_to_info.align_y = 0.0;
         }
       else if (ld->last_cp == Ctrl_Pt_Rel4)
         {
            y_add = lh;
            ld->rel_to_info.align_x = 0.0;
            ld->rel_to_info.align_y = 1.0;
         }
       else if (ld->last_cp == Ctrl_Pt_Left)
         {
            y_add = lh / 2;
            ld->rel_to_info.align_x = 0.0;
            ld->rel_to_info.align_y = 0.5;
         }
       else if (ld->last_cp == Ctrl_Pt_Right)
         {
            x_add = lw;
            y_add = lh / 2;
            ld->rel_to_info.align_x = 1.0;
            ld->rel_to_info.align_y = 0.5;
         }
       else if (ld->last_cp == Ctrl_Pt_Top)
         {
            x_add = lw / 2;
            ld->rel_to_info.align_x = 0.5;
            ld->rel_to_info.align_y = 0.0;
         }
       else if (ld->last_cp == Ctrl_Pt_Bottom)
         {
            x_add = lw / 2;
            y_add = lh;
            ld->rel_to_info.align_x = 0.5;
            ld->rel_to_info.align_y = 1.0;
         }

        if (ld->rel_to_info.rel1_x_to) free(ld->rel_to_info.rel1_x_to);
        if (ld->rel_to_info.rel1_y_to) free(ld->rel_to_info.rel1_y_to);
        if (ld->rel_to_info.rel2_x_to) free(ld->rel_to_info.rel2_x_to);
        if (ld->rel_to_info.rel2_y_to) free(ld->rel_to_info.rel2_y_to);
        ld->rel_to_info.rel1_x_to =
           strndup(rel_data->part_name, strlen(rel_data->part_name));
        ld->rel_to_info.rel1_y_to =
           strndup(rel_data->part_name, strlen(rel_data->part_name));
        ld->rel_to_info.rel2_x_to =
           strndup(rel_data->part_name, strlen(rel_data->part_name));
        ld->rel_to_info.rel2_y_to =
           strndup(rel_data->part_name, strlen(rel_data->part_name));

        //Calculate part relative which is matched to base relative
        double part_rel_x = (double)abs(((lx + x_add) - rel_data->pt1.x)) /
                            (rel_data->pt2.x - rel_data->pt1.x);
        double part_rel_y = (double)abs(((ly + y_add) - rel_data->pt1.y)) /
                            (rel_data->pt2.y - rel_data->pt1.y);

        if (rel_data->is_rel_to_x)
          {
             ld->rel_to_info.rel1_to_x = rel_data->rel_x;
             ld->rel_to_info.rel2_to_x = rel_data->rel_x;
             ld->rel_to_info.rel1_to_y = part_rel_y;
             ld->rel_to_info.rel2_to_y = part_rel_y;
          }

        if (rel_data->is_rel_to_y)
          {
             ld->rel_to_info.rel1_to_y = rel_data->rel_y;
             ld->rel_to_info.rel2_to_y = rel_data->rel_y;
             ld->rel_to_info.rel1_to_x = part_rel_x;
             ld->rel_to_info.rel2_to_x = part_rel_x;
          }
     }

   elm_ctxpopup_dismiss(obj);
}

static void
fixed_ctxpopup_dismissed_cb(void *data, Evas_Object *obj,
                            void *event_info EINA_UNUSED)
{
   live_data *ld = data;
   ld->fixed_ctxpopup = NULL;
   evas_object_del(obj);
}

static void
rel_to_ctxpopup_dismissed_cb(void *data, Evas_Object *obj,
                             void *event_info EINA_UNUSED)
{
   rel_to_data *rel_data = data;
   live_data *ld = rel_data->ld;
   ld->rel_to_ctxpopup = NULL;
   free(rel_data);
   evas_object_del(obj);
}

rel_to_data*
make_rel_data(live_data *ld, auto_align_data *al_pos, int is_rel_to_x, int is_rel_to_y,
              float rel_x, float rel_y, char *rel_dir)
{
   char rel_part_name[PART_NAME_MAX];
   rel_to_data *rel_data = calloc(1, sizeof(rel_to_data));
   int part_name_length = strlen(al_pos->part_name);
   strncpy(rel_data->part_name, al_pos->part_name, part_name_length);
   rel_data->is_rel_to_x = is_rel_to_x;
   rel_data->is_rel_to_y = is_rel_to_y;
   rel_data->rel_x = rel_x;
   rel_data->rel_y = rel_y;
   rel_data->ld = ld;
   snprintf(rel_part_name, PART_NAME_MAX, "%s: %s", al_pos->part_name, rel_dir);
   rel_data->pt1.x = al_pos->pt1.x;
   rel_data->pt1.y = al_pos->pt1.y;
   rel_data->pt2.x = al_pos->pt2.x;
   rel_data->pt2.y = al_pos->pt2.y;
   elm_ctxpopup_item_append(ld->rel_to_ctxpopup, rel_part_name, NULL,
                            rel_to_ctxpopup_cb, rel_data);
   evas_object_smart_callback_add(ld->rel_to_ctxpopup, "dismissed",
                                  rel_to_ctxpopup_dismissed_cb, rel_data);

   return rel_data;

}

static void
show_relative_to_list(live_data *ld, int x, int y)
{
   unsigned int i;
   Eina_Array_Iterator iter;
   auto_align_data *al_pos;

   Evas_Coord_Point cur_ctrl_pt = calc_ctrl_pt_auto_align_pos(ld, x, y);

   if (ld->rel_to_ctxpopup)
     return;

   ld->rel_to_ctxpopup = elm_ctxpopup_add(ld->live_view);
   //FIXME: because the focus highlighting is floated after ctxpopup is
   //dismissed, i disable the focus here
   elm_object_tree_focus_allow_set(ld->rel_to_ctxpopup, EINA_FALSE);

   Eina_Bool is_rel_to = EINA_FALSE;

   //Find relative_to part corresponding to the current control point
   EINA_ARRAY_ITER_NEXT(ld->auto_align_array, i, al_pos, iter)
   {
      //Case 1: Find relative_to x
      if ((cur_ctrl_pt.y >= al_pos->pt1.y) && (cur_ctrl_pt.y <= al_pos->pt2.y))
        {
           if (al_pos->pt1.x == cur_ctrl_pt.x)
             {
                is_rel_to = EINA_TRUE;
                make_rel_data(ld, al_pos, 1, 0, 0.0, 0.0, "to_x");
             }
           if (al_pos->pt2.x == cur_ctrl_pt.x)
             {
                is_rel_to = EINA_TRUE;
                make_rel_data(ld, al_pos, 1, 0, 1.0, 0.0, "to_x");
             }
        }
      //Case 2: Find relative_to y
      if ((cur_ctrl_pt.x >= al_pos->pt1.x) && (cur_ctrl_pt.x <= al_pos->pt2.x))
        {
           if (al_pos->pt1.y == cur_ctrl_pt.y)
             {
                is_rel_to = EINA_TRUE;
                make_rel_data(ld, al_pos, 0, 1, 0.0, 0.0, "to_y");
             }
           if (al_pos->pt2.y == cur_ctrl_pt.y)
             {
                is_rel_to = EINA_TRUE;
                make_rel_data(ld, al_pos, 0, 1, 0.0, 1.0, "to_y");
             }
        }
   }

   if (is_rel_to == EINA_FALSE)
     {
        evas_object_del(ld->rel_to_ctxpopup);
        ld->rel_to_ctxpopup = NULL;
     }
   else
     {
        evas_object_move(ld->rel_to_ctxpopup, x, y);
        evas_object_show(ld->rel_to_ctxpopup);
     }
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

   Evas_Coord x = ev->canvas.x;
   Evas_Coord y = ev->canvas.y;

   show_relative_to_list(ld, x, y);

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
        evas_object_data_set(layout, "index", (void *)(uintptr_t)i);

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
free_auto_align_data(Eina_Array *arr)
{
   unsigned int i;
   Eina_Array_Iterator iter;
   auto_align_data *al_pos;

   if (arr)
     {
        EINA_ARRAY_ITER_NEXT(arr, i, al_pos, iter)
        {
           free(al_pos);
        }
        eina_array_free(arr);
        arr = NULL;
     }
}

static void
live_edit_auto_align_target_parts_init(live_data *ld, Eina_Bool is_update)
{
   Eina_List *l;

   Evas_Object *view_obj = view_obj_get(ld);
   Evas_Coord vx, vy;
   evas_object_geometry_get(view_obj, &vx, &vy, NULL, NULL);

   // set target parts for finding the boundary of exists parts
   char *part_name;
   Eina_List *parts = enventor_object_parts_list_get(base_enventor_get());

   Evas_Coord x,y,w,h;
   //Case 1: create new auto_align_data for new live edit item
   if (!is_update)
     {
        free_auto_align_data(ld->auto_align_array);
        ld->auto_align_array = eina_array_new(eina_list_count(parts));
        EINA_LIST_FOREACH(parts, l, part_name)
        {
           edje_object_part_geometry_get(view_obj, part_name, &x, &y, &w, &h);
           auto_align_data *al_pos = calloc(1, sizeof(auto_align_data));
           snprintf(al_pos->part_name, PART_NAME_MAX, "%s", part_name);
           al_pos->pt1.x = x + vx;
           al_pos->pt1.y = y + vy;
           al_pos->pt2.x = x + w + vx;
           al_pos->pt2.y = y + h + vy;
           eina_array_push(ld->auto_align_array, al_pos);
        }
     }
   //Case 2: update the exsit auto_align_data when view is resized
   else
     {
        int i = 0, item_cnt;
        if (ld->auto_align_array)
          {
             item_cnt = eina_array_count_get(ld->auto_align_array);
             EINA_LIST_FOREACH(parts, l, part_name)
             {
                edje_object_part_geometry_get(view_obj, part_name,
                                              &x, &y, &w, &h);

                if (i < item_cnt)
                  {
                     auto_align_data *al_pos =
                        eina_array_data_get(ld->auto_align_array, i++);
                     al_pos->pt1.x = x + vx;
                     al_pos->pt1.y = y + vy;
                     al_pos->pt2.x = x + w + vx;
                     al_pos->pt2.y = y + h + vy;
                  }
              }
         }
     }
}

static void
layout_update(live_data *ld)
{
   Evas_Coord x, y, w, h;
   Evas_Object *view = view_obj_get(ld);
   evas_object_geometry_get(view, &x, &y, &w, &h);

   double x2 = round(w * ld->rel_info.rel1_x);
   double y2 = round(h * ld->rel_info.rel1_y);
   evas_object_move(ld->layout, (x + x2), (y + y2));
   double w2 =
     round(((double) w * (ld->rel_info.rel2_x - ld->rel_info.rel1_x)));
   double h2 =
     round(((double) h * (ld->rel_info.rel2_y - ld->rel_info.rel1_y)));
   evas_object_resize(ld->layout, w2, h2);

   live_edit_auto_align_target_parts_init(ld, EINA_TRUE);
}

static void
live_edit_update_internal(live_data *ld)
{
   evas_smart_objects_calculate(evas_object_evas_get(ld->layout));
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
calc_layout_auto_align_pos(Evas_Object *layout, live_data *ld, int x, int y,
                           int layout_dir_x, int layout_dir_y, int *ret_x,
                           int *ret_y)
{
   // This function cacluates the position of layout to the closest part edge
   static int pre_layout_dir_x, pre_layout_dir_y;

   Eina_Bool is_up, is_down, is_left, is_right;
   is_up = is_down = is_left = is_right = EINA_FALSE;

   Evas_Coord w, h;
   evas_object_geometry_get(layout, NULL, NULL, &w, &h);

   // layout_dir_x and layout_dir_y are the current direction of layout,
   // pre_layout_dir_x and pre_layout_dir_y are previous direction of layout.
   // These conditions are used to align the layout in moving direction
   if (layout_dir_x == 0)
     {
       if (pre_layout_dir_x < 0)
         {
            is_left = EINA_TRUE;
            pre_layout_dir_x = -1;
         }
       else
         {
            is_right = EINA_TRUE;
            pre_layout_dir_x = 1;
         }
     }
   else
     {
        pre_layout_dir_x = layout_dir_x;
        if (layout_dir_x < 0)
          is_left = EINA_TRUE;
        else if (layout_dir_x > 0)
          is_right = EINA_TRUE;
     }

   if (layout_dir_y == 0)
     {
       if (pre_layout_dir_y < 0)
         {
            is_up = EINA_TRUE;
            pre_layout_dir_y = -1;
         }
     else
       {
          is_down = EINA_TRUE;
          pre_layout_dir_y = 1;
       }
     }
   else
     {
        pre_layout_dir_y = layout_dir_y;
        if (layout_dir_y < 0)
          is_up = EINA_TRUE;
        else if (layout_dir_y > 0)
          is_down = EINA_TRUE;
     }

   int res_x1, res_y1, res_x2, res_y2;
   unsigned int nx, ny, nx2, ny2;
   res_x1 = res_y1 = res_x2 = res_y2 = -1;
   nx = ny = nx2 = ny2 = LIVE_EDIT_MAX_DIST;
   unsigned int dist = ld->auto_align_dist;

   // This loop finds the closest part to the layout
   unsigned int i;
   auto_align_data *al_pos;
   Eina_Array_Iterator iter;
   EINA_ARRAY_ITER_NEXT(ld->auto_align_array, i, al_pos, iter)
   {
      unsigned int dx1, dy1, dx2, dy2;
      dx1 = dy1 = dx2 = dy2 = LIVE_EDIT_MAX_DIST;
      if (((al_pos->pt1.y <= y) && (al_pos->pt2.y >= y)) ||
          ((al_pos->pt1.y <= (y + h)) && (al_pos->pt2.y >= (y + h))) ||
          ((al_pos->pt1.y >= y) && (al_pos->pt2.y <= (y + h))))
        {
           dx1 = abs(al_pos->pt1.x - x);
           dx2 = abs(al_pos->pt1.x - (x + w));
        }

      if (((al_pos->pt1.x <= x) && (al_pos->pt2.x >= x)) ||
          ((al_pos->pt1.x <= (x + w)) && (al_pos->pt2.x >= (x + w))) ||
          ((al_pos->pt1.x >= x) && (al_pos->pt2.x <= (x + w))))
        {
           dy1 = abs(al_pos->pt1.y - y);
           dy2 = abs(al_pos->pt1.y - (y + h));
        }

      if (is_left && (dx1 < dist) && (dx1 < nx))
        {
           nx = dx1;
           res_x1 = al_pos->pt1.x;
        }
      if (is_right && (dx2 < dist) && (dx2 < nx2))
        {
           nx2 = dx2;
           res_x2 = al_pos->pt1.x;
        }
      if (is_up && (dy1 < dist) && (dy1 < ny))
        {
           ny = dy1;
           res_y1 = al_pos->pt1.y;
        }
      if (is_down && (dy2 < dist) && (dy2 < ny2))
        {
           ny2 = dy2;
           res_y2 = al_pos->pt1.y;
        }

      if (((al_pos->pt1.y <= y) && (al_pos->pt2.y >= y)) ||
          ((al_pos->pt1.y <= (y + h)) && (al_pos->pt2.y >= (y + h))) ||
          ((al_pos->pt1.y >= y) && (al_pos->pt2.y <= (y + h))))
        {
           dx1 = abs(al_pos->pt2.x - x);
           dx2 = abs(al_pos->pt2.x - (x + w));
        }

      if (((al_pos->pt1.x <= x) && (al_pos->pt2.x >= x)) ||
          ((al_pos->pt1.x <= (x + w)) && (al_pos->pt2.x >= (x + w))) ||
          ((al_pos->pt1.x >= x) && (al_pos->pt2.x <= (x + w))))
        {
           dy1 = abs(al_pos->pt2.y - y);
           dy2 = abs(al_pos->pt2.y - (y + h));
        }

      if (is_left && (dx1 < dist) && (dx1 < nx))
        {
           nx = dx1;
           res_x1 = al_pos->pt2.x;
        }
      if (is_right && (dx2 < dist) && (dx2 < nx2))
        {
           nx2 = dx2;
           res_x2 = al_pos->pt2.x;
        }
      if (is_up && (dy1 < dist) && (dy1 < ny))
        {
           ny = dy1;
           res_y1 = al_pos->pt2.y;
        }
      if (is_down && (dy2 < dist) && (dy2 < ny2))
        {
           ny2 = dy2;
           res_y2 = al_pos->pt2.y;
        }
   }

   // If we find the closest position and return it
   if (is_left && (res_x1 != -1))
     *ret_x = res_x1;
   else if (is_right && (res_x2 != -1))
     *ret_x = res_x2 - w;

   if (is_up && (res_y1 != -1))
     *ret_y = res_y1;
   else if (is_down && (res_y2 != -1))
     *ret_y = res_y2 - h;
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

   Evas_Coord dir_x, dir_y;
   dir_x =  ev->cur.canvas.x - ev->prev.canvas.x;
   dir_y = ev->cur.canvas.y - ev->prev.canvas.y;

   int ret_x, ret_y;
   ret_x = x;
   ret_y = y;

   // This function set the position of layout to the closest part edge
   calc_layout_auto_align_pos(obj, ld, x, y, dir_x, dir_y, &ret_x, &ret_y);

   x = ret_x;
   y = ret_y;

   //limit to live view boundary
   if (vx > x) x = vx;
   if ((x + w) > (vx + vw)) x = (vx + vw) - w;
   if (vy > y) y = vy;
   if ((y + h) > (vy + vh)) y -= ((y + h) - (vy + vh));

   double orig_rel1_x = ld->rel_info.rel1_x;
   double orig_rel1_y = ld->rel_info.rel1_y;
   ld->rel_info.rel1_x = ROUNDING(((double) (x - vx) / vw), 2);
   ld->rel_info.rel1_y = ROUNDING(((double) (y - vy) / vh), 2);
   ld->rel_info.rel2_x += ROUNDING((ld->rel_info.rel1_x - orig_rel1_x), 2);
   ld->rel_info.rel2_y += ROUNDING((ld->rel_info.rel1_y - orig_rel1_y), 2);

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
key_grab_add(Evas_Object *keygrabber, const char *key)
{
   if (!evas_object_key_grab(keygrabber, key, 0, 0, EINA_TRUE))
     EINA_LOG_ERR(_("Failed to grab key - %s"), key);
}

static void
fixed_w_check_changed_cb(void *data, Evas_Object *obj,
                         void *event_info EINA_UNUSED)
{
   live_data *ld = data;
   ld->fixed_w = elm_check_state_get(obj);
}

static void
fixed_h_check_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   live_data *ld = data;
   ld->fixed_h = elm_check_state_get(obj);
}

static void
show_fixed_check_list(live_data *ld)
{
   if (ld->fixed_ctxpopup)
     return;

   ld->fixed_ctxpopup = elm_ctxpopup_add(ld->live_view);
   //FIXME: because the focus highlighting is floated after ctxpopup is
   //dismissed, i disable the focus here
   elm_object_tree_focus_allow_set(ld->fixed_ctxpopup, EINA_FALSE);
   evas_object_smart_callback_add(ld->fixed_ctxpopup, "dismissed",
                                  fixed_ctxpopup_dismissed_cb, ld);

   Evas_Object *view_obj = view_obj_get(ld);
   Evas_Coord vx, vy, vw, vh;
   evas_object_geometry_get(view_obj, &vx, &vy, &vw, &vh);

   evas_object_move(ld->fixed_ctxpopup, vx + (vw / 2), vy + (vh / 2));
   elm_object_scale_set(ld->fixed_ctxpopup, 1.2);

   Evas_Object *fixed_box = elm_box_add(ld->fixed_ctxpopup);
   elm_box_horizontal_set(fixed_box, EINA_TRUE);
   evas_object_show(fixed_box);

   Evas_Object *label = elm_label_add(fixed_box);
   elm_object_text_set(label, "Set Fixed Properties: ");
   evas_object_show(label);
   elm_box_pack_end(fixed_box, label);

   Evas_Object *fixed_w_check = elm_check_add(fixed_box);
   Evas_Object *fixed_h_check = elm_check_add(fixed_box);

   elm_check_state_set(fixed_w_check, ld->fixed_w);
   elm_check_state_set(fixed_h_check, ld->fixed_h);
   elm_object_text_set(fixed_w_check, "width");
   elm_object_text_set(fixed_h_check, "height");
   elm_box_pack_end(fixed_box, fixed_w_check);
   elm_box_pack_end(fixed_box, fixed_h_check);
   evas_object_show(fixed_w_check);
   evas_object_show(fixed_h_check);

   evas_object_smart_callback_add(fixed_w_check, "changed",
                                  fixed_w_check_changed_cb, ld);
   evas_object_smart_callback_add(fixed_h_check, "changed",
                                  fixed_h_check_changed_cb, ld);

   elm_object_content_set(ld->fixed_ctxpopup, fixed_box);
   evas_object_show(ld->fixed_ctxpopup);
}

static void
live_edit_layer_set(live_data *ld)
{
   //Keygrabber
   ld->keygrabber =
      evas_object_rectangle_add(evas_object_evas_get(ld->live_view));
   evas_object_event_callback_add(ld->keygrabber, EVAS_CALLBACK_KEY_DOWN,
                                  keygrabber_key_down_cb, ld);
   evas_object_event_callback_add(ld->keygrabber, EVAS_CALLBACK_KEY_DOWN,
                                  keygrabber_direction_key_down_cb, ld);
   evas_object_event_callback_add(ld->live_view, EVAS_CALLBACK_RESIZE,
                                  live_view_geom_cb, ld);
   evas_object_event_callback_add(ld->live_view, EVAS_CALLBACK_MOVE,
                                  live_view_geom_cb, ld);
   key_grab_add(ld->keygrabber, "Return");
   key_grab_add(ld->keygrabber, "Delete");
   key_grab_add(ld->keygrabber, "BackSpace");
   key_grab_add(ld->keygrabber, "Up");
   key_grab_add(ld->keygrabber, "Down");
   key_grab_add(ld->keygrabber, "Left");
   key_grab_add(ld->keygrabber, "Right");

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

   evas_object_smart_callback_add(view_scroller_get(ld), "scroll",
                                  view_scroll_cb, ld);

   //Initial Layout Geometry
   ld->rel_info.rel1_x = LIVE_EDIT_REL1;
   ld->rel_info.rel1_y = LIVE_EDIT_REL1;
   ld->rel_info.rel2_x = LIVE_EDIT_REL2;
   ld->rel_info.rel2_y = LIVE_EDIT_REL2;
   ld->rel_to_info.align_x = 0.5;
   ld->rel_to_info.align_y = 0.5;
   ld->rel_to_info.rel1_x_to = NULL;
   ld->rel_to_info.rel1_y_to = NULL;
   ld->rel_to_info.rel2_x_to = NULL;
   ld->rel_to_info.rel2_y_to = NULL;

   live_edit_symbol_set(ld);
   ctrl_pt_init(ld);
   align_line_init(ld);
   live_edit_update_internal(ld);
   info_text_init(ld);
   live_edit_auto_align_target_parts_init(ld, EINA_FALSE);
   show_fixed_check_list(ld);
   ld->last_cp = Ctrl_Pt_Cnt;
}

static void
live_btn_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   live_edit_cancel();
   goto_close();
   search_close();

   live_data *ld = g_ld;

   ld->type = (unsigned int)(uintptr_t)data;
   enventor_object_disabled_set(base_enventor_get(), EINA_TRUE);
   ld->live_view = enventor_object_live_view_get(base_enventor_get());
   ld->on = EINA_TRUE;

   live_edit_layer_set(ld);

   stats_info_msg_update(_("Double click part to confirm. (Esc = cancel)"));
}

static Evas_Object *
live_btn_create(Evas_Object *parent, const char *name, void * data)
{
   Evas_Object *btn = elm_button_add(parent);
   elm_object_style_set(btn, ENVENTOR_NAME);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_focus_allow_set(btn, EINA_FALSE);
   char buf[128];
   snprintf(buf, sizeof(buf), "Add %s", name);
   elm_object_tooltip_text_set(btn, buf);
   elm_object_tooltip_orient_set(btn, ELM_TOOLTIP_ORIENT_BOTTOM);

   Evas_Object *img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, name);
   elm_object_content_set(btn, img);

   evas_object_smart_callback_add(btn, "clicked", live_btn_clicked_cb, data);
   evas_object_show(btn);

   return btn;
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

Eina_Bool
live_edit_get(void)
{
   live_data *ld = g_ld;
   if (!ld) return EINA_FALSE;
   return ld->on;
}

Eina_Bool
live_edit_cancel(void)
{
   live_data *ld = g_ld;
   if (!ld->on) return EINA_FALSE;

   evas_object_del(ld->fixed_ctxpopup);
   evas_object_del(ld->rel_to_ctxpopup);
   ld->fixed_ctxpopup = NULL;
   ld->rel_to_ctxpopup = NULL;

   enventor_object_disabled_set(base_enventor_get(), EINA_FALSE);

   evas_object_del(ld->keygrabber);
   ld->keygrabber = NULL;

   evas_object_event_callback_del(ld->live_view, EVAS_CALLBACK_RESIZE,
                                  live_view_geom_cb);
   evas_object_event_callback_del(ld->live_view, EVAS_CALLBACK_MOVE,
                                  live_view_geom_cb);

   evas_object_smart_callback_del(view_scroller_get(ld),
                                  "scroll",
                                  view_scroll_cb);
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

   return EINA_TRUE;
}

//Create and return a list of buttons.
Eina_List *
live_edit_tools_create(Evas_Object *parent)
{
   Eina_List *btn_list = NULL;
   Evas_Object *btn;
   int i;

   for (i = 0; i < (LIVEEDIT_ITEMS_NUM - 1); i++)
     {
        btn = live_btn_create(parent, LIVEEDIT_ITEMS[i].name,
                              (void *)(uintptr_t) i);
        btn_list = eina_list_append(btn_list, btn);
     }

   //Just for spacer. Because we'd like to avoid margin in the icon image,
   //We use 2 images - button icon's and live edit object's.
   btn = live_btn_create(parent, "Spacer_Icon",
                         (void *)(uintptr_t) i);
   btn_list = eina_list_append(btn_list, btn);

   return btn_list;
}

void
live_edit_init(void)
{
   live_data *ld = calloc(1, sizeof(live_data));
   if (!ld)
     {
        EINA_LOG_ERR(_("Faild to allocate Memory!"));
        return;
     }
   g_ld = ld;
   ld->auto_align_dist = LIVE_EDIT_AUTO_ALIGN_DIST;
}

void
live_edit_term(void)
{
   live_data *ld = g_ld;
   evas_object_del(ld->toolbox);
   live_edit_cancel();

   free_auto_align_data(ld->auto_align_array);
   free(ld);
   g_ld = NULL;
}
