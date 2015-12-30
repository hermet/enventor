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
template_random_string_create(char *buf, int size)
{
   int i;
   for (i = 0; i < (size - 1); i++)
     buf[i] = NAME_SEED[(rand() % NAME_SEED_LEN)];
   buf[i]='\0';
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
        template_insert(ed, ENVENTOR_TEMPLATE_INSERT_LIVE_EDIT, NULL, 0);
        cursor_pos2 = elm_entry_cursor_pos_get(edit_entry);
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

   if (!styles_block)
     elm_entry_entry_insert(edit_entry, TEMPLATE_TEXTBLOCK_STYLE_BLOCK[0]);

   int buf_len = strlen(TEMPLATE_TEXTBLOCK_STYLE_BLOCK[1]) + strlen(style_name);
   char *buf = malloc(buf_len);
   snprintf(buf, buf_len, TEMPLATE_TEXTBLOCK_STYLE_BLOCK[1], style_name);
   elm_entry_entry_insert(edit_entry, buf);
   free(buf);

   if (!styles_block)
     elm_entry_entry_insert(edit_entry, TEMPLATE_TEXTBLOCK_STYLE_BLOCK[2]);

   int line_inc = TEMPLATE_TEXTBLOCK_STYLE_LINE_CNT;
   if (!styles_block) line_inc += 2;
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

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

Eina_Bool
template_part_insert(edit_data *ed, Edje_Part_Type part_type,
                     Enventor_Template_Insert_Type insert_type, float rel1_x,
                     float rel1_y, float rel2_x, float rel2_y,
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
   char random_name[9];
   template_random_string_create(random_name, 9);

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
     }

   //Insert relatives
   elm_entry_entry_insert(edit_entry, p);
   snprintf(buf, sizeof(buf), "      rel1.relative: %.2f %.2f;<br/>", rel1_x,
            rel1_y);
   elm_entry_entry_insert(edit_entry, buf);
   elm_entry_entry_insert(edit_entry, p);
   snprintf(buf, sizeof(buf), "      rel2.relative: %.2f %.2f;<br/>", rel2_x,
            rel2_y);
   elm_entry_entry_insert(edit_entry, buf);

   //Insert the tale of the part that contains closing brackets
   t = (char **) &TEMPLATE_PART_TALE;
   for (i = 0; i < TEMPLATE_PART_TALE_LINE_CNT; i++)
     {
        elm_entry_entry_insert(edit_entry, p);
        elm_entry_entry_insert(edit_entry, t[i]);
     }

   //add new line only in live edit mode
   if (insert_type == ENVENTOR_TEMPLATE_INSERT_LIVE_EDIT)
     elm_entry_entry_insert(edit_entry, "<br/>");

   /* Increase (part name + body + relatives + tail) line. But line increase
      count should be -1 in entry template insertion because the
      cursor position would be taken one line additionally. */
   int line_inc = 1 + line_cnt + 2 + TEMPLATE_PART_TALE_LINE_CNT;
   if (insert_type == ENVENTOR_TEMPLATE_INSERT_DEFAULT) line_inc--;
   edit_line_increase(ed, line_inc);

   int cursor_pos2 = elm_entry_cursor_pos_get(edit_entry);
   edit_redoundo_region_push(ed, cursor_pos1, cursor_pos2);

   elm_entry_cursor_pos_set(edit_entry, cursor_pos);

   if (part_type == EDJE_PART_TYPE_IMAGE)
     image_description_add(ed);
   else if (part_type == EDJE_PART_TYPE_TEXTBLOCK)
     textblock_style_add(ed, random_name);

   edit_syntax_color_partial_apply(ed, 0);
   edit_changed_set(ed, EINA_TRUE);

   strncpy(syntax, type_name, n);

   return EINA_TRUE;
}

Eina_Bool
template_insert(edit_data *ed, Enventor_Template_Insert_Type insert_type,
                char *syntax, size_t n)
{
   Evas_Object *entry = edit_entry_get(ed);
   Eina_Stringshare *paragh = edit_cur_paragh_get(ed);
   Eina_Bool ret = EINA_FALSE;
   if (!paragh) return EINA_FALSE;

   if (!strcmp(paragh, "parts"))
     {
        ret = template_part_insert(ed, EDJE_PART_TYPE_IMAGE,
                                   ENVENTOR_TEMPLATE_INSERT_DEFAULT,
                                   REL1_X, REL1_Y, REL2_X, REL2_Y, NULL, syntax,
                                   n);
        goto end;
     }

   int line_cnt;
   char **t = NULL;

   if (!strcmp(paragh, "part"))
     {
        line_cnt = TEMPLATE_DESC_LINE_CNT;
        t = (char **) &TEMPLATE_DESC;
        strncpy(syntax, "Description", n);
     }
   else if (!strcmp(paragh, "programs"))
     {
        line_cnt = TEMPLATE_PROG_LINE_CNT;
        t = (char **) &TEMPLATE_PROG;
        strncpy(syntax, "Program", n);
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
     }

   if (!t) goto end;

   int cursor_pos = elm_entry_cursor_pos_get(entry);
   elm_entry_cursor_line_begin_set(entry);
   int cursor_pos1 = elm_entry_cursor_pos_get(entry);
   int space = edit_cur_indent_depth_get(ed);

   //Alloc Empty spaces
   char *p = alloca(space + 1);
   memset(p, ' ', space);
   p[space] = '\0';

   int i;
   for (i = 0; i < (line_cnt - 1); i++)
     {
        elm_entry_entry_insert(entry, p);
        elm_entry_entry_insert(entry, t[i]);
     }
   elm_entry_entry_insert(entry, p);
   elm_entry_entry_insert(entry, t[i]);

   /* Line increase count should be -1 in entry template insertion because the
      cursor position would be taken one line additionally. */
   if (insert_type == ENVENTOR_TEMPLATE_INSERT_DEFAULT) line_cnt--;
   edit_line_increase(ed, line_cnt);

   int cursor_pos2 = elm_entry_cursor_pos_get(entry);
   edit_redoundo_region_push(ed, cursor_pos1, cursor_pos2);

   if (!strcmp(paragh, "images"))
     cursor_pos += (cursor_pos2 - cursor_pos1);

   elm_entry_cursor_pos_set(entry, cursor_pos);

   edit_syntax_color_partial_apply(ed, 0);

   ret = EINA_TRUE;

end:
   eina_stringshare_del(paragh);

   return ret;
}
