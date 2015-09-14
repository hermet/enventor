#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#define ENVENTOR_BETA_API_SUPPORT 1

#include <Enventor.h>
#include <Eio.h>
#include "enventor_private.h"

//FIXME: Make flexible
const int MAX_LINE_DIGIT_CNT = 10;
const int SYNTAX_COLOR_SPARE_LINES = 42;
const double SYNTAX_COLOR_DEFAULT_TIME = 0.25;
const double SYNTAX_COLOR_SHORT_TIME = 0.025;

typedef struct syntax_color_thread_data_s
{
   edit_data *ed;
   char *text;
   const char *translated;
} syntax_color_td;

struct editor_s
{
   Evas_Object *en_edit;
   Evas_Object *en_line;
   Evas_Object *scroller;
   Evas_Object *layout;
   Evas_Object *ctxpopup;
   Evas_Object *enventor;

   syntax_helper *sh;
   parser_data *pd;
   redoundo_data *rd;

   int cur_line;
   int line_max;
   int error_line;
   int syntax_color_lock;
   Evas_Coord scroller_h;

   Ecore_Timer *syntax_color_timer;
   Ecore_Thread *syntax_color_thread;

   void (*view_sync_cb)(void *data, Eina_Stringshare *state_name, double state_value,
                        Eina_Stringshare *part_name, Eina_Stringshare *group_name);
   void *view_sync_cb_data;
   int select_pos;
   double font_scale;
   const char *font_name;
   const char *font_style;
   const char *error_target;

   Eina_Bool edit_changed : 1;
   Eina_Bool linenumber : 1;
   Eina_Bool ctrl_pressed : 1;
   Eina_Bool on_select_recover : 1;
   Eina_Bool auto_indent : 1;
   Eina_Bool part_highlight : 1;
   Eina_Bool ctxpopup_enabled : 1;
   Eina_Bool on_save : 1;
};

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static Eina_Bool
image_preview_show(edit_data *ed, char *cur, Evas_Coord x, Evas_Coord y);
static void
edit_font_apply(edit_data *ed, const char *font_name, const char *font_style);

static void
line_init(edit_data *ed)
{
   char buf[MAX_LINE_DIGIT_CNT];

   ed->line_max = 1;
   snprintf(buf, sizeof(buf), "%d", ed->line_max);
   elm_entry_entry_set(ed->en_line, buf);
}

static void
visible_text_region_get(edit_data *ed, int *from_line, int *to_line)
{
   Evas_Coord region_y, region_h;
   Evas_Coord cursor_h;

   elm_scroller_region_get(ed->scroller, NULL, &region_y, NULL, &region_h);
   elm_entry_cursor_geometry_get(ed->en_edit, NULL, NULL, NULL, &cursor_h);

   int from = (region_y / cursor_h);
   int to = from + (region_h / cursor_h);
   from -= SYNTAX_COLOR_SPARE_LINES;
   to += SYNTAX_COLOR_SPARE_LINES;

   if (from < 1) from = 1;
   if (to > ed->line_max) to = ed->line_max;

   *from_line = from;
   *to_line = to;
}

static void
entry_recover(edit_data *ed, int cursor_pos)
{
   elm_entry_calc_force(ed->en_edit);

   //recover cursor position??
   elm_entry_cursor_pos_set(ed->en_edit, 0);
   elm_entry_cursor_pos_set(ed->en_edit, cursor_pos);

   //not on selection mode
   if (ed->select_pos == -1) return;

   //recover selection region
   const char *selected = elm_entry_selection_get(ed->en_edit);
   if (!selected) return;
   char *utf8 = evas_textblock_text_markup_to_utf8(NULL, selected);
   ed->on_select_recover = EINA_TRUE;
   elm_entry_select_region_set(ed->en_edit, ed->select_pos, cursor_pos);
   ed->on_select_recover = EINA_FALSE;
   free(utf8);
}

static void
edit_font_apply(edit_data *ed, const char *font_name, const char *font_style)
{
   char *font = NULL;
   if (font_name)
     font = elm_font_fontconfig_name_get(font_name, font_style);
   edje_text_class_set("enventor_entry", font, -100);
   elm_font_fontconfig_name_free(font);

   elm_entry_calc_force(ed->en_line);
   int pos = elm_entry_cursor_pos_get(ed->en_edit);
   entry_recover(ed, pos);
}

