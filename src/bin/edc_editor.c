#include <Elementary.h>
#include "common.h"
#include "template_code.h"

//FIXME: Make flexible
const int MAX_LINE_DIGIT_CNT = 10;
const double SYNTAX_COLOR_TIME = 0.25;

struct editor_s
{
   Evas_Object *en_edit;
   Evas_Object *en_line;
   Evas_Object *scroller;
   Evas_Object *layout;
   Evas_Object *parent;

   syntax_helper *sh;
   stats_data *sd;
   config_data *cd;
   parser_data *pd;
   view_data *vd;

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

void
edit_vd_set(edit_data *ed, view_data *vd)
{
   ed->vd = vd;
}

static int
indent_space_get(edit_data *ed)
{
   const int TAB_SPACE = 3;

   //Get the indentation depth
   int pos = elm_entry_cursor_pos_get(ed->en_edit);
   char *src = elm_entry_markup_to_utf8(elm_entry_entry_get(ed->en_edit));
   int space = indent_depth_get(syntax_indent_data_get(ed->sh), src, pos);
   space *= TAB_SPACE;
   free(src);

   return space;
}

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

DFUNC_NAME();

}

static void
syntax_color_apply(edit_data *ed)
{
   //FIXME: Optimize here by applying color syntax for only changed lines 
   ed->syntax_color_timer = NULL;

   Evas_Object *tb = elm_entry_textblock_get(ed->en_edit);
   char *text = (char *) evas_object_textblock_text_markup_get(tb);

   int pos = elm_entry_cursor_pos_get(ed->en_edit);

   char *utf8 = (char *) color_cancel(syntax_color_data_get(ed->sh), text,
                                      strlen(text));
   if (!utf8) return;

   utf8 = strdup(utf8);
   const char *translated = color_apply(syntax_color_data_get(ed->sh), utf8,
                                        strlen(utf8), EINA_TRUE);
   elm_entry_entry_set(ed->en_edit, NULL);
   elm_entry_entry_append(ed->en_edit, translated);
//FIXME: don't know why this api reset the entry cursor.
//   elm_entry_entry_set(ed->en_edit, translated);
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

static void
indent_apply(edit_data *ed)
{
   int space = indent_space_get(ed);

   //Alloc Empty spaces
   char *p = alloca(space) + 1;
   memset(p, ' ', space);
   p[space] = '\0';

   elm_entry_entry_insert(ed->en_edit, p);
}

static void
edit_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Entry_Change_Info *info = event_info;
   edit_data *ed = data;
   ed->edit_changed = EINA_TRUE;

   Eina_Bool syntax_color = EINA_TRUE;

   if (info->insert)
     {
        if (!strcmp(info->change.insert.content, "<br/>"))
          {
             last_line_inc(ed);
             if (config_auto_indent_get(ed->cd)) indent_apply(ed);
             syntax_color = EINA_FALSE;
          }
        else if (info->change.insert.content[0] == '}')
          {
             //TODO: auto indent.
          }
     }
   else
     {
        //Check the deleted line
        if (!strcmp(info->change.del.content, "<br/>"))
          {
             line_decrease(ed, 1);
             syntax_color = EINA_FALSE;
          }
     }

   if (!syntax_color) return;

   if (ed->syntax_color_timer) ecore_timer_del(ed->syntax_color_timer);
     ed->syntax_color_timer = ecore_timer_add(SYNTAX_COLOR_TIME,
                                              syntax_color_timer_cb, ed);
}

static void
save_msg_show(edit_data *ed)
{
   if (!config_stats_bar_get(ed->cd)) return;

   char buf[PATH_MAX];

   if (ed->edit_changed)
     snprintf(buf, sizeof(buf), "File saved. \"%s\"",
              config_edc_path_get(ed->cd));
   else
     snprintf(buf, sizeof(buf), "Already saved. \"%s\"",
              config_edc_path_get(ed->cd));

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

   FILE *fp = fopen(config_edc_path_get(ed->cd), "w");
   if (!fp) return EINA_FALSE;

   fputs(text2, fp);
   fclose(fp);

   save_msg_show(ed);
   //FIXME: If compile edc here? we can edit_changed FALSE;
   //ed->edit_changed = EINA_FALSE;

   return EINA_TRUE;
}

