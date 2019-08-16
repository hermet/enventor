#include "common.h"

typedef enum
{
   PANES_LIVE_VIEW_EXPAND,
   PANES_TEXT_EDITOR_EXPAND,
   PANES_EDITORS_EXPAND,
   PANES_CONSOLE_EXPAND,
   PANES_SPLIT_VIEW
} Panes_State;

typedef struct _pane_data
{
   Evas_Object *obj;
   Panes_State state;
   double origin;
   double delta;
   double last_size[2]; //when down the panes bar
   Elm_Transit *transit;
} pane_data;

typedef struct _panes_data
{
   pane_data horiz;  //horizontal pane data (editors, console)
   pane_data vert;    //vertical pane data (live view, text editor)
   Evas_Object *text_tool_layout;
   Evas_Object *live_tool_layout;
} panes_data;

static panes_data *g_pd = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
transit_op_v(void *data, Elm_Transit *transit EINA_UNUSED, double progress)
{
   panes_data *pd = data;
   elm_panes_content_right_size_set(pd->vert.obj,
                                    pd->vert.origin +
                                    (pd->vert.delta * progress));
}

static void
transit_op_h(void *data, Elm_Transit *transit EINA_UNUSED, double progress)
{
   panes_data *pd = data;
   elm_panes_content_right_size_set(pd->horiz.obj,
                                    pd->horiz.origin +
                                    (pd->horiz.delta * progress));
}

static void
v_press_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   panes_data *pd = data;
   pd->vert.last_size[0] = elm_panes_content_right_size_get(obj);
}

static void
v_unpress_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   panes_data *pd = data;
   double size = elm_panes_content_right_size_get(obj);
   if (pd->vert.last_size[0] != size) pd->vert.last_size[1] = size;
   config_editor_size_set(size);
}

static void
h_press_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   panes_data *pd = data;
   pd->horiz.last_size[0] = elm_panes_content_right_size_get(obj);
}

static void
h_unpress_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   panes_data *pd = data;
   double size = elm_panes_content_right_size_get(obj);
   if (pd->horiz.last_size[0] != size) pd->horiz.last_size[1] = size;
   config_console_size_set(size);

   double origin = elm_panes_content_right_size_get(pd->horiz.obj);
   if (origin == 0.0)
     {
        pd->horiz.state = PANES_EDITORS_EXPAND;
        tools_console_update(EINA_FALSE);
     }
   else
     {
        pd->horiz.state = PANES_SPLIT_VIEW;
        tools_console_update(EINA_TRUE);
     }
}

static void
horiz_transit_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   panes_data *pd = data;
   pd->horiz.transit = NULL;
}

static void
vert_transit_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   panes_data *pd = data;
   pd->vert.transit = NULL;
}

static void
panes_h_full_view_cancel(panes_data *pd)
{
   pd->horiz.origin = elm_panes_content_right_size_get(pd->horiz.obj);
   pd->horiz.delta = pd->horiz.last_size[1] - pd->horiz.origin;

   //init console size to default
   if (pd->horiz.delta == 0.0)
     {
        pd->horiz.delta = DEFAULT_CONSOLE_SIZE;
        config_console_size_set(DEFAULT_CONSOLE_SIZE);
     }
   
   elm_transit_del(pd->horiz.transit);

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op_h, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_del_cb_set(transit, horiz_transit_del_cb, pd);
   elm_transit_go(transit);

   pd->horiz.transit = transit;
   pd->horiz.state = PANES_SPLIT_VIEW;
}

static void
panes_v_full_view_cancel(panes_data *pd)
{
   pd->vert.origin = elm_panes_content_right_size_get(pd->vert.obj);
   pd->vert.delta = pd->vert.last_size[1] - pd->vert.origin;

   elm_transit_del(pd->vert.transit);

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op_v, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_del_cb_set(transit, vert_transit_del_cb, pd);
   elm_transit_go(transit);

   pd->vert.transit = transit;
   pd->vert.state = PANES_SPLIT_VIEW;
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
panes_text_editor_full_view(void)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   //Revert state if the current state is full view right already.
   if (pd->vert.state == PANES_TEXT_EDITOR_EXPAND)
     {
        panes_v_full_view_cancel(pd);
        return;
     }

   double origin = elm_panes_content_right_size_get(pd->vert.obj);
   if (origin == 0.0) return;

   pd->vert.origin = origin;
   pd->vert.delta = 0.0 - pd->vert.origin;

   elm_transit_del(pd->vert.transit);

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op_v, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_del_cb_set(transit, vert_transit_del_cb, pd);
   elm_transit_go(transit);

   pd->vert.transit = transit;
   pd->vert.state = PANES_TEXT_EDITOR_EXPAND;
}

