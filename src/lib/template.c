#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include "enventor_private.h"
#include "template_code.h"

const char *NAME_SEED = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const int NAME_SEED_LEN = 52;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
template_random_string_create(char *paragh, char *buf, int size)
{
   int i, paragh_len = 0;

   if (paragh)
     paragh_len = strlen(paragh);

   if (paragh_len > 0)
     {
        memcpy(buf, paragh, paragh_len);
        buf[paragh_len++] = '_';
     }

   for (i = 0; i < size; i++)
     buf[paragh_len + i] = NAME_SEED[(rand() % NAME_SEED_LEN)];
   buf[paragh_len + i] = '\0';
}

static void
image_description_add(edit_data *ed)
{
   int cursor_pos;
   Evas_Object * edit_entry = edit_entry_get(ed);

   Evas_Coord cursor_pos_to_restore = elm_entry_cursor_pos_get(edit_entry_get(ed));

   Eina_Bool images_block = parser_images_pos_get(edit_entry, &cursor_pos);
   if (cursor_pos == -1) return;

   elm_entry_cursor_pos_set(edit_entry, cursor_pos);
   elm_entry_cursor_line_begin_set(edit_entry);
   int cursor_pos1 = elm_entry_cursor_pos_get(edit_entry);
   int cursor_pos2;
   if (images_block)
     {
        // the first line of TEMPLATE_IMG to check it is already exist
        const char *template_image_str = TEMPLATE_IMG[0];
        if (parser_is_image_name(edit_entry, template_image_str))
          cursor_pos2 = cursor_pos1;
        else
          {
             template_insert(ed, ENVENTOR_TEMPLATE_INSERT_LIVE_EDIT, NULL, 0);
             cursor_pos2 = elm_entry_cursor_pos_get(edit_entry);
          }
     }
   else
     {
        int space = edit_cur_indent_depth_get(ed);

        //Alloc Empty spaces
        char *p = alloca(space + 1);
        memset(p, ' ', space);
        p[space] = '\0';

        int i;
        for (i = 0; i < (TEMPLATE_IMG_BLOCK_LINE_CNT - 1); i++)
          {
             elm_entry_entry_insert(edit_entry, p);
             elm_entry_entry_insert(edit_entry, TEMPLATE_IMG_BLOCK[i]);
          }
        elm_entry_entry_insert(edit_entry, p);
        elm_entry_entry_insert(edit_entry, TEMPLATE_IMG_BLOCK[i]);

        edit_line_increase(ed, TEMPLATE_IMG_BLOCK_LINE_CNT);
        cursor_pos2 = elm_entry_cursor_pos_get(edit_entry);
        edit_redoundo_region_push(ed, cursor_pos1, cursor_pos2);
     }
   cursor_pos_to_restore += (cursor_pos2 - cursor_pos1);
   elm_entry_cursor_pos_set(edit_entry, cursor_pos_to_restore);
}