static void
ctxpopup_candidate_dismiss_cb(void *data, Evas_Object *obj, void *event_info)
{
   edit_data *ed = data;
   evas_object_del(obj);
   elm_object_disabled_set(ed->layout, EINA_FALSE);
   elm_object_focus_set(ed->en_edit, EINA_TRUE);
}

static void
ctxpopup_candidate_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   edit_data *ed = data;
   const char *text = event_info;
   elm_entry_entry_insert(ed->en_edit, text);
   elm_ctxpopup_dismiss(obj);
   ed->edit_changed = EINA_TRUE;
   edit_save(ed);
}

static void
ctxpopup_preview_dismiss_cb(void *data, Evas_Object *obj, void *event_info)
{
   edit_data *ed = data;
   evas_object_del(obj);
   elm_object_disabled_set(ed->layout, EINA_FALSE);
   elm_object_focus_set(ed->en_edit, EINA_TRUE);
}

static void
ctxpopup_preview_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   edit_data *ed = data;
   elm_ctxpopup_dismiss(obj);
}

void
edit_template_insert(edit_data *ed)
{
   const char *paragh = parser_paragh_name_get(ed->pd, ed->en_edit);
   if (!paragh) return;

   if (!strcmp(paragh, "parts"))
     {
        edit_template_part_insert(ed, EDJE_PART_TYPE_IMAGE);
        return;
     }

   int line_cnt;
   char **t = NULL;
   char buf[64];
   char buf2[12];

   if (!strcmp(paragh, "part"))
     {
        line_cnt = TEMPLATE_DESC_LINE_CNT;
        t = (char **) &TEMPLATE_DESC;
        strcpy(buf2, "Description");
     }
   else if (!strcmp(paragh, "programs"))
     {
        line_cnt = TEMPLATE_PROG_LINE_CNT;
        t = (char **) &TEMPLATE_PROG;
        strcpy(buf2, "Program");
     }
   else if (!strcmp(paragh, "images"))
     {
        line_cnt = TEMPLATE_IMG_LINE_CNT;
        t = (char **) &TEMPLATE_IMG;
        strcpy(buf2, "Image File");
     }

   if (!t)
     {
        stats_info_msg_update(ed->sd,
                              "Can't insert template code here. Move the cursor inside the \"Images|Parts|Part\" scope.");
        return;
     }

   int cursor_pos = elm_entry_cursor_pos_get(ed->en_edit);
   elm_entry_cursor_line_begin_set(ed->en_edit);
   int space = indent_space_get(ed);

   //Alloc Empty spaces
   char *p = alloca(space) + 1;
   memset(p, ' ', space);
   p[space] = '\0';

   int i;
   for (i = 0; i < line_cnt; i++)
     {
        elm_entry_entry_insert(ed->en_edit, p);
        elm_entry_entry_insert(ed->en_edit, t[i]);
        last_line_inc(ed);
     }

   elm_entry_cursor_pos_set(ed->en_edit, cursor_pos);

   if (ed->syntax_color_timer) ecore_timer_del(ed->syntax_color_timer);
     ed->syntax_color_timer = ecore_timer_add(SYNTAX_COLOR_TIME,
                                              syntax_color_timer_cb, ed);

     snprintf(buf, sizeof(buf), "Template code inserted. (%s)", buf2);
     stats_info_msg_update(ed->sd, buf);
}

