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
   parser_data *pd;

   int cur_line;
   int line_max;

   Ecore_Idler *syntax_color_timer;

   void (*view_sync_cb)(void *data, Eina_Stringshare *part_name,
                         Eina_Stringshare *group_name);
   void *view_sync_cb_data;

   Eina_Bool edit_changed : 1;
   Eina_Bool linenumber : 1;
   Eina_Bool ctrl_pressed : 1;
};

static Eina_Bool image_preview_show(edit_data *ed, char *cur, Evas_Coord x, Evas_Coord y);

static void
line_increase(edit_data *ed)
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

   Evas_Textblock_Cursor *cur2 = evas_object_textblock_cursor_new(textblock);
   evas_textblock_cursor_line_set(cur2, ed->line_max);
   evas_textblock_cursor_paragraph_last(cur2);

   evas_textblock_cursor_range_delete(cur1, cur2);

   evas_textblock_cursor_free(cur1);
   evas_textblock_cursor_free(cur2);

   elm_entry_calc_force(ed->en_line);

   ed->line_max -= cnt;

   if (ed->line_max < 0) ed->line_max = 0;
}

static void
syntax_color_apply(edit_data *ed)
{
   //FIXME: Optimize here by applying color syntax for only changed lines 
   char *text = (char *) elm_entry_entry_get(ed->en_edit);
   int pos = elm_entry_cursor_pos_get(ed->en_edit);
   char *utf8 = (char *) color_cancel(syntax_color_data_get(ed->sh), text,
                                      strlen(text));
   if (!utf8) return;
   utf8 = strdup(utf8);
   const char *translated = color_apply(syntax_color_data_get(ed->sh), utf8,
                                        strlen(utf8));
   elm_entry_entry_set(ed->en_edit, NULL);
   elm_entry_entry_append(ed->en_edit, translated);
   elm_entry_cursor_pos_set(ed->en_edit, pos);
   //FIXME: Need to recover selection area.
   free(utf8);
}

static Eina_Bool
syntax_color_timer_cb(void *data)
{
   edit_data *ed = data;
   syntax_color_apply(ed);
   ed->syntax_color_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
syntax_color_timer_update(edit_data *ed)
{
   if (ed->syntax_color_timer) ecore_timer_del(ed->syntax_color_timer);
   ed->syntax_color_timer = ecore_timer_add(SYNTAX_COLOR_TIME,
                                            syntax_color_timer_cb, ed);
}

static void
edit_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Entry_Change_Info *info = event_info;
   edit_data *ed = data;
   ed->edit_changed = EINA_TRUE;

   Eina_Bool syntax_color = EINA_TRUE;

   if (info->insert)
     {
        if ((info->change.insert.plain_length == 1)&&
            (info->change.insert.content[0] == ' ')) return;

        if (!strcmp(info->change.insert.content, "<br/>"))
          {
             line_increase(ed);
             syntax_color = EINA_FALSE;
          }

        if (config_auto_indent_get())
          indent_insert_apply(syntax_indent_data_get(ed->sh), ed->en_edit,
                              info->change.insert.content, ed->cur_line);
     }
   else
     {
        int decrease = parser_line_cnt_get(ed->pd, info->change.del.content);

        if (config_auto_indent_get())
          {
             if (indent_delete_apply(syntax_indent_data_get(ed->sh),
                                     ed->en_edit, info->change.del.content,
                                     ed->cur_line))
               decrease++;
          }

        line_decrease(ed, decrease);
        if (info->change.del.content[0] == ' ') return;
     }

   if (!syntax_color) return;
   /* FIXME: after searching the text, it couldn't recover the selected text
      right after applying syntax color. This workaround makes avoid to not
      applying syntax color while entry has the selected text. */
   if (elm_entry_selection_get(ed->en_edit)) return;
   syntax_color_timer_update(ed);
}