static void
error_highlight(edit_data *ed, Evas_Object *tb)
{
   Evas_Textblock_Cursor *cur1 = evas_object_textblock_cursor_new(tb);
   if (ed->error_line)
     {
        evas_textblock_cursor_line_set(cur1, ed->error_line);
        evas_textblock_cursor_line_char_first(cur1);
        while(evas_textblock_cursor_content_get(cur1)[0] == ' ')
           evas_textblock_cursor_char_next(cur1);
        evas_object_textblock_text_markup_prepend(cur1, "<error>");
        evas_textblock_cursor_line_char_last(cur1);
        evas_object_textblock_text_markup_prepend(cur1, "</error>");
     }
   else if (ed->error_target)
     {
        const char *ptr = NULL;
        const char *par = NULL;
        while (evas_textblock_cursor_paragraph_next(cur1))
          {
             par = evas_textblock_cursor_paragraph_text_get(cur1);
             if (par && (ptr = strstr(par, ed->error_target)))
                break;
          }
        evas_textblock_cursor_paragraph_char_first(cur1);
        while(evas_textblock_cursor_content_get(cur1)[0] == ' ')
           evas_textblock_cursor_char_next(cur1);
        evas_object_textblock_text_markup_prepend(cur1, "<error>");
        evas_textblock_cursor_paragraph_char_last(cur1);
        evas_object_textblock_text_markup_prepend(cur1, "</error>");
     }
   evas_textblock_cursor_free(cur1);
}
static void
syntax_color_apply(edit_data *ed, Eina_Bool partial)
{
   Evas_Object *tb = elm_entry_textblock_get(ed->en_edit);
   char *text = (char *) evas_object_textblock_text_markup_get(tb);
   int pos = elm_entry_cursor_pos_get(ed->en_edit);

   int from_line = 1;
   int to_line = -1;
   if (partial) visible_text_region_get(ed, &from_line, &to_line);

   char *from = NULL;
   char *to = NULL;
   char *utf8 = (char *) color_cancel(syntax_color_data_get(ed->sh), text,
                                      strlen(text), from_line, to_line, &from,
                                      &to);
   if (!utf8) return;

   const char *translated = color_apply(syntax_color_data_get(ed->sh), utf8,
                                        strlen(utf8), from, to);
   if (!translated) return;

   /* I'm not sure this will be problem.
      But it can avoid entry_object_text_escaped_set() in Edje.
      Logically that's unnecessary in this case. */
   evas_object_textblock_text_markup_set(tb, translated);
   error_highlight(ed, tb);
   entry_recover(ed, pos);
}

static Eina_Bool
syntax_color_timer_cb(void *data)
{
   edit_data *ed = data;
   if (!color_ready(syntax_color_data_get(ed->sh))) return ECORE_CALLBACK_RENEW;
   syntax_color_apply(ed, EINA_TRUE);
   ed->syntax_color_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
syntax_color_partial_update(edit_data *ed, double interval)
{
   /* If the syntax_color_full_update is requested forcely, lock would be -1
      in this case, it should avoid partial updation by entry changed. */
   if (ed->syntax_color_lock != 0) return;
   ecore_thread_cancel(ed->syntax_color_thread);
   ed->syntax_color_thread = NULL;
   ecore_timer_del(ed->syntax_color_timer);
   ed->syntax_color_timer = ecore_timer_add(interval, syntax_color_timer_cb, ed);
}

static void
syntax_color_thread_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   syntax_color_td *td = data;
   char *utf8 = (char *) color_cancel(syntax_color_data_get(td->ed->sh),
                                      td->text, strlen(td->text), -1, -1, NULL,
                                      NULL);
   if (!utf8) return;
   td->translated = color_apply(syntax_color_data_get(td->ed->sh), utf8,
                                strlen(utf8), NULL, NULL);
}

static void
syntax_color_thread_end_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   syntax_color_td *td = data;
   if (!td->translated) return;

   int pos = elm_entry_cursor_pos_get(td->ed->en_edit);
   Evas_Object *tb = elm_entry_textblock_get(td->ed->en_edit);

   /* I'm not sure this will be problem.
      But it can avoid entry_object_text_escaped_set() in Edje.
      Logically that's unnecessary in this case. */
   evas_object_textblock_text_markup_set(tb, td->translated);
   error_highlight(td->ed, tb);
   entry_recover(td->ed, pos);

   td->ed->syntax_color_thread = NULL;
   free(td);
}

static void
syntax_color_thread_cancel_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   syntax_color_td *td = data;
   Evas_Object *tb = elm_entry_textblock_get(td->ed->en_edit);
   error_highlight(td->ed, tb);
   td->ed->syntax_color_thread = NULL;
   free(td);
}

static void
edit_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Entry_Change_Info *info = event_info;
   edit_data *ed = data;
   edit_changed_set(ed, EINA_TRUE);

   Eina_Bool syntax_color = EINA_TRUE;
   ed->error_line = 0;
   eina_stringshare_del(ed->error_target);
   ed->error_target = NULL;

   if (info->insert)
     {
        int increase = 0;
        if ((info->change.insert.plain_length == 1)&&
            (info->change.insert.content[0] == ' ')) return;

        if (!strcmp(info->change.insert.content, EOL))
          {
             increase++;
             syntax_color = EINA_FALSE;
          }
        else
          {
             increase =
                parser_line_cnt_get(ed->pd, info->change.insert.content);
          }

        if (edit_auto_indent_get(ed))
          {
             increase =
                indent_insert_apply(syntax_indent_data_get(ed->sh), ed->en_edit,
                                    info->change.insert.content, ed->cur_line);
          }
        edit_line_increase(ed, increase);

     }
   else
     {
        int decrease = parser_line_cnt_get(ed->pd, info->change.del.content);

        if (edit_auto_indent_get(ed))
          {
             if (indent_delete_apply(syntax_indent_data_get(ed->sh),
                                     ed->en_edit, info->change.del.content,
                                     ed->cur_line))
               decrease++;
          }

        edit_line_decrease(ed, decrease);
        if (info->change.del.content[0] == ' ') return;
     }

   if (!syntax_color) return;
   syntax_color_partial_update(ed, SYNTAX_COLOR_DEFAULT_TIME);
}

