#include <Elementary.h>
#include "common.h"

//FIXME: Make flexible
const int MAX_LINE_DIGIT_CNT = 10;

struct editor_s
{
   Evas_Object *en_edit;
   Evas_Object *en_line;
   Evas_Object *scroller;
   Evas_Object *layout;
   Evas_Object *ctxpopup;
   Evas_Object *parent;

   color_data *cd;
   stats_data *sd;
   option_data *od;
   parser_data *pd;
   menu_data *md;

   int line_max;
   Eina_Stringshare *group_name;
   Eina_Stringshare *part_name;

   Ecore_Idler *syntax_color_timer;

   void (*part_changed_cb)(void *data, const char *part_name);
   void *part_changed_cb_data;

   Eina_Bool edit_changed : 1;
   Eina_Bool linenumber : 1;
   Eina_Bool ctrl_pressed : 1;
};

static void
last_line_inc(edit_data *ed)
{
   char buf[MAX_LINE_DIGIT_CNT];

   ed->line_max++;
   snprintf(buf, sizeof(buf), "%d<br/>", ed->line_max);
   elm_entry_entry_append(ed->en_line, buf);
}

static void
line_decrease(edit_data *ed, int cnt)
{
   if (cnt < 1) return;

   Evas_Object *textblock = elm_entry_textblock_get(ed->en_line);
   Evas_Textblock_Cursor *cur1 = evas_object_textblock_cursor_new(textblock);
   evas_textblock_cursor_line_set(cur1, (ed->line_max - cnt));
   evas_textblock_cursor_word_start(cur1);

   Evas_Textblock_Cursor *cur2 = evas_object_textblock_cursor_new(textblock);
   evas_textblock_cursor_line_set(cur2, ed->line_max);
   evas_textblock_cursor_paragraph_last(cur2);

   evas_textblock_cursor_range_delete(cur1, cur2);

   evas_textblock_cursor_free(cur1);
   evas_textblock_cursor_free(cur2);

   elm_entry_calc_force(ed->en_line);

   ed->line_max -= cnt;
}

static void
syntax_color_apply(edit_data *ed)
{
   //FIXME: Optimize here by applying color syntax for only changed lines 
   ed->syntax_color_timer = NULL;

   Evas_Object *tb = elm_entry_textblock_get(ed->en_edit);
   char *text = (char *) evas_object_textblock_text_markup_get(tb);

   int pos = elm_entry_cursor_pos_get(ed->en_edit);

   char *utf8 = (char *) color_cancel(ed->cd, text, strlen(text));
   if (!utf8) return;

   utf8 = strdup(utf8);
   const char *translated = color_apply(ed->cd, utf8, strlen(utf8), EINA_TRUE);

   elm_entry_entry_set(ed->en_edit, NULL);
   elm_entry_entry_append(ed->en_edit, translated);
   elm_entry_cursor_pos_set(ed->en_edit, pos);
DFUNC_NAME();
   free(utf8);
}

static Eina_Bool
syntax_color_timer_cb(void *data)
{
   edit_data *ed = data;
   syntax_color_apply(ed);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
syntax_color_animator_cb(void *data)
{
   edit_data *ed = data;
   syntax_color_apply(ed);
   return ECORE_CALLBACK_CANCEL;
}

static int
deleted_line_cnt(const char *str)
{
   int num = 0;

   while(str)
     {
        str = strstr(str, "<br/>");
        if (str) { num++; str++; }
        else break;
     }
   return num;
}

static void
edit_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Entry_Change_Info *info = event_info;
   edit_data *ed = data;
   ed->edit_changed = EINA_TRUE;

   if (info->insert)
     {
        if (!strcmp(info->change.insert.content, "<br/>")) last_line_inc(ed);
     }
   else
     {
        //Check the deleted line
        int num = deleted_line_cnt(info->change.del.content);
        line_decrease(ed, num);
     }

   if (ed->syntax_color_timer) ecore_timer_del(ed->syntax_color_timer);
     ed->syntax_color_timer = ecore_timer_add(0.25, syntax_color_timer_cb, ed);
}