static void
save_msg_show(edit_data *ed)
{
   if (!config_stats_bar_get()) return;

   char buf[PATH_MAX];

   if (ed->edit_changed)
     snprintf(buf, sizeof(buf), "File saved. \"%s\"", config_edc_path_get());
   else
     snprintf(buf, sizeof(buf), "Already saved. \"%s\"", config_edc_path_get());

   stats_info_msg_update(buf);
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
   char *utf8 = elm_entry_markup_to_utf8(text);

   FILE *fp = fopen(config_edc_path_get(), "w");
   if (!fp) return EINA_FALSE;

   fputs(utf8, fp);
   fclose(fp);

   free(utf8);

   save_msg_show(ed);
   //FIXME: If compile edc here? we can edit_changed FALSE;
   //ed->edit_changed = EINA_FALSE;

   edit_view_sync(ed);

   return EINA_TRUE;
}

static void
ctxpopup_candidate_dismiss_cb(void *data, Evas_Object *obj,
                              void *event_info EINA_UNUSED)
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
ctxpopup_preview_dismiss_cb(void *data, Evas_Object *obj,
                            void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   int skip_focus = (int) evas_object_data_get(obj, "continue");
   evas_object_del(obj);

   //Since the ctxpopup will be shown again, Don't revert the focus.
   if (skip_focus) return;
   elm_object_disabled_set(ed->layout, EINA_FALSE);
   elm_object_focus_set(ed->en_edit, EINA_TRUE);
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
   else if (!strcmp(paragh, "collections"))
     {
        line_cnt = TEMPLATE_GROUP_LINE_CNT;
        t = (char **) &TEMPLATE_GROUP;
        strcpy(buf2, "Group");
     }

   if (!t)
     {
        stats_info_msg_update("Can't insert template code here. Move the cursor inside the \"Images|Parts|Part\" scope.");
        return;
     }

   int cursor_pos = elm_entry_cursor_pos_get(ed->en_edit);
   elm_entry_cursor_line_begin_set(ed->en_edit);
   int space = indent_space_get(syntax_indent_data_get(ed->sh), ed->en_edit);

   //Alloc Empty spaces
   char *p = alloca(space + 1);
   memset(p, ' ', space);
   p[space] = '\0';

   int i;
   for (i = 0; i < (line_cnt - 1); i++)
     {
        elm_entry_entry_insert(ed->en_edit, p);
        elm_entry_entry_insert(ed->en_edit, t[i]);
        //Incease line by (line count - 1)
        line_increase(ed);
     }

   elm_entry_entry_insert(ed->en_edit, p);
   elm_entry_entry_insert(ed->en_edit, t[i]);

   elm_entry_cursor_pos_set(ed->en_edit, cursor_pos);

   syntax_color_timer_update(ed);
   snprintf(buf, sizeof(buf), "Template code inserted. (%s)", buf2);
   stats_info_msg_update(buf);
}

void
edit_template_part_insert(edit_data *ed, Edje_Part_Type type)
{
   if (type == EDJE_PART_TYPE_NONE) return;

   int cursor_pos = elm_entry_cursor_pos_get(ed->en_edit);
   elm_entry_cursor_line_begin_set(ed->en_edit);
   int space = indent_space_get(syntax_indent_data_get(ed->sh), ed->en_edit);

   //Alloc Empty spaces
   char *p = alloca(space + 1);
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
        case EDJE_PART_TYPE_NONE:
        case EDJE_PART_TYPE_GRADIENT:
        case EDJE_PART_TYPE_GROUP:
        case EDJE_PART_TYPE_BOX:
        case EDJE_PART_TYPE_TABLE:
        case EDJE_PART_TYPE_EXTERNAL:
        case EDJE_PART_TYPE_PROXY:
        case EDJE_PART_TYPE_LAST:
           line_cnt = TEMPLATE_PART_IMAGE_LINE_CNT;
           t = (char **) &TEMPLATE_PART_IMAGE;
           strcpy(part, "Image");
           break;
     }

   int i;
   for (i = 0; i < (line_cnt - 1); i++)
     {
        elm_entry_entry_insert(ed->en_edit, p);
        elm_entry_entry_insert(ed->en_edit, t[i]);
        //Incease line by (line count - 1)
        line_increase(ed);
     }

   elm_entry_entry_insert(ed->en_edit, p);
   elm_entry_entry_insert(ed->en_edit, t[i]);

   elm_entry_cursor_pos_set(ed->en_edit, cursor_pos);

   syntax_color_timer_update(ed);
   snprintf(buf, sizeof(buf), "Template code inserted. (%s Part)", part);
   stats_info_msg_update(buf);
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
   Evas_Coord y, h;
   elm_entry_cursor_geometry_get(ed->en_edit, NULL, &y, NULL, &h);
   int line = (y / h) + 1;
   if (line < 0) line = 0;
   if (ed->cur_line == line) return;
   ed->cur_line = line;

   if (!config_stats_bar_get()) return;
   stats_line_num_update(ed->cur_line, ed->line_max);
}