static void
textblock_style_add(edit_data *ed, const char *style_name)
{
   int cursor_pos;
   Evas_Object * edit_entry = edit_entry_get(ed);
   Eina_Bool styles_block = parser_styles_pos_get(edit_entry, &cursor_pos);
   if (cursor_pos == -1) return;
   int cursor_pos_to_restore = elm_entry_cursor_pos_get(edit_entry_get(ed));

   elm_entry_cursor_pos_set(edit_entry, cursor_pos);
   elm_entry_cursor_line_begin_set(edit_entry);
   int cursor_pos1 = elm_entry_cursor_pos_get(edit_entry);

   int space = edit_cur_indent_depth_get(ed);
   if (styles_block)
      space -= TAB_SPACE;

   //Alloc Empty spaces
   char *p = alloca(space + 1);
   memset(p, ' ', space);
   p[space] = '\0';

   if (!styles_block)
     {
        elm_entry_entry_insert(edit_entry, p);
        elm_entry_entry_insert(edit_entry, TEMPLATE_TEXTBLOCK_STYLE_BLOCK[0]);
     }

   int buf_len = strlen(TEMPLATE_TEXTBLOCK_STYLE_BLOCK[1]) + strlen(style_name);
   char *buf = malloc(buf_len);
   snprintf(buf, buf_len, TEMPLATE_TEXTBLOCK_STYLE_BLOCK[1], style_name);
   elm_entry_entry_insert(edit_entry, p);
   elm_entry_entry_insert(edit_entry, buf);
   elm_entry_entry_insert(edit_entry, p);
   elm_entry_entry_insert(edit_entry, TEMPLATE_TEXTBLOCK_STYLE_BLOCK[2]);
   elm_entry_entry_insert(edit_entry, p);
   elm_entry_entry_insert(edit_entry, TEMPLATE_TEXTBLOCK_STYLE_BLOCK[3]);
   free(buf);

   if (!styles_block)
     {
       elm_entry_entry_insert(edit_entry, p);
       elm_entry_entry_insert(edit_entry, TEMPLATE_TEXTBLOCK_STYLE_BLOCK[4]);
     }

   int line_inc = TEMPLATE_TEXTBLOCK_STYLE_LINE_CNT;
   if (styles_block) line_inc -= 2;
   edit_line_increase(ed, line_inc);

   int cursor_pos2 = elm_entry_cursor_pos_get(edit_entry);
   edit_redoundo_region_push(ed, cursor_pos1, cursor_pos2);

   cursor_pos_to_restore += (cursor_pos2 - cursor_pos1);
   elm_entry_cursor_pos_set(edit_entry, cursor_pos_to_restore);
}

static int
template_part_insert_cursor_pos_set(edit_data *ed,
                                    Enventor_Template_Insert_Type insert_type,
                                    const Eina_Stringshare *group_name)
{
   int cursor_pos = -1;
   Evas_Object *edit_entry = edit_entry_get(ed);
   if (insert_type == ENVENTOR_TEMPLATE_INSERT_LIVE_EDIT)
     {
        cursor_pos = parser_end_of_parts_block_pos_get(edit_entry, group_name);
        if (cursor_pos != -1)
          elm_entry_cursor_pos_set(edit_entry, cursor_pos);
     }
   else
     {
        cursor_pos = elm_entry_cursor_pos_get(edit_entry);
     }
   elm_entry_cursor_line_begin_set(edit_entry);
   return cursor_pos;
}