static void
ctxpopup_candidate_dismiss_cb(void *data, Evas_Object *obj,
                              void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   evas_object_del(obj);
   elm_object_tree_focus_allow_set(ed->layout, EINA_TRUE);
   elm_object_focus_set(ed->en_edit, EINA_TRUE);
   evas_object_smart_callback_call(ed->enventor, SIG_CTXPOPUP_DISMISSED, NULL);
}

static void
ctxpopup_candidate_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
   edit_data *ed = data;
   const char *text = event_info;
   char *ch;
   int cur_pos, end_pos;
   int i;
   cur_pos = elm_entry_cursor_pos_get(ed->en_edit);
   elm_entry_cursor_line_end_set(ed->en_edit);
   end_pos = elm_entry_cursor_pos_get(ed->en_edit);

   for (i = cur_pos; i <= end_pos; i++)
   {
      elm_entry_cursor_pos_set(ed->en_edit, i);
      ch = elm_entry_cursor_content_get(ed->en_edit);
      if (!strcmp(ch, ";"))
        {
           //1 more space for end_pos to replace until ';'.
           end_pos = elm_entry_cursor_pos_get(ed->en_edit) + 1;
           break;
        }
   }

   elm_entry_cursor_pos_set(ed->en_edit, cur_pos);
   elm_entry_cursor_selection_begin(ed->en_edit);
   elm_entry_cursor_pos_set(ed->en_edit, end_pos);
   elm_entry_cursor_selection_end(ed->en_edit);

   redoundo_text_relative_push(ed->rd, text);
   elm_entry_entry_insert(ed->en_edit, text);
   elm_entry_calc_force(ed->en_edit);

   edit_changed_set(ed, EINA_TRUE);
   evas_object_smart_callback_call(ed->enventor, SIG_CTXPOPUP_CHANGED,
                                   (void *)text);
}

static void
ctxpopup_preview_dismiss_cb(void *data, Evas_Object *obj,
                            void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   int skip_focus = (int)(uintptr_t) evas_object_data_get(obj, "continue");

   //Since the ctxpopup will be shown again, Don't revert the focus.
   if (skip_focus) return;
   elm_object_tree_focus_allow_set(ed->layout, EINA_TRUE);
   elm_object_focus_set(ed->en_edit, EINA_TRUE);
   evas_object_smart_callback_call(ed->enventor, SIG_CTXPOPUP_DISMISSED, NULL);
   evas_object_del(obj);
}

static void
ctxpopup_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   ed->ctxpopup = NULL;
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
        evas_object_event_callback_del(ctxpopup, EVAS_CALLBACK_DEL,
                                       ctxpopup_del_cb);

        //Set the entry selection region to next image.
        const char *colon = parser_colon_pos_get(NULL, text);
        if (!colon) goto end;

        const char *selection = elm_entry_selection_get(ed->en_edit);
        if (!selection) goto end;

        char *select_utf8 = elm_entry_markup_to_utf8(selection);
        if (!select_utf8) goto end;
        int select_len = strlen(select_utf8);
        free(select_utf8);
        const char *p = (colon - select_len);
        if (p < text) goto end;

        int cursor_pos = elm_entry_cursor_pos_get(ed->en_edit);
        elm_entry_select_region_set(ed->en_edit, (cursor_pos - select_len),
                                    cursor_pos);
     }
end:
   elm_ctxpopup_dismiss(ctxpopup);
}

static void
ctxpopup_preview_relay_cb(void *data, Evas_Object *obj, void *event_info)
{
   edit_data *ed = data;
   int next = (int)(uintptr_t) event_info;
   preview_img_relay_show(ed, obj, (Eina_Bool) next);
}

static Eina_Bool
image_preview_show(edit_data *ed, char *cur, Evas_Coord x, Evas_Coord y)
{
   char *filename = parser_name_get(ed->pd, cur);
   if (!filename) return EINA_FALSE;

   char fullpath[PATH_MAX];

   //1.Find the image path.
   Eina_List *list = build_path_get(ENVENTOR_PATH_TYPE_IMAGE);
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

   //2. Find in the default fault.
   if (!found)
   {
      snprintf(fullpath, sizeof(fullpath), "%s/images/%s",
               elm_app_data_dir_get(), filename);
      if (ecore_file_exists(fullpath)) found = EINA_TRUE;
   }

   Eina_Bool succeed;

   //Create Ctxpopup with the image pathes.
   if (found)
     {
        Evas_Object *ctxpopup =
           ctxpopup_img_preview_create(ed, fullpath,
                                       ctxpopup_preview_dismiss_cb,
                                       ctxpopup_preview_relay_cb);
        if (!ctxpopup)
          {
             free(filename);
             return EINA_FALSE;
          }

        evas_object_move(ctxpopup, x, y);
        evas_object_show(ctxpopup);
        evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL,
                                       ctxpopup_del_cb, ed);
        ed->ctxpopup = ctxpopup;
        elm_object_tree_focus_allow_set(ed->layout, EINA_FALSE);
        succeed = EINA_TRUE;
     }
   else
     {
        succeed = EINA_FALSE;
     }

   free(filename);

   return succeed;
}