static void
save_msg_show(edit_data *ed)
{
   if (!option_stats_bar_get(ed->od)) return;

   char buf[PATH_MAX];

   if (ed->edit_changed)
     snprintf(buf, sizeof(buf), "File saved. \"%s\"",
              option_edc_path_get(ed->od));
   else
     snprintf(buf, sizeof(buf), "Already saved. \"%s\"",
              option_edc_path_get(ed->od));

   stats_info_msg_update(ed->sd, buf);
}

Eina_Bool
edit_save(edit_data *ed)
{
   if (!ed->edit_changed)
     {
        save_msg_show(ed);
        return EINA_TRUE;
     }

   const char *text = elm_entry_entry_get(ed->en_edit);
   Evas_Object *tb = elm_entry_textblock_get(ed->en_edit);
   const char *text2 = evas_textblock_text_markup_to_utf8(tb, text);

   FILE *fp = fopen(option_edc_path_get(ed->od), "w");
   if (!fp) return EINA_FALSE;

   fputs(text2, fp);
   fclose(fp);

   save_msg_show(ed);
   //FIXME: If compile edc here? we can edit_changed FALSE;
   //ed->edit_changed = EINA_FALSE;

   return EINA_TRUE;
}

static void
ctxpopup_dismiss_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   evas_object_del(obj);
   ed->ctxpopup = NULL;
   elm_object_disabled_set(ed->layout, EINA_FALSE);
   elm_object_focus_set(ed->en_edit, EINA_TRUE);
}

static void
ctxpopup_it_cb(void *data, Evas_Object *obj, void *event_info)
{
   edit_data *ed = data;

   Elm_Object_Item *it = event_info;
   elm_entry_entry_insert(ed->en_edit, elm_object_item_text_get(it));

   elm_ctxpopup_dismiss(obj);
}

static void
slider_dismiss_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   Evas_Object *layout = elm_object_content_get(obj);
   Evas_Object *slider = elm_object_part_content_get(layout,
                                                     "elm.swallow.slider");
   char buf[128];
   attr_value *attr = evas_object_data_get(slider, "attr");
   if (attr->integer)
     {
        snprintf(buf, sizeof(buf), "%d",
                 (int) roundf(elm_slider_value_get(slider)));
     }
   else
     {
        //if the last digit number is 0 then round up.
        double val = elm_slider_value_get(slider);
        snprintf(buf, sizeof(buf), "%0.2f", val);
        double round_down = atof(buf);
        snprintf(buf, sizeof(buf), "%0.1f", val);
        double round_down2 = atof(buf);
        if (fabs(round_down - round_down2) < 0.0005)
          snprintf(buf, sizeof(buf), "%0.1f", val);
        else
          snprintf(buf, sizeof(buf), "%0.2f", val);
     }
   elm_entry_entry_insert(ed->en_edit, buf);
   ed->edit_changed = EINA_TRUE;
   edit_save(ed);
}

static void
btn_plus_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   Evas_Object *slider = data;
   attr_value *attr = evas_object_data_get(slider, "attr");
   double value = elm_slider_value_get(slider);

   if (attr->integer) elm_slider_value_set(slider, value + 1);
   else elm_slider_value_set(slider, value + 0.01);
}

static void
btn_minus_cb(void *data, Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   Evas_Object *slider = data;
   attr_value *attr = evas_object_data_get(slider, "attr");
   double value = elm_slider_value_get(slider);

   if (attr->integer) elm_slider_value_set(slider, value - 1);
   else elm_slider_value_set(slider, value - 0.01);
}