static void
select_random_name(Evas_Object *entry, const char* first_line,
                   const char* random_name, int space)
{
   char *matched = strstr(first_line, random_name);
   if (matched)
     {
        int random_name_pos = matched - first_line;
        random_name_pos += space;
        elm_entry_cursor_line_begin_set(entry);
        int line_start = elm_entry_cursor_pos_get(entry);
        int start = line_start + random_name_pos;
        int end = start + strlen(random_name);
        elm_entry_select_region_set(entry, start, end);
     }
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

Eina_Bool
template_part_insert(edit_data *ed, Edje_Part_Type part_type,
                     Enventor_Template_Insert_Type insert_type,
                     Eina_Bool fixed_w, Eina_Bool fixed_h,
                     char *rel1_x_to, char *rel1_y_to,
                     char *rel2_x_to, char *rel2_y_to,
                     float align_x, float align_y, int min_w, int min_h,
                     float rel1_x, float rel1_y, float rel2_x, float rel2_y,
                     const Eina_Stringshare *group_name, char *syntax, size_t n)
{
   Evas_Object *edit_entry = edit_entry_get(ed);

   if (insert_type == ENVENTOR_TEMPLATE_INSERT_LIVE_EDIT)
     group_name = view_group_name_get(VIEW_DATA);

   int cursor_pos = template_part_insert_cursor_pos_set(ed, insert_type,
                                                        group_name);
   if (cursor_pos == -1) return EINA_FALSE;

   int cursor_pos1 = elm_entry_cursor_pos_get(edit_entry);
   int space = edit_cur_indent_depth_get(ed);

   //Alloc Empty spaces
   char *p = alloca(space + 1);
   memset(p, ' ', space);
   p[space] = '\0';

   int line_cnt = 0;
   char **t = NULL;
   char buf[64];
   char type_name[20];

   switch(part_type)
     {
        case EDJE_PART_TYPE_RECTANGLE:
           line_cnt = TEMPLATE_PART_RECT_LINE_CNT;
           t = (char **) &TEMPLATE_PART_RECT;
           strncpy(type_name, "rect\0", 5);
           break;
        case EDJE_PART_TYPE_TEXT:
           line_cnt = TEMPLATE_PART_TEXT_LINE_CNT;
           t = (char **) &TEMPLATE_PART_TEXT;
           strncpy(type_name, "text\0", 5);
           break;
        case EDJE_PART_TYPE_SWALLOW:
           line_cnt = TEMPLATE_PART_SWALLOW_LINE_CNT;
           t = (char **) &TEMPLATE_PART_SWALLOW;
           strncpy(type_name, "swallow\0", 8);
           break;
        case EDJE_PART_TYPE_TEXTBLOCK:
           line_cnt = TEMPLATE_PART_TEXTBLOCK_LINE_CNT;
           t = (char **) &TEMPLATE_PART_TEXTBLOCK;
           strncpy(type_name, "textblock\0", 10);
           break;
        case EDJE_PART_TYPE_SPACER:
           line_cnt = TEMPLATE_PART_SPACER_LINE_CNT;
           t = (char **) &TEMPLATE_PART_SPACER;
           strncpy(type_name, "spacer\0", 7);
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
           strncpy(type_name, "image\0", 6);
           break;
        //for avoiding compiler warning.
        case EDJE_PART_TYPE_MESH_NODE:
        case EDJE_PART_TYPE_LIGHT:
        case EDJE_PART_TYPE_CAMERA:
        case EDJE_PART_TYPE_SNAPSHOT:
           break;
     }

   //Insert first line of the part block with generated name.
   char first_line[40];
   char random_name[15];
   template_random_string_create(type_name, random_name, 4);

   elm_entry_entry_insert(edit_entry, p);
   snprintf(first_line, 40, "%s { \"%s\";<br/>", type_name, random_name);
   elm_entry_entry_insert(edit_entry, first_line);

   //Insert part body
   int i;
   for (i = 0; i < line_cnt; i++)
     {
        elm_entry_entry_insert(edit_entry, p);
        elm_entry_entry_insert(edit_entry, t[i]);
     }

   if (part_type == EDJE_PART_TYPE_TEXTBLOCK)
     {
        elm_entry_entry_insert(edit_entry, p);
        snprintf(buf, sizeof(buf), "      text.style: \"%s\";<br/>",
                 random_name);
        elm_entry_entry_insert(edit_entry, buf);
        line_cnt++;
     }

   //Apply align values
   elm_entry_entry_insert(edit_entry, p);
   snprintf(buf, sizeof(buf), "      align: %.1f %.1f;<br/>", align_x, align_y);
   elm_entry_entry_insert(edit_entry, buf);
   line_cnt++;

   //Width is fixed or Height is fixed
   if (fixed_w || fixed_h)
     {
        elm_entry_entry_insert(edit_entry, p);
        snprintf(buf, sizeof(buf), "      fixed: %d %d;<br/>", fixed_w, fixed_h);
        elm_entry_entry_insert(edit_entry, buf);
        elm_entry_entry_insert(edit_entry, p);
        snprintf(buf, sizeof(buf), "      min: %d %d;<br/>", min_w, min_h);
        elm_entry_entry_insert(edit_entry, buf);
        line_cnt += 2;
     }

   //If there are some relative_to part then insert relative_to 
   if (rel1_x_to)
     {
        elm_entry_entry_insert(edit_entry, p);
        snprintf(buf, sizeof(buf), "      rel1.to_x: \"%s\";<br/>", rel1_x_to);
        elm_entry_entry_insert(edit_entry, buf);
        line_cnt++;
     }
   if (rel1_y_to)
     {
        elm_entry_entry_insert(edit_entry, p);
        snprintf(buf, sizeof(buf), "      rel1.to_y: \"%s\";<br/>", rel1_y_to);
        elm_entry_entry_insert(edit_entry, buf);
        line_cnt++;
     }
   if (rel2_x_to)
     {
        elm_entry_entry_insert(edit_entry, p);
        snprintf(buf, sizeof(buf), "      rel2.to_x: \"%s\";<br/>", rel2_x_to);
        elm_entry_entry_insert(edit_entry, buf);
        line_cnt++;
     }
   if (rel2_y_to)
     {
        elm_entry_entry_insert(edit_entry, p);
        snprintf(buf, sizeof(buf), "      rel2.to_y: \"%s\";<br/>", rel2_y_to);
        elm_entry_entry_insert(edit_entry, buf);
        line_cnt++;
     }

   //Insert relatives
   elm_entry_entry_insert(edit_entry, p);

   //These conditions check whether the relative number is 4 places of decimals or not
   //Condition 1: relative values are 4 places of decimals
   if ((int)(rel1_x * 10000 + 0.5) % 100 ||
       (int)(rel1_y * 10000 + 0.5) % 100 ||
       (int)(rel2_x * 10000 + 0.5) % 100 ||
       (int)(rel2_y * 10000 + 0.5) % 100)
     {

        snprintf(buf, sizeof(buf), "      rel1.relative: %.4f %.4f;<br/>",
                 rel1_x, rel1_y);
        elm_entry_entry_insert(edit_entry, buf);
        elm_entry_entry_insert(edit_entry, p);
        snprintf(buf, sizeof(buf), "      rel2.relative: %.4f %.4f;<br/>",
                 rel2_x, rel2_y);
     }
   //Condition 2: relative values are 2 places of decimals
   else
     {
        snprintf(buf, sizeof(buf), "      rel1.relative: %.2f %.2f;<br/>",
                 rel1_x, rel1_y);
        elm_entry_entry_insert(edit_entry, buf);
        elm_entry_entry_insert(edit_entry, p);
        snprintf(buf, sizeof(buf), "      rel2.relative: %.2f %.2f;<br/>",
                 rel2_x, rel2_y);
     }

   elm_entry_entry_insert(edit_entry, buf);

   //Insert the tale of the part that contains closing brackets
   t = (char **) &TEMPLATE_PART_TALE;
   for (i = 0; i < TEMPLATE_PART_TALE_LINE_CNT; i++)
     {
        elm_entry_entry_insert(edit_entry, p);
        elm_entry_entry_insert(edit_entry, t[i]);
     }

   //Add a new line in the end of inserted template
   elm_entry_entry_insert(edit_entry, "<br/>");

   //Increase (part name + body + relatives + tail) line
   int line_inc = 1 + line_cnt + 2 + TEMPLATE_PART_TALE_LINE_CNT;
   edit_line_increase(ed, line_inc);

   int cursor_pos2 = elm_entry_cursor_pos_get(edit_entry);
   edit_redoundo_region_push(ed, cursor_pos1, cursor_pos2);

   elm_entry_cursor_pos_set(edit_entry, cursor_pos);

   if (part_type == EDJE_PART_TYPE_IMAGE)
     image_description_add(ed);
   else if (part_type == EDJE_PART_TYPE_TEXTBLOCK)
     textblock_style_add(ed, random_name);

   //select random name
   select_random_name(edit_entry, first_line, random_name, space);

   edit_syntax_color_partial_apply(ed, 0);
   edit_changed_set(ed, EINA_TRUE);

   strncpy(syntax, type_name, n);

   return EINA_TRUE;
}

Eina_Bool
template_insert(edit_data *ed,
                Enventor_Template_Insert_Type insert_type,
                char *syntax, size_t n)
{
   Evas_Object *entry = edit_entry_get(ed);
   Eina_Stringshare *paragh = edit_cur_paragh_get(ed);

   Eina_Bool ret = EINA_FALSE;
   if (!paragh) return EINA_FALSE;

   //Move cursor position to the beginning of the next line to apply indentation
   elm_entry_cursor_line_end_set(entry);
   int cursor_line_end_pos = elm_entry_cursor_pos_get(entry);
   elm_entry_cursor_pos_set(entry, cursor_line_end_pos + 1);

   if (!strcmp(paragh, "parts"))
     {
        ret = template_part_insert(ed, EDJE_PART_TYPE_IMAGE,
                                   ENVENTOR_TEMPLATE_INSERT_DEFAULT,
                                   EINA_FALSE, EINA_FALSE,
                                   NULL, NULL, NULL, NULL,
                                   0.5, 0.5, 0, 0,
                                   REL1_X, REL1_Y, REL2_X, REL2_Y, NULL, syntax,
                                   n);
        goto end;
     }

   int line_cnt;
   char **t = NULL;
   char first_line[40];
   char random_name[9];
   int space = edit_cur_indent_depth_get(ed);

   //Alloc Empty spaces
   char *p = alloca(space + 1);
   memset(p, ' ', space);
   p[space] = '\0';

   template_random_string_create(NULL, random_name, 8);
   elm_entry_cursor_line_begin_set(entry);

   if (!strcmp(paragh, "part") || !strcmp(paragh, "image") ||
       !strcmp(paragh, "rect") || !strcmp(paragh, "swallow") ||
       !strcmp(paragh, "text") || !strcmp(paragh, "textblock"))
     {
        line_cnt = TEMPLATE_DESC_LINE_CNT;
        t = (char **) &TEMPLATE_DESC;
        strncpy(syntax, "Description", n);
        snprintf(first_line, 40, "desc { \"%s\";<br/>", random_name);
     }
   else if (!strcmp(paragh, "spacer"))
     {
        line_cnt = TEMPLATE_DESC_SPACER_LINE_CNT;
        t = (char **) &TEMPLATE_DESC_SPACER;
        strncpy(syntax, "Description", n);
        snprintf(first_line, 40, "desc { \"%s\";<br/>", random_name);
     }
   else if (!strcmp(paragh, "programs"))
     {
        line_cnt = TEMPLATE_PROG_LINE_CNT;
        t = (char **) &TEMPLATE_PROG;
        strncpy(syntax, "Program", n);
        snprintf(first_line, 40, "program { \"%s\";<br/>", random_name);
     }
   else if (!strcmp(paragh, "images"))
     {
        line_cnt = TEMPLATE_IMG_LINE_CNT;
        t = (char **) &TEMPLATE_IMG;
        strncpy(syntax, "Image File", n);
     }
   else if (!strcmp(paragh, "collections"))
     {
        line_cnt = TEMPLATE_GROUP_LINE_CNT;
        t = (char **) &TEMPLATE_GROUP;
        strncpy(syntax, "Group", n);
        snprintf(first_line, 40, "group { \"%s\";<br/>", random_name);
     }

   if (!t) goto end;

   int cursor_pos = elm_entry_cursor_pos_get(entry);
   int cursor_pos1 = elm_entry_cursor_pos_get(entry);

   if (strcmp(paragh, "images"))
     {
        elm_entry_entry_insert(entry, p);
        elm_entry_entry_insert(entry, first_line);
     }

   int i;
   for (i = 0; i < (line_cnt - 1); i++)
     {
        elm_entry_entry_insert(entry, p);
        elm_entry_entry_insert(entry, t[i]);
     }
   elm_entry_entry_insert(entry, p);
   elm_entry_entry_insert(entry, t[i]);

   //Add a new line in the end of inserted template
   elm_entry_entry_insert(entry, "<br/>");

   //Increase (template + last new line) line
   edit_line_increase(ed, line_cnt + 1);

   int cursor_pos2 = elm_entry_cursor_pos_get(entry);
   edit_redoundo_region_push(ed, cursor_pos1, cursor_pos2);

   if (!strcmp(paragh, "images"))
     cursor_pos += (cursor_pos2 - cursor_pos1);

   elm_entry_cursor_pos_set(entry, cursor_pos);

   //select random name
   select_random_name(entry, first_line, random_name, space);

   edit_syntax_color_partial_apply(ed, 0);
   edit_changed_set(ed, EINA_TRUE);

   ret = EINA_TRUE;

end:
   eina_stringshare_del(paragh);

   return ret;
}