static void
candidate_list_show(edit_data *ed, char *text, char *cur, char *selected)
{
   attr_value * attr = parser_attribute_get(ed->pd, text, cur, selected);
   if (!attr) return;

   parser_attribute_value_set(attr, cur);

   //Show up the list of the types
   Evas_Object *ctxpopup =
      ctxpopup_candidate_list_create(ed, attr,
                                     ctxpopup_candidate_dismiss_cb,
                                     ctxpopup_candidate_changed_cb);
   if (!ctxpopup) return;

   int x, y;
   evas_pointer_output_xy_get(evas_object_evas_get(ed->en_edit), &x, &y);
   evas_object_move(ctxpopup, x, y);
   evas_object_show(ctxpopup);
   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL, ctxpopup_del_cb, ed);
   ed->ctxpopup = ctxpopup;
   elm_object_tree_focus_allow_set(ed->layout, EINA_FALSE);
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

static void
edit_cursor_double_clicked_cb(void *data, Evas_Object *obj,
                              void *event_info EINA_UNUSED)
{
   edit_data *ed = data;

   if (ed->ctrl_pressed) return;
   if (!ed->ctxpopup_enabled) return;

   char *selected = (char *) elm_entry_selection_get(obj);
   if (!selected) return;

   selected = elm_entry_markup_to_utf8(selected);
   if (selected[0] == '\"')
     {
        free(selected);
        return;
     }

   const char *str = elm_entry_entry_get(obj);
   char *text = elm_entry_markup_to_utf8(str);
   int cur_pos = elm_entry_cursor_pos_get(obj);
   char *cur = text + (cur_pos - strlen(selected));

  /* TODO: improve parser_name_get, for recognize cases when name is absent.
   * Because right now any text inside quotes that placed after selection is
   * recognized as name.
   */
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

   free(selected);
   free(text);
}

static void
cur_name_get_cb(void *data, Eina_Stringshare *state_name, double state_value,
                Eina_Stringshare *part_name, Eina_Stringshare *group_name)
{
   edit_data *ed = data;

   if (ed->view_sync_cb)
     ed->view_sync_cb(ed->view_sync_cb_data, state_name, state_value,
                      part_name, group_name);
}

static void
cur_line_pos_set(edit_data *ed, Eina_Bool force)
{
   Evas_Coord y, h;
   elm_entry_cursor_geometry_get(ed->en_edit, NULL, &y, NULL, &h);
   int line = (y / h) + 1;

   if (line < 0) line = 1;
   if (!force && (ed->cur_line == line)) return;
   ed->cur_line = line;

   Enventor_Cursor_Line cur_line;
   cur_line.line = line;
   evas_object_smart_callback_call(ed->enventor, SIG_CURSOR_LINE_CHANGED,
                                   &cur_line);
}

void
edit_selection_clear(edit_data *ed)
{
   if (ed->on_select_recover) return;
   cur_line_pos_set(ed, EINA_TRUE);
   ed->select_pos = -1;
}

static void
edit_selection_cleared_cb(void *data, Evas_Object *obj EINA_UNUSED,
                          void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   edit_selection_clear(ed);
}

static void
edit_selection_start_cb(void *data, Evas_Object *obj EINA_UNUSED,
                        void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   if (ed->select_pos != -1) return;
   ed->select_pos = elm_entry_cursor_pos_get(ed->en_edit);
}

void
edit_text_insert(edit_data *ed, const char *text)
{
   const char *selection = elm_entry_selection_get(ed->en_edit);
   selection = elm_entry_markup_to_utf8(selection);
   if (!selection)
     {
        elm_entry_entry_set(ed->en_edit, text);
        return;
     }
   int lenght = strlen(selection);
   int pos_from = elm_entry_cursor_pos_get(ed->en_edit) - lenght;

   Evas_Object *tb = elm_entry_textblock_get(ed->en_edit);
   Evas_Textblock_Cursor *cur = evas_object_textblock_cursor_get(tb);
   int old_pos = evas_textblock_cursor_pos_get(cur);
   evas_textblock_cursor_pos_set(cur, pos_from + lenght);

   /* append replacement text, and add relative diff into redoundo module */
   evas_textblock_cursor_pos_set(cur, pos_from + lenght);
   evas_textblock_cursor_text_append(cur, text);
   redoundo_text_relative_push(ed->rd, text);

   Evas_Textblock_Cursor *c_1 = evas_object_textblock_cursor_new(tb);
   evas_textblock_cursor_pos_set(c_1, pos_from);

   Evas_Textblock_Cursor *c_2 = evas_object_textblock_cursor_new(tb);
   evas_textblock_cursor_pos_set(c_2, pos_from + lenght);
   /* delete replaced text, and make diff into redoundo module */
   redoundo_text_push(ed->rd, selection, pos_from, lenght, EINA_FALSE);
   evas_textblock_cursor_range_delete(c_1, c_2);

   evas_textblock_cursor_free(c_1);
   evas_textblock_cursor_free(c_2);
   evas_textblock_cursor_pos_set(cur, old_pos);

   elm_entry_calc_force(ed->en_edit);
   free(selection);
}

static void
edit_cursor_changed_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   cur_line_pos_set(ed, EINA_FALSE);
   edit_view_sync(ed);
}

