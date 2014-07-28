#include <Elementary.h>
#include "auto_comp_code.h"
#include "common.h"

#define QUEUE_SIZE 20
#define COMPSET_PAIR_MINIMUM 1

typedef struct comp_set_s
{
   Eina_Stringshare *key;
   char **txt;
   int cursor_offset;
   int line_back;
   int line_cnt;
} comp_set;

typedef struct autocomp_s
{
   char queue[QUEUE_SIZE];
   int queue_pos;
   comp_set compset[COMPSET_CNT];
   edit_data *ed;
   Evas_Object *anchor;
   Evas_Object *list;
   Eina_List *compset_list;
   Ecore_Thread *init_thread;
   Eina_Bool anchor_visible : 1;
   Eina_Bool initialized : 1;
} autocomp_data;

static autocomp_data *g_ad = NULL;

#define COMPDATA_SET(ad, key, txt, cursor_offset, line_back) \
   compdata_set(ad, idx++, key, (char **)(&txt), cursor_offset, line_back, txt##_LINE_CNT)

static void
compdata_set(autocomp_data *ad, int idx, char *key, char **txt, int cursor_offset, int line_back, int line_cnt)
{
   ad->compset[idx].key = eina_stringshare_add(key);
   ad->compset[idx].txt = txt;
   ad->compset[idx].cursor_offset = cursor_offset;
   ad->compset[idx].line_back = line_back;
   ad->compset[idx].line_cnt = line_cnt;
}

static void
init_thread_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   autocomp_data *ad = data;
   int idx = 0;

   COMPDATA_SET(ad, "collections",AUTOCOMP_COLLECTIONS, 2, 1);
   COMPDATA_SET(ad, "image",AUTOCOMP_IMAGE, 7, 0);
   COMPDATA_SET(ad, "images",AUTOCOMP_IMAGES, 2, 1);
   COMPDATA_SET(ad, "group",AUTOCOMP_GROUP, 4, 1);
   COMPDATA_SET(ad, "type",AUTOCOMP_TYPE, 0, 0);
   COMPDATA_SET(ad, "part",AUTOCOMP_PART, 4, 1);
   COMPDATA_SET(ad, "parts",AUTOCOMP_PARTS, 2, 1);
   COMPDATA_SET(ad, "description",AUTOCOMP_DESCRIPTION, 8, 1);
   COMPDATA_SET(ad, "inherit",AUTOCOMP_INHERIT, 6, 0);
   COMPDATA_SET(ad, "program",AUTOCOMP_PROGRAM, 4, 1);
   COMPDATA_SET(ad, "programs",AUTOCOMP_PROGRAMS, 2, 1);
   COMPDATA_SET(ad, "signal",AUTOCOMP_SIGNAL, 2, 0);
   COMPDATA_SET(ad, "source",AUTOCOMP_SOURCE, 2, 0);
   COMPDATA_SET(ad, "target",AUTOCOMP_TARGET, 2, 0);
   COMPDATA_SET(ad, "scale",AUTOCOMP_SCALE, 1, 0);
   COMPDATA_SET(ad, "rel1",AUTOCOMP_REL1, 2, 1);
   COMPDATA_SET(ad, "rel2",AUTOCOMP_REL2, 2, 1);
   COMPDATA_SET(ad, "relatvie",AUTOCOMP_RELATIVE, 1, 0);
   COMPDATA_SET(ad, "offset", AUTOCOMP_OFFSET, 1, 0);
   COMPDATA_SET(ad, "color", AUTOCOMP_COLOR, 1, 0);
   COMPDATA_SET(ad, "color2", AUTOCOMP_COLOR2, 1, 0);
   COMPDATA_SET(ad, "color3", AUTOCOMP_COLOR3, 1, 0);
   COMPDATA_SET(ad, "aspect", AUTOCOMP_ASPECT, 1, 0);
   COMPDATA_SET(ad, "aspect_preference", AUTOCOMP_ASPECT_PREFERENCE, 1, 0);
   COMPDATA_SET(ad, "normal", AUTOCOMP_NORMAL, 2, 0);
   COMPDATA_SET(ad, "effect", AUTOCOMP_EFFECT, 0, 0);
   COMPDATA_SET(ad, "text", AUTOCOMP_TEXT, 2, 1);
   COMPDATA_SET(ad, "font", AUTOCOMP_FONT, 2, 0);
   COMPDATA_SET(ad, "align", AUTOCOMP_ALIGN, 1, 0);
   COMPDATA_SET(ad, "size", AUTOCOMP_SIZE, 1, 0);
   COMPDATA_SET(ad, "action", AUTOCOMP_ACTION, 6, 0);
   COMPDATA_SET(ad, "transition", AUTOCOMP_TRANSITION, 1, 0);
   COMPDATA_SET(ad, "after", AUTOCOMP_AFTER, 2, 0);
   COMPDATA_SET(ad, "styles", AUTOCOMP_STYLES, 2, 1);
   COMPDATA_SET(ad, "style", AUTOCOMP_STYLE, 4, 1);
   COMPDATA_SET(ad, "base", AUTOCOMP_BASE, 2, 0);
   COMPDATA_SET(ad, "sounds", AUTOCOMP_SOUNDS, 2, 1);
   COMPDATA_SET(ad, "sample", AUTOCOMP_SAMPLE, 13, 1);
   COMPDATA_SET(ad, "map", AUTOCOMP_MAP, 2, 1);
   COMPDATA_SET(ad, "on", AUTOCOMP_ON, 1, 0);
   COMPDATA_SET(ad, "visible", AUTOCOMP_VISIBLE, 1, 0);
   COMPDATA_SET(ad, "perspective_on", AUTOCOMP_PERSPECTIVE_ON, 1, 0);
   COMPDATA_SET(ad, "perspective", AUTOCOMP_PERSPECTIVE, 2, 0);
   COMPDATA_SET(ad, "backface_cull", AUTOCOMP_BACKFACE_CULL, 1, 0);
   COMPDATA_SET(ad, "rotation", AUTOCOMP_ROTATION, 2, 1);
   COMPDATA_SET(ad, "min", AUTOCOMP_MIN, 1, 0);
   COMPDATA_SET(ad, "max", AUTOCOMP_MAX, 1, 0);
   COMPDATA_SET(ad, "fixed", AUTOCOMP_FIXED, 1, 0);
   COMPDATA_SET(ad, "clip_to", AUTOCOMP_CLIP_TO, 2, 0);
   COMPDATA_SET(ad, "tween", AUTOCOMP_TWEEN, 2, 0);
}