static void
edit_attr_candidate_show(edit_data *ed, attr_value *attr, int x, int y, const char *selected)
{
   //Show up the list of the types
   Evas_Object *ctxpopup = elm_ctxpopup_add(ed->parent);
   elm_object_style_set(ctxpopup, elm_app_name_get());

   //case of strings
   if (attr->strs)
     {
        Eina_List *l;
        Eina_Stringshare *candidate;
        EINA_LIST_FOREACH(attr->strs, l, candidate)
          elm_ctxpopup_item_append(ctxpopup, candidate, NULL, ctxpopup_it_cb,
                                   ed);
     }
   //case of numbers
   else
     {
        //Layout
        Evas_Object *layout = elm_layout_add(ed->en_edit);
        elm_layout_file_set(layout, EDJE_PATH, "slider_layout");
        evas_object_show(layout);

        elm_object_content_set(ctxpopup, layout);

        //Slider
        Evas_Object *slider = elm_slider_add(ed->en_edit);
        elm_object_scale_set(slider, 1.2125);
        if (attr->integer) elm_slider_unit_format_set(slider, "%1.0f");
        else elm_slider_unit_format_set(slider, "%1.2f");
        elm_slider_span_size_set(slider, 120);
        elm_slider_indicator_show_set(slider, EINA_FALSE);
        elm_slider_min_max_set(slider, attr->min, attr->max);
        elm_slider_value_set(slider, atof(selected));
        evas_object_data_set(slider, "attr", attr);
        evas_object_show(slider);

        elm_object_part_content_set(layout, "elm.swallow.slider", slider);

        Evas_Object *btn;
        Evas_Object *img;

        //Minus Button
        btn = elm_button_add(layout);
        evas_object_show(btn);
        evas_object_smart_callback_add(btn, "clicked", btn_minus_cb, slider);

        elm_object_part_content_set(layout, "elm.swallow.minus", btn);

        //Minus Image
        img = elm_image_add(btn);
        elm_image_file_set(img, EDJE_PATH, "minus_img");
        evas_object_show(img);

        elm_object_content_set(btn, img);

        //Plus Button
        btn = elm_button_add(layout);
        evas_object_show(btn);
        evas_object_smart_callback_add(btn, "clicked", btn_plus_cb, slider);

        elm_object_part_content_set(layout, "elm.swallow.plus", btn);

        //Plus Image
        img = elm_image_add(btn);
        elm_image_file_set(img, EDJE_PATH, "plus_img");
        evas_object_show(img);

        elm_object_content_set(btn, img);

        evas_object_smart_callback_add(ctxpopup, "dismissed",
                                       slider_dismiss_cb, ed);
     }

   evas_object_smart_callback_add(ctxpopup, "dismissed", ctxpopup_dismiss_cb,
                                  ed);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);

   menu_ctxpopup_register(ctxpopup);
   elm_object_disabled_set(ed->layout, EINA_TRUE);

   ed->ctxpopup = ctxpopup;
}

static void
edit_mouse_down_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                   Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Event_Mouse_Down *ev = event_info;

   if (ev->button == 1)
     {
        elm_entry_select_none(obj);
        return;
     }
}

static void
cur_line_pos_set(edit_data *ed)
{
   if (!option_stats_bar_get(ed->od)) return;

   Evas_Coord y, h;
   elm_entry_cursor_geometry_get(ed->en_edit, NULL, &y, NULL, &h);
   stats_line_num_update(ed->sd, (y / h) + 1, ed->line_max);
}

static void
edit_cursor_double_clicked_cb(void *data, Evas_Object *obj,
                              void *event_info EINA_UNUSED)
{
   edit_data *ed = data;

   if (ed->ctrl_pressed) return;

   const char *selected = elm_entry_selection_get(obj);
   if (!selected) return;
   selected = parser_markup_escape(ed->pd, selected);

   Evas_Object *textblock = elm_entry_textblock_get(obj);
   Evas_Textblock_Cursor *cursor = evas_object_textblock_cursor_get(textblock);
   const char *str = evas_textblock_cursor_paragraph_text_get(cursor);
   char *text = elm_entry_markup_to_utf8(str);
   char *cur = strstr(text, selected);

   //Get the attribute values
   attr_value * attr = parser_attribute_get(ed->pd, text, cur);
   if (!attr) goto end;

   int x, y;
   evas_pointer_output_xy_get(evas_object_evas_get(obj), &x, &y);
   edit_attr_candidate_show(ed, attr, x, y, selected);

end:
   free(text);
}