static void
syntax_color_full_update(edit_data *ed, Eina_Bool thread)
{
   if (ed->syntax_color_lock > 0) return;

   ecore_timer_del(ed->syntax_color_timer);
   ed->syntax_color_timer = NULL;

   if (thread)
     {
        syntax_color_td *td = calloc(1, sizeof(syntax_color_td));
        if (!td)
          {
             EINA_LOG_ERR("Failed to allocate Memory!");
             return;
          }
        td->ed = ed;
        Evas_Object *tb = elm_entry_textblock_get(ed->en_edit);
        td->text = (char *) evas_object_textblock_text_markup_get(tb);
        ed->syntax_color_thread =
           ecore_thread_run(syntax_color_thread_cb,
                            syntax_color_thread_end_cb,
                            syntax_color_thread_cancel_cb,
                            td);
     }
   else
     {
        syntax_color_apply(ed, EINA_FALSE);
     }
}

static Eina_Bool
key_down_cb(void *data, int type EINA_UNUSED, void *ev)
{
   Ecore_Event_Key *event = ev;
   edit_data *ed = data;

   /* FIXME: seems event fd handler is broken. so key down event
      is triggered multiple times, to make enventor stable,
      we put the workaround code here. please remove if this won't
      be problem anymore */
   if (event->timestamp == 0) return ECORE_CALLBACK_PASS_ON;

   //Control Key
   if (!strcmp("Control_L", event->key))
     {
        ed->ctrl_pressed = EINA_TRUE;
        return ECORE_CALLBACK_PASS_ON;
     }

   if (ed->ctrl_pressed)
     {
        //Undo
        if (!strcmp(event->key, "z") || !strcmp(event->key, "Z"))
          {
             edit_redoundo(ed, EINA_TRUE);
             return ECORE_CALLBACK_DONE;
          }
        //Redo
        if (!strcmp(event->key, "r") || !strcmp(event->key, "R"))
          {
             edit_redoundo(ed, EINA_FALSE);
             return ECORE_CALLBACK_DONE;
          }
     }

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

static void
scroller_scroll_cb(void *data, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   syntax_color_partial_update(ed, SYNTAX_COLOR_SHORT_TIME);
}

static void
scroller_resize_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                   void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   Evas_Coord h;
   evas_object_geometry_get(obj, NULL, NULL, NULL, &h);

   if (h == ed->scroller_h) return;
   syntax_color_partial_update(ed, SYNTAX_COLOR_DEFAULT_TIME);
   ed->scroller_h = h;
}

static void
scroller_vbar_press_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   syntax_color_full_update(ed, EINA_TRUE);
   ed->syntax_color_lock++;
}

static void
scroller_vbar_unpress_cb(void *data, Evas_Object *obj EINA_UNUSED,
                         void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   ed->syntax_color_lock--;
   syntax_color_partial_update(ed, SYNTAX_COLOR_SHORT_TIME);
}

static Eina_Bool
edit_edc_load(edit_data *ed, const char *file_path)
{
   char buf[MAX_LINE_DIGIT_CNT];

   Eina_File *file = NULL;
   Eina_Strbuf *strbuf_line = NULL;
   Eina_Stringshare *group_name = NULL;
   char *utf8_edit = NULL;
   char *markup_edit = NULL;
   char *markup_line = NULL;
   int line_num = 1;
   Eina_Bool ret = EINA_FALSE;

   ed->line_max = 0;

   file = eina_file_open(file_path, EINA_FALSE);
   if (!file) goto err;

   strbuf_line = eina_strbuf_new();
   if (!strbuf_line) goto err;

   utf8_edit = eina_file_map_all(file, EINA_FILE_POPULATE);
   if (!utf8_edit) goto err;

   //Append line numbers
   if (!eina_strbuf_append_char(strbuf_line, '1')) goto err;
   char *p = utf8_edit;
   int len = strlen(p);
   while ((p = strchr(p, '\n')) && p < (utf8_edit + len))
     {
        line_num++;
        ++p;
        sprintf(buf, "\n%d", line_num);
        if (!eina_strbuf_append(strbuf_line, buf)) goto err;
     }

   markup_line = elm_entry_utf8_to_markup(eina_strbuf_string_get(strbuf_line));
   if (!markup_line) goto err;
   elm_entry_entry_append(ed->en_line, markup_line);
   free(markup_line);

   markup_edit = elm_entry_utf8_to_markup(utf8_edit);
   if (!markup_edit) goto err;
   elm_entry_entry_set(ed->en_edit, markup_edit);
   free(markup_edit);

   ed->line_max = line_num;

   group_name = parser_first_group_name_get(ed->pd, ed->en_edit);

   ecore_animator_add(syntax_color_timer_cb, ed);

   ret = EINA_TRUE;

err:
   //Even any text is not inserted, line number should start with 1
   if (ed->line_max == 0) line_init(ed);
   if (strbuf_line) eina_strbuf_free(strbuf_line);
   if (utf8_edit) eina_file_map_free(file, utf8_edit);
   if (file) eina_file_close(file);

   Enventor_Max_Line max_line;
   max_line.line = ed->line_max;
   evas_object_smart_callback_call(ed->enventor, SIG_MAX_LINE_CHANGED,
                                   &max_line);

   if (ed->view_sync_cb)
     ed->view_sync_cb(ed->view_sync_cb_data, NULL, 0.0, NULL, group_name);

   return ret;
}