static void
init_thread_end_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   autocomp_data *ad = data;
   ad->initialized = EINA_TRUE;
   ad->init_thread = NULL;
}

static void
init_thread_cancel_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   autocomp_data *ad = data;
   ad->init_thread = NULL;
}

static void
entry_anchor_off(autocomp_data *ad)
{
   if (ad->anchor_visible) elm_object_tooltip_hide(ad->anchor);
   ad->anchor_visible = EINA_FALSE;
   ad->compset_list = eina_list_free(ad->compset_list);
}

static void
anchor_unfocused_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;
   entry_anchor_off(ad);
}

static void
queue_reset(autocomp_data *ad)
{
   if (ad->queue_pos == -1) return;
   ad->queue_pos = -1;
   memset(ad->queue, 0x0, sizeof(ad->queue));
   entry_anchor_off(ad);
}

static void
compset_list_update(autocomp_data *ad)
{
   if (!config_auto_complete_get() || !ad->initialized) return;
   if (ad->queue_pos < COMPSET_PAIR_MINIMUM) return;
   int i;

   ad->compset_list = eina_list_free(ad->compset_list);
   for (i = 0; i < COMPSET_CNT; i++)
     {
        if (ad->queue[0] == ad->compset[i].key[0])
          {
             if (!strncmp(ad->queue, ad->compset[i].key, ad->queue_pos + 1))
               {
                  ad->compset_list = eina_list_append(ad->compset_list,
                                                      &ad->compset[i]);
               }
          }
     }
}