static void
part_name_get_cb(void *data, Eina_Stringshare *part_name)
{
   edit_data *ed = data;
   ed->part_name = part_name;
   if (ed->part_changed_cb)
     ed->part_changed_cb(ed->part_changed_cb_data, ed->part_name);
}

void
edit_cur_part_update(edit_data *ed)
{
   if (!option_part_highlight_get(ed->od)) return;

   parser_part_name_get(ed->pd, ed->en_edit, part_name_get_cb, ed);
}

static void
edit_cursor_changed_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   cur_line_pos_set(ed);
   edit_cur_part_update(ed);
}

void
edit_part_changed_cb_set(edit_data *ed, void (*cb)(void *data, const char *part_name), void *data)
{
   ed->part_changed_cb = cb;
   ed->part_changed_cb_data = data;
}

static Eina_Bool
key_down_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   edit_data *ed = data;

   //Control Key
   if (!strcmp("Control_L", event->keyname))
     ed->ctrl_pressed = EINA_TRUE;

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
key_up_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   edit_data *ed = data;

   //Control Key
   if (!strcmp("Control_L", event->keyname))
     ed->ctrl_pressed = EINA_FALSE;

   return ECORE_CALLBACK_PASS_ON;
}

edit_data *
edit_init(Evas_Object *parent, stats_data *sd, option_data *od)
{
   parser_data *pd = parser_init();
   color_data *cd = color_init();

   edit_data *ed = calloc(1, sizeof(edit_data));
   ed->sd = sd;
   ed->pd = pd;
   ed->cd = cd;
   ed->od = od;

   ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, key_down_cb, ed);
   ecore_event_handler_add(ECORE_EVENT_KEY_UP, key_up_cb, ed);

   //Scroller
   Evas_Object *scroller = elm_scroller_add(parent);
   //FIXME: Scroller bars doesn't appeard?
   elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_AUTO,
                           ELM_SCROLLER_POLICY_AUTO);
   elm_object_focus_allow_set(scroller, EINA_FALSE);
   evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(scroller);

   //Layout
   Evas_Object *layout = elm_layout_add(scroller);
   elm_layout_file_set(layout, EDJE_PATH,  "edit_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(layout);

   elm_object_content_set(scroller, layout);

   //Line Number Entry
   Evas_Object *en_line = elm_entry_add(layout);
   elm_object_style_set(en_line, "linenumber");
   elm_entry_editable_set(en_line, EINA_FALSE);
   elm_entry_line_wrap_set(en_line, EINA_FALSE);
   evas_object_size_hint_weight_set(en_line, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en_line, 0, EVAS_HINT_FILL);
   evas_object_show(en_line);

   elm_object_part_content_set(layout, "elm.swallow.linenumber", en_line);

   //EDC Editor Entry
   Evas_Object *en_edit = elm_entry_add(layout);
   elm_object_style_set(en_edit, elm_app_name_get());
   elm_entry_context_menu_disabled_set(en_edit, EINA_TRUE);
   elm_entry_line_wrap_set(en_edit, EINA_FALSE);
   evas_object_smart_callback_add(en_edit, "changed,user", edit_changed_cb, ed);
   evas_object_smart_callback_add(en_edit, "cursor,changed",
                                  edit_cursor_changed_cb, ed);
   evas_object_smart_callback_add(en_edit, "clicked,double",
                                  edit_cursor_double_clicked_cb, ed);
   evas_object_size_hint_weight_set(en_edit, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en_edit, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_event_callback_add(en_edit, EVAS_CALLBACK_MOUSE_DOWN,
                                  edit_mouse_down_cb, ed);
   evas_object_show(en_edit);

   elm_object_focus_set(en_edit, EINA_TRUE);

   elm_object_part_content_set(layout, "elm.swallow.edit", en_edit);

   ed->scroller = scroller;
   ed->en_line = en_line;
   ed->en_edit = en_edit;
   ed->layout = layout;
   ed->parent = parent;
   ed->linenumber = EINA_TRUE;

   edit_line_number_toggle(ed);

   return ed;
}

void
edit_editable_set(edit_data *ed, Eina_Bool editable)
{
   elm_entry_editable_set(ed->en_edit, editable);
}

Evas_Object *
edit_obj_get(edit_data *ed)
{
   return ed->scroller;
}

void
edit_term(edit_data *ed)
{
   if (!ed) return;

   color_data *cd = ed->cd;
   parser_data *pd = ed->pd;

   if (ed->group_name) eina_stringshare_del(ed->group_name);
   if (ed->syntax_color_timer) ecore_timer_del(ed->syntax_color_timer);
   free(ed);

   color_term(cd);
   parser_term(pd);
}

void
edit_edc_read(edit_data *ed, const char *file_path)
{
   char buf[MAX_LINE_DIGIT_CNT];

   Eina_File *file = eina_file_open(file_path, EINA_FALSE);
   if (!file) goto err;

   Eina_Iterator *itr = eina_file_map_lines(file);
   if (!itr) goto err;

   Eina_Strbuf *strbuf = eina_strbuf_new();
   if (!strbuf) goto err;

   Eina_File_Line *line;
   int line_num = 0;

   EINA_ITERATOR_FOREACH(itr, line)
     {
        //Append edc code
        if (line_num > 0)
          {
             if (!eina_strbuf_append(strbuf, "<br/>")) goto err;
          }

        if (!eina_strbuf_append_length(strbuf, line->start, line->length))
          goto err;
        line_num++;

        //Append line number
        sprintf(buf, "%d<br/>", line_num);
        elm_entry_entry_append(ed->en_line, buf);
     }

   elm_entry_entry_append(ed->en_edit, eina_strbuf_string_get(strbuf));

   ed->line_max = line_num;
   if (ed->group_name) eina_stringshare_del(ed->group_name);
   ed->group_name = parser_group_name_get(ed->pd, ed->en_edit);

   stats_edc_file_set(ed->sd, ed->group_name);
   ecore_animator_add(syntax_color_animator_cb, ed);

err:
   if (strbuf) eina_strbuf_free(strbuf);
   if (itr) eina_iterator_free(itr);
   if (file) eina_file_close(file);
}

void
edit_focus_set(edit_data *ed)
{
   elm_object_focus_set(ed->en_edit, EINA_TRUE);
}

Eina_Bool
edit_changed_get(edit_data *ed)
{
   return ed->edit_changed;
}

void
edit_changed_set(edit_data *ed, Eina_Bool changed)
{
   ed->edit_changed = changed;
}

void
edit_line_number_toggle(edit_data *ed)
{
   Eina_Bool linenumber = option_linenumber_get(ed->od);
   if (ed->linenumber == linenumber) return;
   ed->linenumber = linenumber;

   if (linenumber)
     elm_object_signal_emit(ed->layout, "elm,state,linenumber,show", "");
   else
     elm_object_signal_emit(ed->layout, "elm,state,linenumber,hide", "");
}

void
edit_new(edit_data *ed)
{
   elm_entry_entry_set(ed->en_edit, "");
   elm_entry_entry_set(ed->en_line, "");
   edit_edc_read(ed, option_edc_path_get(ed->od));
   ed->edit_changed = EINA_TRUE;
}

const char *
edit_group_name_get(edit_data *ed)
{
   return ed->group_name;
}
