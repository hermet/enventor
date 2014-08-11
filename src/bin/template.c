#include <Elementary.h>
#include "common.h"
#include "template_code.h"

typedef enum {
   TEMPLATE_PART_INSERT_DEFAULT,
   TEMPLATE_PART_INSERT_LIVE_EDIT
} Template_Part_Insert_Type;

const char *NAME_SEED = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const int NAME_SEED_LEN = 52;

static const char *
template_part_first_line_get(void)
{
   static char buf[40];
   char name[9];
   int i;

   for (i = 0; i < 8; i++)
     name[i] = NAME_SEED[(rand() % NAME_SEED_LEN)];
   name[8] = '\0';

   snprintf(buf, sizeof(buf), "part { name: \"%s\";<br/>", name);

   return (const char *) buf;
}

void
template_insert(edit_data *ed)
{
   Evas_Object *entry = edit_entry_get(ed);
   Eina_Stringshare *paragh = edit_cur_paragh_get(ed);
   if (!paragh) return;

   if (!strcmp(paragh, "parts"))
     {
        template_part_insert(ed, EDJE_PART_TYPE_IMAGE);
        goto end;
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
        stats_info_msg_update("Can't insert template code here. Move the cursor inside the \"Collections,Images,Parts,Part,Programs\" scope.");
        goto end;
     }

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
   edit_line_increase(ed, (line_cnt -1));
   elm_entry_entry_insert(entry, p);
   elm_entry_entry_insert(entry, t[i]);

   int cursor_pos2 = elm_entry_cursor_pos_get(entry);
   edit_redoundo_region_push(ed, cursor_pos1, cursor_pos2);

   elm_entry_cursor_pos_set(entry, cursor_pos);

   edit_syntax_color_partial_apply(ed, 0);
   snprintf(buf, sizeof(buf), "Template code inserted. (%s)", buf2);
   stats_info_msg_update(buf);

end:
   eina_stringshare_del(paragh);
}

static void
image_description_add(edit_data *ed)
{
   int cursor_pos;
   Evas_Object * edit_entry = edit_entry_get(ed);

   Evas_Coord cursor_pos_to_restore = elm_entry_cursor_pos_get(edit_entry_get(ed));

   Eina_Bool images_block = parser_images_pos_get(edit_entry, &cursor_pos);
   if (cursor_pos == -1) return;
   if (images_block)
      {
         elm_entry_cursor_pos_set(edit_entry, cursor_pos);
         template_insert(ed);
      }
   else
      {
         elm_entry_cursor_pos_set(edit_entry, cursor_pos);
         elm_entry_cursor_line_begin_set(edit_entry);
         int cursor_pos1 = elm_entry_cursor_pos_get(edit_entry);
         elm_entry_entry_insert(edit_entry, TEMPLATE_IMG_BLOCK);
         edit_line_increase(ed, TEMPLATE_IMG_BLOCK_LINE_CNT);
         int cursor_pos2 = elm_entry_cursor_pos_get(edit_entry);
         edit_redoundo_region_push(ed, cursor_pos1, cursor_pos2);
      }
   elm_entry_cursor_pos_set(edit_entry, cursor_pos_to_restore);
}


static int
template_part_insert_cursor_pos_set(edit_data *ed,
                                    Template_Part_Insert_Type insert_type,
                                    const Eina_Stringshare *group_name)
{
   int cursor_pos = -1;
   Evas_Object *edit_entry = edit_entry_get(ed);
   if (insert_type == TEMPLATE_PART_INSERT_LIVE_EDIT)
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
internal_template_part_insert(edit_data *ed,
                              Edje_Part_Type type,
                              Template_Part_Insert_Type insert_type,
                              const Eina_Stringshare *group_name)
{
   if (type == EDJE_PART_TYPE_NONE) return;
   if ((type == EDJE_PART_TYPE_IMAGE) &&
       (insert_type == TEMPLATE_PART_INSERT_LIVE_EDIT))
     image_description_add(ed);

   Evas_Object *edit_entry = edit_entry_get(ed);
   int cursor_pos = template_part_insert_cursor_pos_set(ed, insert_type,
                                                             group_name);
   if (cursor_pos == -1) return;
   int cursor_pos1 = elm_entry_cursor_pos_get(edit_entry);
   int space = edit_cur_indent_depth_get(ed);

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

   elm_entry_entry_insert(edit_entry, p);
   const char *first_line = template_part_first_line_get();
   elm_entry_entry_insert(edit_entry, first_line);
   edit_line_increase(ed, 1);

   int i;
   for (i = 0; i < (line_cnt - 1); i++)
     {
        elm_entry_entry_insert(edit_entry, p);
        elm_entry_entry_insert(edit_entry, t[i]);
        //Incease line by (line count - 1)
        edit_line_increase(ed, 1);
     }

   elm_entry_entry_insert(edit_entry, t[i]);

   int cursor_pos2 = elm_entry_cursor_pos_get(edit_entry);
   edit_redoundo_region_push(ed, cursor_pos1, cursor_pos2);

   elm_entry_cursor_pos_set(edit_entry, cursor_pos);

   snprintf(buf, sizeof(buf), "Template code inserted. (%s Part)", part);
   stats_info_msg_update(buf);
   edit_changed_set(ed, EINA_TRUE);
   edit_save(ed);
}

void
template_live_edit_part_insert(edit_data *ed,
                               Edje_Part_Type type,
                               const Eina_Stringshare *group_name)
{
   internal_template_part_insert(ed, type, TEMPLATE_PART_INSERT_LIVE_EDIT,
                                 group_name);
}

void
template_part_insert(edit_data *ed,
                     Edje_Part_Type type)
{
   internal_template_part_insert(ed, type,
                                 TEMPLATE_PART_INSERT_DEFAULT, NULL);
}