static void
push_char(autocomp_data *ad, char c)
{
   ad->queue_pos++;
   if (ad->queue_pos == QUEUE_SIZE)
     {
        memset(ad->queue, 0x0, sizeof(ad->queue));
        ad->queue_pos = 0;
     }
   ad->queue[ad->queue_pos] = c;

   compset_list_update(ad);
}

static void
pop_char(autocomp_data *ad, int cnt)
{
   if (ad->queue_pos == -1) return;

   int i;
   for (i = 0; i < cnt; i++)
     {
        ad->queue[ad->queue_pos] = 0x0;
        ad->queue_pos--;
        if (ad->queue_pos < 0) break;
     }

   if (ad->queue_pos == -1) return;

   compset_list_update(ad);
}

static void
insert_completed_text(autocomp_data *ad)
{
   if (!ad->compset_list) return;

   Elm_Object_Item *it = elm_list_selected_item_get(ad->list);

   comp_set *compset =  elm_object_item_data_get(it);
   char **txt = compset->txt;
   Evas_Object *entry = edit_entry_get(ad->ed);

   int space = edit_cur_indent_depth_get(ad->ed);

   //Insert the first line.
   elm_entry_entry_insert(entry,  txt[0]+ (ad->queue_pos + 1));

   //Insert last lines
   if (compset->line_cnt > 1)
     {
        //Alloc Empty spaces
        char *p = alloca(space + 1);
        memset(p, ' ', space);
        p[space] = '\0';

        int i;
        for (i = 1; i < (compset->line_cnt -1); i++)
          {
             elm_entry_entry_insert(entry, p);
             elm_entry_entry_insert(entry, txt[i]);
          }
        elm_entry_entry_insert(entry, p);
        elm_entry_entry_insert(entry, txt[i]);
     }

   int cursor_pos = elm_entry_cursor_pos_get(entry);
   cursor_pos -= (compset->cursor_offset + (compset->line_back * space));
   elm_entry_cursor_pos_set(entry, cursor_pos);
   edit_line_increase(ad->ed, (compset->line_cnt - 1));
}

static void
list_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;
   ad->list = NULL;
}

static Evas_Object *
entry_tooltip_content_cb(void *data, Evas_Object *obj EINA_UNUSED,
                         Evas_Object *tt EINA_UNUSED)
{
   autocomp_data *ad = data;

   ad->list = elm_list_add(obj);
   elm_object_focus_allow_set(ad->list, EINA_FALSE);
   elm_list_mode_set(ad->list, ELM_LIST_EXPAND);

   Eina_List *l;
   comp_set *compset;
   EINA_LIST_FOREACH(ad->compset_list, l, compset)
     elm_list_item_append(ad->list, compset->key, NULL, NULL, NULL, compset);
   Elm_Object_Item *it = elm_list_first_item_get(ad->list);
   elm_list_item_selected_set(it, EINA_TRUE);
   evas_object_smart_callback_add(ad->list, "unfocused", anchor_unfocused_cb,
                                  ad);
   evas_object_event_callback_add(ad->list, EVAS_CALLBACK_DEL, list_del_cb, ad);
   elm_list_go(ad->list);
   evas_object_show(ad->list);

   return ad->list;
}