static void
edit_focused_cb(void *data, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   edit_data *ed = data;
   evas_object_smart_callback_call(ed->enventor, SIG_FOCUSED, NULL);
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
edit_view_sync_cb_set(edit_data *ed,
                      void (*cb)(void *data, Eina_Stringshare *state_name, double state_value,
                      Eina_Stringshare *part_name, Eina_Stringshare *group_name), void *data)
{
   ed->view_sync_cb = cb;
   ed->view_sync_cb_data = data;
}

Eina_Bool
edit_saved_get(edit_data *ed)
{
   return ed->on_save;
}

void
edit_saved_set(edit_data *ed, Eina_Bool saved)
{
   ed->on_save = saved;
}

Eina_Bool
edit_save(edit_data *ed, const char *file)
{
   if (!ed->edit_changed) return EINA_FALSE;

   const char *text = elm_entry_entry_get(ed->en_edit);
   char *utf8 = elm_entry_markup_to_utf8(text);
   FILE *fp = fopen(file, "w");
   if (!fp)
     {
        EINA_LOG_ERR("Failed to open file \"%s\"", file);
        return EINA_FALSE;
     }

   fputs(utf8, fp);
   fclose(fp);
   free(utf8);

   edit_view_sync(ed);

   edit_changed_set(ed, EINA_FALSE);
   edit_saved_set(ed, EINA_TRUE);

   return EINA_TRUE;
}

void
edit_syntax_color_set(edit_data *ed, Enventor_Syntax_Color_Type color_type,
                      const char *val)
{
   color_set(syntax_color_data_get(ed->sh), color_type, val);
}

const char *
edit_syntax_color_get(edit_data *ed, Enventor_Syntax_Color_Type color_type)
{
   return color_get(syntax_color_data_get(ed->sh), color_type);
}

void
edit_syntax_color_full_apply(edit_data *ed, Eina_Bool force)
{
   int lock;

   if (force)
     {
        lock = ed->syntax_color_lock;
        ed->syntax_color_lock = -1;
     }
   syntax_color_full_update(ed, EINA_FALSE);

   if (force) ed->syntax_color_lock = lock;
   else ed->syntax_color_lock++;
}

void
edit_syntax_color_partial_apply(edit_data *ed, double interval)
{
   if (ed->syntax_color_lock > 0) ed->syntax_color_lock = 0;
   if (interval < 0) syntax_color_partial_update(ed, SYNTAX_COLOR_DEFAULT_TIME);
   else syntax_color_partial_update(ed, interval);
}

void
edit_view_sync(edit_data *ed)
{
   parser_cur_state_get(ed->pd, ed->en_edit, cur_name_get_cb, ed);
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
        redoundo_text_push(ed->rd, elm_entry_entry_get(ed->en_edit), 0, 0,
                           EINA_FALSE);
        elm_entry_entry_set(ed->en_edit, "");
        line_init(ed);
        return;
     }

   Evas_Textblock_Cursor *cur1 = evas_object_textblock_cursor_new(textblock);
   evas_textblock_cursor_line_set(cur1, line1);
   if (max) evas_textblock_cursor_line_char_last(cur1);

   Evas_Textblock_Cursor *cur2 = evas_object_textblock_cursor_new(textblock);
   evas_textblock_cursor_line_set(cur2, line2);
   if (max) evas_textblock_cursor_line_char_last(cur2);
   int cur1_pos, cur2_pos;

   cur1_pos = evas_textblock_cursor_pos_get(cur1);
   cur2_pos = evas_textblock_cursor_pos_get(cur2);
   const char *content = evas_textblock_cursor_range_text_get(cur1, cur2,
                                                    EVAS_TEXTBLOCK_TEXT_MARKUP);

   evas_textblock_cursor_range_delete(cur1, cur2);
   evas_textblock_cursor_free(cur1);
   evas_textblock_cursor_free(cur2);
   redoundo_text_push(ed->rd, content, cur1_pos, abs(cur2_pos - cur1_pos),
                      EINA_FALSE);
   elm_entry_calc_force(ed->en_edit);

   edit_line_decrease(ed, 1);

   cur_line_pos_set(ed, EINA_TRUE);
   edit_changed_set(ed, EINA_TRUE);

   syntax_color_partial_update(ed, SYNTAX_COLOR_DEFAULT_TIME);
}

int
edit_cur_indent_depth_get(edit_data *ed)
{
   return indent_space_get(syntax_indent_data_get(ed->sh), ed->en_edit);
}