static void
program_run(edit_data *ed, char *cur)
{
   char *program = parser_name_get(ed->pd, cur);
   if (program)
     {
        view_data *vd = edj_mgr_view_get(NULL);
        view_program_run(vd, program);
        free(program);
     }
}

//This function is called when user press up/down key or mouse wheel up/down
static void
preview_img_relay_show(edit_data *ed, Evas_Object *ctxpopup, Eina_Bool next)
{
   if (next) elm_entry_cursor_down(ed->en_edit);
   else elm_entry_cursor_up(ed->en_edit);

   Evas_Object *textblock = elm_entry_textblock_get(ed->en_edit);
   Evas_Textblock_Cursor *cursor = evas_object_textblock_cursor_get(textblock);
   const char *str = evas_textblock_cursor_paragraph_text_get(cursor);
   char *text = elm_entry_markup_to_utf8(str);

   //Compute current ctxpopup position.
   Evas_Coord x, y, h;
   evas_object_geometry_get(ctxpopup, &x, &y, NULL, NULL);
   elm_entry_cursor_geometry_get(ed->en_edit, NULL, NULL, NULL, &h);

   if (next) y += h;
   else y -= h;

   //Limit the ctxpopup position in the scroller vertical zone.
   Evas_Coord scrl_y, scrl_h;
   evas_object_geometry_get(ed->scroller, NULL, &scrl_y, NULL, &scrl_h);

   if (y > (scrl_y + scrl_h)) y = (scrl_y + scrl_h);
   else if (y < scrl_y) y = scrl_y;

   if (image_preview_show(ed, text, x, y))
     {
        /* Since the ctxpopup will be shown again,
           Don't revert the focus in the dismiss cb. */
        evas_object_data_set(ctxpopup, "continue", (void *) 1);
     }

   menu_ctxpopup_unregister(ctxpopup);
   elm_ctxpopup_dismiss(ctxpopup);
}

static void
ctxpopup_preview_relay_cb(void *data, Evas_Object *obj, void *event_info)
{
   edit_data *ed = data;
   int next = (int) event_info;
   preview_img_relay_show(ed, obj, (Eina_Bool) next);
}

static Eina_Bool
image_preview_show(edit_data *ed, char *cur, Evas_Coord x, Evas_Coord y)
{
   char *filename = parser_name_get(ed->pd, cur);
   if (!filename) return EINA_FALSE;

   char fullpath[PATH_MAX];

   //1.Find the image path.
   Eina_List *list = config_edc_img_path_list_get();
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

   Eina_Bool succeed;

   //Create Ctxpopup with the image pathes.
   if (found)
     {
        Evas_Object *ctxpopup =
           ctxpopup_img_preview_create(ed->parent,
                                       fullpath,
                                       ctxpopup_preview_dismiss_cb,
                                       ctxpopup_preview_relay_cb,
                                       ed);
        if (!ctxpopup) return EINA_FALSE;

        evas_object_move(ctxpopup, x, y);
        evas_object_show(ctxpopup);
        menu_ctxpopup_register(ctxpopup);
        elm_object_disabled_set(ed->layout, EINA_TRUE);
        succeed = EINA_TRUE;
     }
   else
     {
        char buf[PATH_MAX];
        snprintf(buf, sizeof(buf), "Failed to load the image. \"%s\"",
                 filename);
        stats_info_msg_update(buf);
        succeed = EINA_FALSE;
     }

   free(filename);

   return succeed;
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
            (!strcmp(selected, "normal")) ||
            (!strcmp(selected, "tween")))
     {
        int x, y;
        evas_pointer_output_xy_get(evas_object_evas_get(ed->en_edit), &x, &y);
        image_preview_show(ed, cur, x, y);
     }
   else
     candidate_list_show(ed, text, cur, selected);

   if (selected) free(selected);
   if (text) free(text);
}