static void
anchor_list_update(autocomp_data *ad)
{
   comp_set *compset;
   Eina_List *items = (Eina_List *) elm_list_items_get(ad->list);
   Eina_List *l, *ll, *l2;
   Elm_Object_Item *it;
   Eina_Bool found;

   const char *it_name;

   //Remove the non-candidates 
   EINA_LIST_FOREACH_SAFE(items, l, ll, it)
     {
        found = EINA_FALSE;
        it_name = elm_object_item_text_get(it);

        EINA_LIST_FOREACH(ad->compset_list, l2, compset)
          {
             if (compset->key == it_name)
               {
                  found = EINA_TRUE;
                  break;
               }
          }

        if (!found) elm_object_item_del(it);
     }

   items = (Eina_List *) elm_list_items_get(ad->list);

   //Append new candidates
   EINA_LIST_FOREACH(ad->compset_list, l, compset)
     {
        found = EINA_FALSE;
        EINA_LIST_FOREACH(items, l2, it)
          {
             it_name = elm_object_item_text_get(it);
             if (it_name != compset->key) continue;
             found = EINA_TRUE;
             break;
          }
        if (!found) elm_list_item_append(ad->list, compset->key, NULL,
                                         NULL, NULL, compset);
     }
   it = elm_list_first_item_get(ad->list);
   elm_list_item_selected_set(it, EINA_TRUE);
   elm_list_go(ad->list);
}

static void
candidate_list_show(autocomp_data *ad)
{
   if (!ad->compset_list)
     {
        entry_anchor_off(ad);
        return;
     }

   Evas_Object *entry = edit_entry_get(ad->ed);

   //Update anchor position
   Evas_Coord x, y, h;
   Evas_Coord cx, cy, cw, ch;
   evas_object_geometry_get(entry, &x, &y, NULL, NULL);
   elm_entry_cursor_geometry_get(entry, &cx, &cy, &cw, &ch);
   evas_object_move(ad->anchor, cx + x, cy + y);
   evas_object_resize(ad->anchor, cw, ch);

   //Show the tooltip
   if (!ad->anchor_visible)
     {
        //Decide the Tooltip direction
        Elm_Tooltip_Orient tooltip_orient = ELM_TOOLTIP_ORIENT_BOTTOM;
        Evas_Object *layout = base_layout_get();
        evas_object_geometry_get(layout, NULL, NULL, NULL, &h);
        if ((cy + y) > (h / 2)) tooltip_orient = ELM_TOOLTIP_ORIENT_TOP;

        //Tooltip set
        elm_object_tooltip_content_cb_set(ad->anchor,
                                          entry_tooltip_content_cb, ad, NULL);
        elm_object_tooltip_orient_set(ad->anchor, tooltip_orient);
        elm_object_tooltip_show(ad->anchor);
        ad->anchor_visible = EINA_TRUE;
     }
   //Already tooltip is visible, just update the list item
   else anchor_list_update(ad);
}

static void
entry_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Entry_Change_Info *info = event_info;
   autocomp_data *ad = data;

   if (info->insert)
     {
        if ((strlen(info->change.insert.content) > 1) ||
            (info->change.insert.content[0] == ' ') ||
            (info->change.insert.content[0] == '.'))
          {
             entry_anchor_off(ad);
             queue_reset(ad);
          }
        else
          {
             push_char(ad, info->change.insert.content[0]);
             candidate_list_show(ad);
          }
     }
   else
     {
        if (info->change.del.content[0] != ' ')
          {
             entry_anchor_off(ad);
             int cnt = abs(info->change.del.end - info->change.del.start);
             pop_char(ad, cnt);
          }
     }
}

static void
entry_cursor_changed_manual_cb(void *data, Evas_Object *obj,
                               void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;
   entry_anchor_off(ad);
}

static void
entry_cursor_changed_cb(void *data, Evas_Object *obj,
                        void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;

   //Update anchor position
   Evas_Coord x, y, cx, cy, cw, ch;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   elm_entry_cursor_geometry_get(obj, &cx, &cy, &cw, &ch);
   evas_object_move(ad->anchor, cx + x, cy + y);
   evas_object_resize(ad->anchor, cw, ch);
}

static void
entry_press_cb(void *data, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;
   entry_anchor_off(ad);
}

static void
entry_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
              void *event_info EINA_UNUSED)
{
   autocomp_data *ad = data;
   entry_anchor_off(ad);
}