void
panes_live_view_full_view(void)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   //Revert state if the current state is full view left already.
   if (pd->vert.state == PANES_LIVE_VIEW_EXPAND)
     {
        panes_v_full_view_cancel(pd);
        return;
     }

   double origin = elm_panes_content_right_size_get(pd->vert.obj);
   if (origin == 1.0) return;

   pd->vert.origin = origin;
   pd->vert.delta = 1.0 - pd->vert.origin;

   elm_transit_del(pd->vert.transit);

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op_v, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_del_cb_set(transit, vert_transit_del_cb, pd);
   elm_transit_go(transit);

   pd->vert.transit = transit;
   pd->vert.state = PANES_LIVE_VIEW_EXPAND;
}

Eina_Bool
panes_editors_full_view_get(void)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd, EINA_FALSE);

   if (pd->horiz.state == PANES_EDITORS_EXPAND) return EINA_TRUE;
   else return EINA_FALSE;
}

void
panes_editors_full_view(Eina_Bool full_view)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   if (full_view)
     {
        if (pd->horiz.state == PANES_EDITORS_EXPAND) return;
        pd->horiz.origin = elm_panes_content_right_size_get(pd->horiz.obj);
        pd->horiz.delta = 0.0 - pd->horiz.origin;

        elm_transit_del(pd->horiz.transit);
        Elm_Transit *transit = elm_transit_add();
        elm_transit_effect_add(transit, transit_op_h, pd, NULL);
        elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
        elm_transit_duration_set(transit, TRANSIT_TIME);
        elm_transit_del_cb_set(transit, horiz_transit_del_cb, pd);
        elm_transit_go(transit);

        pd->horiz.transit = transit;
        pd->horiz.state = PANES_EDITORS_EXPAND;
     }
   else
     {
        //Revert state if the current state is full view bottom already.
        if (pd->horiz.state == PANES_SPLIT_VIEW) return;
        panes_h_full_view_cancel(pd);
     }
}

void
panes_console_full_view(void)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   //Revert state if the current state is full view top already.
   if (pd->horiz.state == PANES_CONSOLE_EXPAND)
     {
        panes_h_full_view_cancel(pd);
        return;
     }

   double origin = elm_panes_content_right_size_get(pd->horiz.obj);
   if (origin == 1.0) return;

   pd->horiz.origin = origin;
   pd->horiz.delta = 1.0 - pd->horiz.origin;

   elm_transit_del(pd->horiz.transit);

   Elm_Transit *transit = elm_transit_add();
   elm_transit_effect_add(transit, transit_op_h, pd, NULL);
   elm_transit_tween_mode_set(transit, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(transit, TRANSIT_TIME);
   elm_transit_del_cb_set(transit, horiz_transit_del_cb, pd);
   elm_transit_go(transit);

   pd->horiz.transit = transit;
   pd->horiz.state = PANES_CONSOLE_EXPAND;
}

void
panes_text_editor_set(Evas_Object *text_editor)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   elm_object_content_set(pd->text_tool_layout, text_editor);
}

void
panes_live_view_set(Evas_Object *live_view)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   elm_object_content_set(pd->live_tool_layout, live_view);
}

void
panes_console_set(Evas_Object *console)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   elm_object_part_content_set(pd->horiz.obj, "bottom", console);
}

void
panes_term(void)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   evas_object_del(pd->horiz.obj);
   elm_transit_del(pd->horiz.transit);
   elm_transit_del(pd->vert.transit);
   free(pd);
}