void
edit_template_part_insert(edit_data *ed, Edje_Part_Type type)
{
   if (type == EDJE_PART_TYPE_NONE) return;

   int cursor_pos = elm_entry_cursor_pos_get(ed->en_edit);
   elm_entry_cursor_line_begin_set(ed->en_edit);
   int space = indent_space_get(ed);

   //Alloc Empty spaces
   char *p = alloca(space) + 1;
   memset(p, ' ', space);
   p[space] = '\0';

   int line_cnt;
   char **t;
   char buf[64];
   char part[20];

   switch(type)
     {
        case EDJE_PART_TYPE_RECTANGLE:
           line_cnt = TEMPLATE_PART_RECT_LINE_CNT;
           t = (char **) &TEMPLATE_PART_RECT;
           strcpy(part, "Rect");
           break;
        case EDJE_PART_TYPE_TEXT:
           line_cnt = TEMPLATE_PART_TEXT_LINE_CNT;
           t = (char **) &TEMPLATE_PART_TEXT;
           strcpy(part, "Text");
           break;
        case EDJE_PART_TYPE_SWALLOW:
           line_cnt = TEMPLATE_PART_SWALLOW_LINE_CNT;
           t = (char **) &TEMPLATE_PART_SWALLOW;
           strcpy(part, "Swallow");
           break;
        case EDJE_PART_TYPE_TEXTBLOCK:
           line_cnt = TEMPLATE_PART_TEXTBLOCK_LINE_CNT;
           t = (char **) &TEMPLATE_PART_TEXTBLOCK;
           strcpy(part, "Textblock");
           break;
        case EDJE_PART_TYPE_SPACER:
           line_cnt = TEMPLATE_PART_SPACER_LINE_CNT;
           t = (char **) &TEMPLATE_PART_SPACER;
           strcpy(part, "Spacer");
           break;
        case EDJE_PART_TYPE_IMAGE:
        defaut:
           line_cnt = TEMPLATE_PART_IMAGE_LINE_CNT;
           t = (char **) &TEMPLATE_PART_IMAGE;
           strcpy(part, "Image");
           break;
     }

   int i;
   for (i = 0; i < line_cnt; i++)
     {
        elm_entry_entry_insert(ed->en_edit, p);
        elm_entry_entry_insert(ed->en_edit, t[i]);
        last_line_inc(ed);
     }

   elm_entry_cursor_pos_set(ed->en_edit, cursor_pos);

   if (ed->syntax_color_timer) ecore_timer_del(ed->syntax_color_timer);
     ed->syntax_color_timer = ecore_timer_add(SYNTAX_COLOR_TIME,
                                              syntax_color_timer_cb, ed);

     snprintf(buf, sizeof(buf), "Template code inserted. (%s Part)", part);
     stats_info_msg_update(ed->sd, buf);
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
   if (!config_stats_bar_get(ed->cd)) return;

   Evas_Coord y, h;
   elm_entry_cursor_geometry_get(ed->en_edit, NULL, &y, NULL, &h);
   stats_line_num_update(ed->sd, (y / h) + 1, ed->line_max);
}

static void
program_run(edit_data *ed, char *cur)
{
   char *program = parser_name_get(ed->pd, cur);
   if (program)
     {
        view_program_run(ed->vd, program);
        free(program);
     }
}

static void
image_preview_show(edit_data *ed, char *cur)
{
   char *filename = parser_name_get(ed->pd, cur);
   if (!filename) return;

   char fullpath[PATH_MAX];

   //1.Find the image path.
   Eina_List *list = config_edc_img_path_list_get(ed->cd);
   Eina_List *l;
   char *path;
   Eina_Bool found = EINA_FALSE;

   EINA_LIST_FOREACH(list, l, path)
     {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);
        if (!ecore_file_exists(fullpath)) continue;
        found = EINA_TRUE;
        break;
     }

   if (found)
     {
        Evas_Object *ctxpopup =
           ctxpopup_img_preview_create(ed->parent,
                                       fullpath,
                                       ctxpopup_preview_dismiss_cb,
                                       NULL,
                                       ed);
        if (!ctxpopup) return;

        int x, y;
        evas_pointer_output_xy_get(evas_object_evas_get(ed->en_edit), &x, &y);
        evas_object_move(ctxpopup, x, y);
        evas_object_show(ctxpopup);
        menu_ctxpopup_register(ctxpopup);
        elm_object_disabled_set(ed->layout, EINA_TRUE);
     }
   free(filename);
}