static void
list_item_move(autocomp_data *ad, Eina_Bool up)
{
   Evas_Object *entry = edit_entry_get(ad->ed);
   evas_object_smart_callback_del(entry, "unfocused", anchor_unfocused_cb);

   Elm_Object_Item *it = elm_list_selected_item_get(ad->list);
   if (up) it = elm_list_item_prev(it);
   else it = elm_list_item_next(it);
   if (it) elm_list_item_selected_set(it, EINA_TRUE);

   evas_object_smart_callback_add(entry, "unfocused", anchor_unfocused_cb, ad);
}

void
autocomp_target_set(edit_data *ed)
{
   autocomp_data *ad = g_ad;
   Evas_Object *entry;

   queue_reset(ad);

   //Unset previous target
   if (ad->ed)
     {
        entry = edit_entry_get(ad->ed);
        evas_object_smart_callback_del(entry, "changed,user", entry_changed_cb);
        evas_object_smart_callback_del(entry, "cursor,changed",
                                       entry_cursor_changed_cb);
        evas_object_smart_callback_del(entry, "cursor,changed,manual",
                                               entry_cursor_changed_manual_cb);
        evas_object_smart_callback_del(entry, "unfocused", anchor_unfocused_cb);
        evas_object_smart_callback_del(entry, "press", entry_press_cb);
        evas_object_event_callback_del(entry, EVAS_CALLBACK_MOVE,
                                       entry_move_cb);
        ad->ed = NULL;
     }

   if (!ed) return;

   entry = edit_entry_get(ed);
   evas_object_smart_callback_add(entry, "changed,user", entry_changed_cb, ad);
   evas_object_smart_callback_add(entry, "cursor,changed,manual",
                                          entry_cursor_changed_manual_cb, ad);
   evas_object_smart_callback_add(entry, "cursor,changed",
                                  entry_cursor_changed_cb, ad);
   evas_object_smart_callback_add(entry, "unfocused", anchor_unfocused_cb, ad);
   evas_object_smart_callback_add(entry, "press", entry_press_cb, ad);
   evas_object_event_callback_add(entry, EVAS_CALLBACK_MOVE,
                                  entry_move_cb, ad);
   ad->ed = ed;
}


void
autocomp_init(Evas_Object *parent)
{
   autocomp_data *ad = calloc(1, sizeof(autocomp_data));
   if (!ad)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }
   ad->init_thread = ecore_thread_run(init_thread_cb, init_thread_end_cb,
                                      init_thread_cancel_cb, ad);
   ad->anchor = elm_button_add(parent);
   ad->queue_pos = -1;
   g_ad = ad;
}

void
autocomp_term()
{
   autocomp_data *ad = g_ad;
   evas_object_del(ad->anchor);
   ecore_thread_cancel(ad->init_thread);

   int i;
   for (i = 0; i < COMPSET_CNT; i++)
     eina_stringshare_del(ad->compset[i].key);

   free(ad);
   g_ad = NULL;
}

void
autocomp_toggle()
{
   Eina_Bool toggle = !config_auto_complete_get();
   if (toggle) stats_info_msg_update("Auto Completion Enabled.");
   else stats_info_msg_update("Auto Completion Disabled.");
   config_auto_complete_set(toggle);
}

Eina_Bool
autocomp_key_event_hook(const char *key)
{
   autocomp_data *ad = g_ad;
   if (!ad || !ad->anchor_visible) return EINA_FALSE;

   //Cancel the auto complete.
   if (!strcmp(key, "BackSpace"))
     {
        queue_reset(ad);
        entry_anchor_off(ad);
        return EINA_TRUE;
     }

    if (!strcmp(key, "Return"))
      {
         insert_completed_text(ad);
         queue_reset(ad);
         edit_syntax_color_partial_apply(ad->ed);
         return EINA_TRUE;
      }

    if (!strcmp(key, "Up"))
      {
         list_item_move(ad, EINA_TRUE);
         return EINA_TRUE;
      }

    if (!strcmp(key, "Down"))
      {
         list_item_move(ad, EINA_FALSE);
         return EINA_TRUE;
      }

   return EINA_FALSE;
}