Evas_Object *
panes_init(Evas_Object *parent)
{
   panes_data *pd = malloc(sizeof(panes_data));
   if (!pd)
     {
        mem_fail_msg();
        return NULL;
     }
   g_pd = pd;

   //Panes Horizontal
   Evas_Object *panes_h = elm_panes_add(parent);
   elm_object_style_set(panes_h, "flush");
   elm_panes_horizontal_set(panes_h, EINA_TRUE);
   evas_object_size_hint_weight_set(panes_h, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(panes_h, "press", h_press_cb, pd);
   evas_object_smart_callback_add(panes_h, "unpress", h_unpress_cb, pd);

   pd->horiz.obj = panes_h;
   pd->horiz.state = PANES_SPLIT_VIEW;
   pd->horiz.last_size[0] = config_console_size_get();
   pd->horiz.last_size[1] = config_console_size_get();
   pd->horiz.transit = NULL;
   
   //Panes Vertical
   Evas_Object *panes_v = elm_panes_add(parent);
   elm_object_style_set(panes_v, ENVENTOR_NAME);
   elm_panes_horizontal_set(panes_v, EINA_FALSE);
   evas_object_size_hint_weight_set(panes_v, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(panes_v, "press", v_press_cb, pd);
   evas_object_smart_callback_add(panes_v, "unpress", v_unpress_cb, pd);

   elm_object_part_content_set(panes_h, "top", panes_v);

   pd->vert.obj = panes_v;
   pd->vert.state = PANES_SPLIT_VIEW;
   pd->vert.last_size[0] = config_editor_size_get();
   pd->vert.last_size[1] = config_editor_size_get();
   pd->vert.transit = NULL;

   elm_panes_content_right_size_set(panes_h, config_console_size_get());
   elm_panes_content_right_size_set(panes_v, config_editor_size_get());

   //Text Tools
   Evas_Object *text_tool_layout = elm_layout_add(pd->vert.obj);
   elm_layout_file_set(text_tool_layout, EDJE_PATH, "tools_layout");
   evas_object_size_hint_weight_set(text_tool_layout, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(text_tool_layout, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   elm_object_part_content_set(pd->vert.obj, "right", text_tool_layout);

   pd->text_tool_layout = text_tool_layout;

   //Live Edit Tools
   Evas_Object *live_tool_layout = elm_layout_add(pd->vert.obj);
   elm_layout_file_set(live_tool_layout, EDJE_PATH, "tools_layout");
   evas_object_size_hint_weight_set(live_tool_layout, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(live_tool_layout, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   elm_object_part_content_set(pd->vert.obj, "left", live_tool_layout);

   pd->live_tool_layout = live_tool_layout;

   return panes_h;
}

void
panes_live_view_tools_set(Evas_Object *tools)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   Evas_Object *live_view = elm_object_part_content_get(pd->vert.obj, "left");
   elm_object_part_content_set(live_view, "elm.swallow.tools", tools);
}

void
panes_live_edit_fixed_bar_set(Evas_Object *fixed_bar)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   Evas_Object *live_view = elm_object_part_content_get(pd->vert.obj, "left");
   elm_object_part_content_set(live_view, "elm.swallow.fixed_bar", fixed_bar);
}

void
panes_text_editor_tools_set(Evas_Object *tools)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   Evas_Object *text_editor = elm_object_part_content_get(pd->vert.obj,
                                                          "right");
   elm_object_part_content_set(text_editor, "elm.swallow.tools", tools);
}

void
panes_live_view_tools_visible_set(Eina_Bool visible)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   Evas_Object *live_view = elm_object_part_content_get(pd->vert.obj, "left");

   if (visible)
     elm_object_signal_emit(live_view, "elm,state,tools,show", "");
   else
     elm_object_signal_emit(live_view, "elm,state,tools,hide", "");
}

void
panes_live_edit_fixed_bar_visible_set(Eina_Bool visible)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   Evas_Object *live_view = elm_object_part_content_get(pd->vert.obj, "left");

   if (visible)
     elm_object_signal_emit(live_view, "elm,state,fixed_bar,show", "");
   else
     elm_object_signal_emit(live_view, "elm,state,fixed_bar,hide", "");
}


void
panes_text_editor_tools_visible_set(Eina_Bool visible)
{
   panes_data *pd = g_pd;
   EINA_SAFETY_ON_NULL_RETURN(pd);

   Evas_Object *text_editor = elm_object_part_content_get(pd->vert.obj,
                                                          "right");

   if (visible)
     elm_object_signal_emit(text_editor, "elm,state,tools,show", "");
   else
     elm_object_signal_emit(text_editor, "elm,state,tools,hide", "");
}