static void
cur_name_get_cb(void *data, Eina_Stringshare *part_name,
                 Eina_Stringshare *group_name)
{
   edit_data *ed = data;

   if (ed->view_sync_cb)
     ed->view_sync_cb(ed->view_sync_cb_data, part_name, group_name);
}

void
edit_view_sync(edit_data *ed)
{
   if (!config_part_highlight_get()) return;

   parser_cur_name_get(ed->pd, ed->en_edit, cur_name_get_cb, ed);
}

static void
edit_cursor_changed_manual_cb(void *data, Evas_Object *obj EINA_UNUSED,
                              void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   edit_view_sync(ed);
}

static void
edit_cursor_changed_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   cur_line_pos_set(ed);
}

void
edit_view_sync_cb_set(edit_data *ed,
                      void (*cb)(void *data, Eina_Stringshare *part_name,
                                 Eina_Stringshare *group_name), void *data)
{
   ed->view_sync_cb = cb;
   ed->view_sync_cb_data = data;
}

void
edit_line_delete(edit_data *ed)
{
   if (!elm_object_focus_get(ed->en_edit)) return;

   Evas_Object *textblock = elm_entry_textblock_get(ed->en_edit);

   int line1 = ed->cur_line - 1;
   int line2 = ed->cur_line;

   //min position case
   if (line1 < 0)
     {
        line1 = 0;
        line2 = 1;
     }

   //Max position case
   Eina_Bool max = EINA_FALSE;
   if (line2 >= ed->line_max)
     {
        line1 = (ed->line_max - 2);
        line2 = (ed->line_max - 1);
        max = EINA_TRUE;
     }

   //only one line remain. clear it.
   if (ed->line_max == 1)
     {
        elm_entry_entry_set(ed->en_edit, "");
        line_decrease(ed, 1);
        return;
     }

   Evas_Textblock_Cursor *cur1 = evas_object_textblock_cursor_new(textblock);
   evas_textblock_cursor_line_set(cur1, line1);
   if (max) evas_textblock_cursor_line_char_last(cur1);

   Evas_Textblock_Cursor *cur2 = evas_object_textblock_cursor_new(textblock);
   evas_textblock_cursor_line_set(cur2, line2);
   if (max) evas_textblock_cursor_line_char_last(cur2);

   evas_textblock_cursor_range_delete(cur1, cur2);
   evas_textblock_cursor_free(cur1);
   evas_textblock_cursor_free(cur2);
   elm_entry_calc_force(ed->en_edit);

   line_decrease(ed, 1);
}

static Eina_Bool
key_down_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   edit_data *ed = data;

   //Control Key
   if (!strcmp("Control_L", event->key))
     ed->ctrl_pressed = EINA_TRUE;

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
key_up_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   edit_data *ed = data;

   //Control Key
   if (!strcmp("Control_L", event->key))
     ed->ctrl_pressed = EINA_FALSE;

   return ECORE_CALLBACK_PASS_ON;
}