static void
candidate_list_show(edit_data *ed, char *text, char *cur, char *selected)
{
   attr_value * attr = parser_attribute_get(ed->pd, text, cur);
   if (!attr) return;

   //Show up the list of the types
   Evas_Object *ctxpopup =
      ctxpopup_candidate_list_create(ed->parent, attr,
                                     atof(selected),
                                     ctxpopup_candidate_dismiss_cb,
                                     ctxpopup_candidate_selected_cb, ed);
   if (!ctxpopup) return;

   int x, y;
   evas_pointer_output_xy_get(evas_object_evas_get(ed->en_edit), &x, &y);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
   menu_ctxpopup_register(ctxpopup);
   elm_object_disabled_set(ed->layout, EINA_TRUE);
}

static void
edit_cursor_double_clicked_cb(void *data, Evas_Object *obj,
                              void *event_info EINA_UNUSED)
{
   edit_data *ed = data;

   if (ed->ctrl_pressed) return;

   char *selected = (char *) elm_entry_selection_get(obj);
   if (!selected) return;
   selected = elm_entry_markup_to_utf8(selected);

   Evas_Object *textblock = elm_entry_textblock_get(obj);
   Evas_Textblock_Cursor *cursor = evas_object_textblock_cursor_get(textblock);
   const char *str = evas_textblock_cursor_paragraph_text_get(cursor);
   char *text = elm_entry_markup_to_utf8(str);
   char *cur = strstr(text, selected);

   if (!strcmp(selected, "program"))
     {
        program_run(ed, cur);
     }
   else if ((!strncmp(selected, "image", 5)) ||  //5: sizeof("image")
            (!strcmp(selected, "normal")))
     {
        image_preview_show(ed, cur);
     }
   else
     {
        candidate_list_show(ed, text, cur, selected);
     }
end:
   if (selected) free(selected);
   if (text) free(text);
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
   if (!config_part_highlight_get(ed->cd)) return;

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
edit_init(Evas_Object *parent, stats_data *sd, config_data *cd)
{
   parser_data *pd = parser_init();
   syntax_helper *sh = syntax_init();

   edit_data *ed = calloc(1, sizeof(edit_data));
   ed->sd = sd;
   ed->pd = pd;
   ed->sh = sh;
   ed->cd = cd;

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

   syntax_helper *sh = ed->sh;
   parser_data *pd = ed->pd;

   if (ed->group_name) eina_stringshare_del(ed->group_name);
   if (ed->syntax_color_timer) ecore_timer_del(ed->syntax_color_timer);
   free(ed);

   syntax_term(sh);
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
        //Append edc ccde
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
   Eina_Bool linenumber = config_linenumber_get(ed->cd);
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
   edit_edc_read(ed, config_edc_path_get(ed->cd));
   ed->edit_changed = EINA_TRUE;

   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "File Path: \"%s\"",
            config_edc_path_get(ed->cd));
   stats_info_msg_update(ed->sd, buf);
}

const char *
edit_group_name_get(edit_data *ed)
{
   return ed->group_name;
}

void
edit_font_size_update(edit_data *ed, Eina_Bool msg)
{
   elm_object_scale_set(ed->en_edit, config_font_size_get(ed->cd));
   elm_object_scale_set(ed->en_line, config_font_size_get(ed->cd));

   if (!msg) return;

   char buf[128];
   snprintf(buf, sizeof(buf), "Font Size: %1.1fx",
            config_font_size_get(ed->cd));
   stats_info_msg_update(ed->sd, buf);
}