edit_data *
edit_init(Evas_Object *enventor)
{
   srand(time(NULL));
   parser_data *pd = parser_init();
   syntax_helper *sh = syntax_init();

   edit_data *ed = calloc(1, sizeof(edit_data));
   if (!ed)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }
   ed->pd = pd;
   ed->sh = sh;

   ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, key_down_cb, ed);
   ecore_event_handler_add(ECORE_EVENT_KEY_UP, key_up_cb, ed);

   //Scroller
   Evas_Object *scroller = elm_scroller_add(enventor);
   elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_AUTO,
                           ELM_SCROLLER_POLICY_AUTO);
   elm_object_focus_allow_set(scroller, EINA_FALSE);
   evas_object_smart_callback_add(scroller, "scroll,up", scroller_scroll_cb,
                                  ed);
   evas_object_smart_callback_add(scroller, "scroll,down", scroller_scroll_cb,
                                  ed);
   evas_object_smart_callback_add(scroller, "vbar,press",
                                  scroller_vbar_press_cb, ed);
   evas_object_smart_callback_add(scroller, "vbar,unpress",
                                  scroller_vbar_unpress_cb, ed);
   evas_object_event_callback_add(scroller, EVAS_CALLBACK_RESIZE,
                                  scroller_resize_cb, ed);
   evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);

   //This is hackish call to not change scroller color by widget.
   evas_object_data_set(scroller, "_elm_leaveme", (void *)1);

   //Layout
   Evas_Object *layout = elm_layout_add(scroller);
   elm_layout_file_set(layout, EDJE_PATH,  "edit_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(scroller, layout);

   //Line Number Entry
   Evas_Object *en_line = elm_entry_add(layout);
   elm_object_style_set(en_line, "enventor");
   evas_object_color_set(en_line, 101, 101, 101, 255);
   elm_entry_editable_set(en_line, EINA_FALSE);
   elm_entry_line_wrap_set(en_line, ELM_WRAP_NONE);
   elm_object_focus_allow_set(en_line, EINA_FALSE);
   evas_object_size_hint_weight_set(en_line, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en_line, 0, EVAS_HINT_FILL);
   elm_object_part_content_set(layout, "elm.swallow.linenumber", en_line);

   //EDC Editor Entry
   Evas_Object *en_edit = elm_entry_add(layout);
   elm_object_style_set(en_edit, "enventor");
   elm_entry_context_menu_disabled_set(en_edit, EINA_TRUE);
   elm_entry_line_wrap_set(en_edit, ELM_WRAP_NONE);
   evas_object_smart_callback_add(en_edit, "focused", edit_focused_cb, ed);
   evas_object_smart_callback_add(en_edit, "changed,user", edit_changed_cb, ed);
   evas_object_smart_callback_add(en_edit, "cursor,changed",
                                  edit_cursor_changed_cb, ed);
   evas_object_smart_callback_add(en_edit, "clicked,double",
                                  edit_cursor_double_clicked_cb, ed);
   evas_object_smart_callback_add(en_edit, "selection,cleared",
                                  edit_selection_cleared_cb, ed);
   evas_object_smart_callback_add(en_edit, "selection,start",
                                  edit_selection_start_cb, ed);
   evas_object_size_hint_weight_set(en_edit, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en_edit, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_focus_set(en_edit, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.edit", en_edit);

   ed->scroller = scroller;
   ed->en_line = en_line;
   ed->en_edit = en_edit;
   ed->layout = layout;
   ed->enventor = enventor;
   ed->linenumber = EINA_TRUE;
   ed->auto_indent = EINA_TRUE;
   ed->part_highlight = EINA_TRUE;
   ed->ctxpopup_enabled = EINA_TRUE;
   ed->cur_line = -1;
   ed->select_pos = -1;
   ed->font_scale = 1;

   ed->rd = redoundo_init(en_edit);
   evas_object_data_set(ed->en_edit, "redoundo", ed->rd);

   return ed;
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

   if (ed->font_name) eina_stringshare_del(ed->font_name);
   if (ed->font_style) eina_stringshare_del(ed->font_style);

   syntax_helper *sh = ed->sh;
   parser_data *pd = ed->pd;

   redoundo_term(ed->rd);
   ecore_thread_cancel(ed->syntax_color_thread);
   ecore_timer_del(ed->syntax_color_timer);
   free(ed);

   syntax_term(sh);
   parser_term(pd);
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

Eina_Bool
edit_linenumber_get(edit_data *ed)
{
   return ed->linenumber;
}

void
edit_linenumber_set(edit_data *ed, Eina_Bool linenumber)
{
   if (ed->linenumber == linenumber) return;
   ed->linenumber = !!linenumber;

   if (linenumber)
     elm_object_signal_emit(ed->layout, "elm,state,linenumber,show", "");
   else
     elm_object_signal_emit(ed->layout, "elm,state,linenumber,hide", "");
}

void
edit_font_scale_set(edit_data *ed, double font_scale)
{
   if (ed->font_scale == font_scale) return;
   elm_object_scale_set(ed->layout, font_scale);
   syntax_color_partial_update(ed, 0);
   ed->font_scale = font_scale;
}

double
edit_font_scale_get(edit_data *ed)
{
   return ed->font_scale;
}

void
edit_font_set(edit_data *ed, const char *font_name, const char *font_style)
{
   eina_stringshare_replace(&ed->font_name, font_name);
   eina_stringshare_replace(&ed->font_style, font_style);
   edit_font_apply(ed, font_name, font_style);
}

void
edit_font_get(edit_data *ed, const char **font_name, const char **font_style)
{
   if (font_name) *font_name = ed->font_name;
   if (font_style) *font_style = ed->font_style;
}

void
edit_part_highlight_set(edit_data *ed, Eina_Bool part_highlight)
{
   part_highlight = !!part_highlight;

   if (ed->part_highlight == part_highlight) return;

   ed->part_highlight = part_highlight;

   if (part_highlight) edit_view_sync(ed);
   else view_part_highlight_set(VIEW_DATA, NULL);
}

Eina_Bool
edit_part_highlight_get(edit_data *ed)
{
   return ed->part_highlight;
}

Eina_Bool
edit_load(edit_data *ed, const char *edc_path)
{
   parser_cancel(ed->pd);
   elm_entry_entry_set(ed->en_edit, "");
   elm_entry_entry_set(ed->en_line, "");
   Eina_Bool ret = edit_edc_load(ed, edc_path);
   if (ret) edit_changed_set(ed, EINA_TRUE);
   edj_mgr_reload_need_set(EINA_TRUE);
   redoundo_clear(ed->rd);

   return ret;
}

Eina_Stringshare *
edit_cur_paragh_get(edit_data *ed)
{
   return parser_paragh_name_get(ed->pd, ed->en_edit);
}

Eina_Stringshare *
edit_cur_prog_name_get(edit_data *ed)
{
   return parser_cur_name_fast_get(ed->en_edit, "program");
}

Eina_Stringshare *
edit_cur_part_name_get(edit_data *ed)
{
   return parser_cur_name_fast_get(ed->en_edit, "part");
}

int
edit_max_line_get(edit_data *ed)
{
   return ed->line_max;
}

void
edit_goto(edit_data *ed, int line)
{
   elm_entry_select_none(ed->en_edit);
   Evas_Object *tb = elm_entry_textblock_get(ed->en_edit);
   Evas_Textblock_Cursor *cur = evas_object_textblock_cursor_get(tb);
   evas_textblock_cursor_line_set(cur, (line - 1));
   elm_entry_calc_force(ed->en_edit);
   elm_object_focus_set(ed->en_edit, EINA_TRUE);
}

Evas_Object *
edit_entry_get(edit_data *ed)
{
   return ed->en_edit;
}

void
edit_line_increase(edit_data *ed, int cnt)
{
   char buf[MAX_LINE_DIGIT_CNT];
   int i;

   for (i = 0; i < cnt; i++)
     {
        ed->line_max++;
        snprintf(buf, sizeof(buf), "<br/>%d", ed->line_max);
        elm_entry_entry_append(ed->en_line, buf);
     }
   elm_entry_calc_force(ed->en_line);

   Enventor_Max_Line max_line;
   max_line.line = ed->line_max;
   evas_object_smart_callback_call(ed->enventor, SIG_MAX_LINE_CHANGED,
                                   &max_line);
}

void
edit_line_decrease(edit_data *ed, int cnt)
{
   if (cnt < 1) return;

   Evas_Object *textblock = elm_entry_textblock_get(ed->en_line);
   Evas_Textblock_Cursor *cur1 = evas_object_textblock_cursor_new(textblock);
   Evas_Textblock_Cursor *cur2 = evas_object_textblock_cursor_new(textblock);

   int i = cnt;
   while (i)
     {
        evas_textblock_cursor_paragraph_last(cur1);
        evas_textblock_cursor_paragraph_prev(cur1);
        evas_textblock_cursor_paragraph_last(cur2);
        evas_textblock_cursor_range_delete(cur1, cur2);
        i--;
     }

   evas_textblock_cursor_free(cur1);
   evas_textblock_cursor_free(cur2);

   elm_entry_calc_force(ed->en_line);

   ed->line_max -= cnt;

   if (ed->line_max < 1) line_init(ed);

   Enventor_Max_Line max_line;
   max_line.line = ed->line_max;
   evas_object_smart_callback_call(ed->enventor, SIG_MAX_LINE_CHANGED,
                                   &max_line);
}

void
edit_redoundo_region_push(edit_data *ed, int cursor_pos1, int cursor_pos2)
{
   redoundo_entry_region_push(ed->rd, cursor_pos1, cursor_pos2);
}

void
edit_disabled_set(edit_data *ed, Eina_Bool disabled)
{
   elm_object_tree_focus_allow_set(ed->layout, !disabled);

   if (disabled)
     {
        elm_object_signal_emit(ed->layout, "elm,state,disabled", "");
        elm_entry_select_none(ed->en_edit);
     }
   else
     elm_object_signal_emit(ed->layout, "elm,state,enabled", "");

   //Turn off the part highlight in case of disable.
   if (disabled) view_part_highlight_set(VIEW_DATA, NULL);
   else if (ed->part_highlight) edit_view_sync(ed);
}

void
edit_auto_indent_set(edit_data *ed, Eina_Bool auto_indent)
{
   auto_indent = !!auto_indent;
   ed->auto_indent = auto_indent;
}

Eina_Bool
edit_auto_indent_get(edit_data *ed)
{
   return ed->auto_indent;
}

void
edit_error_set(edit_data *ed, int line, const char *target)
{
   ed->error_line = line;
   ed->error_target = target;
}

Eina_Bool
edit_ctxpopup_enabled_get(edit_data *ed)
{
   return ed->ctxpopup_enabled;
}

void
edit_ctxpopup_enabled_set(edit_data *ed, Eina_Bool enabled)
{
   enabled = !!enabled;
   ed->ctxpopup_enabled = enabled;
}

Eina_Bool
edit_ctxpopup_visible_get(edit_data *ed)
{
   return (ed->ctxpopup ? EINA_TRUE : EINA_FALSE);
}

void
edit_ctxpopup_dismiss(edit_data *ed)
{
   if (ed->ctxpopup) elm_ctxpopup_dismiss(ed->ctxpopup);
}

Eina_Bool
edit_redoundo(edit_data *ed, Eina_Bool undo)
{
   int lines;
   Eina_Bool changed;

   if (undo) lines = redoundo_undo(ed->rd, &changed);
   else lines = redoundo_redo(ed->rd, &changed);
   if (!changed) return EINA_FALSE;

   if (lines > 0) edit_line_increase(ed, lines);
   else edit_line_decrease(ed, abs(lines));

   edit_changed_set(ed, EINA_TRUE);
   syntax_color_full_update(ed, EINA_TRUE);

   return EINA_TRUE;
}