edit_data *
edit_init(Evas_Object *parent)
{
   parser_data *pd = parser_init();
   syntax_helper *sh = syntax_init();

   edit_data *ed = calloc(1, sizeof(edit_data));
   ed->pd = pd;
   ed->sh = sh;

   ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, key_down_cb, ed);
   ecore_event_handler_add(ECORE_EVENT_KEY_UP, key_up_cb, ed);

   //Scroller
   Evas_Object *scroller = elm_scroller_add(parent);
   elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_AUTO,
                           ELM_SCROLLER_POLICY_AUTO);
   elm_object_focus_allow_set(scroller, EINA_FALSE);
   evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);

   //Layout
   Evas_Object *layout = elm_layout_add(scroller);
   elm_layout_file_set(layout, EDJE_PATH,  "edit_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(scroller, layout);

   //Line Number Entry
   Evas_Object *en_line = elm_entry_add(layout);
   evas_object_color_set(en_line, 101, 101, 101, 255);
   elm_entry_editable_set(en_line, EINA_FALSE);
   elm_entry_line_wrap_set(en_line, EINA_FALSE);
   evas_object_size_hint_weight_set(en_line, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en_line, 0, EVAS_HINT_FILL);
   elm_object_part_content_set(layout, "elm.swallow.linenumber", en_line);

   //EDC Editor Entry
   Evas_Object *en_edit = elm_entry_add(layout);
   elm_entry_context_menu_disabled_set(en_edit, EINA_TRUE);
   elm_entry_line_wrap_set(en_edit, EINA_FALSE);
   evas_object_smart_callback_add(en_edit, "changed,user", edit_changed_cb, ed);
   evas_object_smart_callback_add(en_edit, "cursor,changed,manual",
                                  edit_cursor_changed_manual_cb, ed);
   evas_object_smart_callback_add(en_edit, "cursor,changed",
                                  edit_cursor_changed_cb, ed);
   evas_object_smart_callback_add(en_edit, "clicked,double",
                                  edit_cursor_double_clicked_cb, ed);
   evas_object_size_hint_weight_set(en_edit, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en_edit, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_event_callback_add(en_edit, EVAS_CALLBACK_MOUSE_DOWN,
                                  edit_mouse_down_cb, ed);
   elm_object_focus_set(en_edit, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.edit", en_edit);

   search_entry_register(en_edit);

   ed->scroller = scroller;
   ed->en_line = en_line;
   ed->en_edit = en_edit;
   ed->layout = layout;
   ed->parent = parent;
   ed->linenumber = EINA_TRUE;
   ed->cur_line = -1;

   edit_line_number_toggle(ed);
   edit_font_size_update(ed, EINA_FALSE);

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
   Eina_Stringshare *group_name =
      parser_first_group_name_get(ed->pd, ed->en_edit);

   stats_edc_group_update(group_name);
   stats_line_num_update(0, ed->line_max);
   base_title_set(config_edc_path_get());

   ecore_animator_add(syntax_color_timer_cb, ed);

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
   //FIXME: edit & config toogle should be handled in one place.
   Eina_Bool linenumber = config_linenumber_get();
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
   parser_cancel(ed->pd);
   elm_entry_entry_set(ed->en_edit, "");
   elm_entry_entry_set(ed->en_line, "");
   edit_edc_read(ed, config_edc_path_get());
   ed->edit_changed = EINA_TRUE;

   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "File Path: \"%s\"", config_edc_path_get());
   stats_info_msg_update(buf);
}

void
edit_font_size_update(edit_data *ed, Eina_Bool msg)
{
   elm_object_scale_set(ed->layout, config_font_size_get());

   if (!msg) return;

   char buf[128];
   snprintf(buf, sizeof(buf), "Font Size: %1.1fx", config_font_size_get());
   stats_info_msg_update(buf);
}

void
edit_part_highlight_toggle(edit_data *ed, Eina_Bool msg)
{
   Eina_Bool highlight = config_part_highlight_get();
   if (highlight) edit_view_sync(ed);
   else view_part_highlight_set(VIEW_DATA, NULL);

   if (!msg) return;

   if (highlight)
     stats_info_msg_update("Part Highlighting Enabled.");
   else
     stats_info_msg_update("Part Highlighting Disabled.");
}

void
edit_edc_reload(edit_data *ed, const char *edc_path)
{
   config_edc_path_set(edc_path);
   edit_new(ed);
   edj_mgr_reload_need_set(EINA_TRUE);
   config_apply();
}
