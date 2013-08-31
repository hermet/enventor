#include <Elementary.h>
#include "common.h"

struct parser_s
{
   Eina_Inlist *attrs;
   Ecore_Thread *thread;
};

typedef struct parser_attr_s
{
   EINA_INLIST;
   Eina_Stringshare *keyword;
   attr_value value;
} parser_attr;

typedef struct part_name_thread_data_s
{
   parser_data *pd;
   char *utf8;
   int cur_pos;
   const char *part_name;
   void (*cb)(void *data, Eina_Stringshare *part_name);
   void *cb_data;
} part_name_td;

static void
parser_type_init(parser_data *pd)
{
   parser_attr *attr;

   //FIXME: construct from the configuration file.
   Eina_List *types = NULL;
   types = eina_list_append(types, eina_stringshare_add("RECT"));
   types = eina_list_append(types, eina_stringshare_add("TEXT"));
   types = eina_list_append(types, eina_stringshare_add("IMAGE"));
   types = eina_list_append(types, eina_stringshare_add("SWALLOW"));
   types = eina_list_append(types, eina_stringshare_add("TEXTBLOCK"));
   types = eina_list_append(types, eina_stringshare_add("GRADIENT"));
   types = eina_list_append(types, eina_stringshare_add("GROUP"));
   types = eina_list_append(types, eina_stringshare_add("BOX"));
   types = eina_list_append(types, eina_stringshare_add("TABLE"));
   types = eina_list_append(types, eina_stringshare_add("EXTERNAL"));
   types = eina_list_append(types, eina_stringshare_add("PROXY"));
   types = eina_list_append(types, eina_stringshare_add("SPACER"));

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("type");
   attr->value.strs = types;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   Eina_List *comps = NULL;
   comps = eina_list_append(comps, eina_stringshare_add("RAW"));
   comps = eina_list_append(comps, eina_stringshare_add("USER"));
   comps = eina_list_append(comps, eina_stringshare_add("COMP"));
   comps = eina_list_append(comps, eina_stringshare_add("LOSSY"));

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("image");
   attr->value.strs = comps;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   Eina_List *trans = NULL;
   trans = eina_list_append(trans, eina_stringshare_add("LINEAR"));
   trans = eina_list_append(trans, eina_stringshare_add("ACCELERATE"));
   trans = eina_list_append(trans, eina_stringshare_add("DECELERATE"));
   trans = eina_list_append(trans, eina_stringshare_add("SINUSOIDAL"));
   trans = eina_list_append(trans, eina_stringshare_add("ACCELERATE_FACTOR"));
   trans = eina_list_append(trans, eina_stringshare_add("DECELERATE_FACTOR"));
   trans = eina_list_append(trans, eina_stringshare_add("SINUSOIDAL_FACTOR"));
   trans = eina_list_append(trans, eina_stringshare_add("DIVISOR_INTERP"));
   trans = eina_list_append(trans, eina_stringshare_add("BOUNCE"));
   trans = eina_list_append(trans, eina_stringshare_add("SPRING"));

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("transition");
   attr->value.strs = trans;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   Eina_List *aspect = NULL;
   aspect = eina_list_append(aspect, eina_stringshare_add("NONE"));
   aspect = eina_list_append(aspect, eina_stringshare_add("VERTICAL"));
   aspect = eina_list_append(aspect, eina_stringshare_add("HORIZONTAL"));
   aspect = eina_list_append(aspect, eina_stringshare_add("BOTH"));

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("aspect_preference");
   attr->value.strs = aspect;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("color");
   attr->value.min = 0;
   attr->value.max = 255;
   attr->value.integer = EINA_TRUE;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("relative");
   attr->value.min = 0.0;
   attr->value.max = 1;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("scale");
   attr->value.min = 0;
   attr->value.max = 1;
   attr->value.integer = EINA_TRUE;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("fixed");
   attr->value.min = 0;
   attr->value.max = 1;
   attr->value.integer = EINA_TRUE;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("aspect");
   attr->value.min = 0.0;
   attr->value.max = 1.0;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("align");
   attr->value.min = 0.0;
   attr->value.max = 1.0;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("size");
   attr->value.min = 1;
   attr->value.max = 255;
   attr->value.integer = EINA_TRUE;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("min");
   attr->value.min = 0;
   attr->value.max = 1000;
   attr->value.integer = EINA_TRUE;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("max");
   attr->value.min = 0;
   attr->value.max = 1000;
   attr->value.integer = EINA_TRUE;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("mouse_events");
   attr->value.min = 0;
   attr->value.max = 1000;
   attr->value.integer = EINA_TRUE;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);
}

attr_value *
parser_attribute_get(parser_data *pd, const char *text, const char *cur)
{
   if (!text || !cur) return NULL;

   char *p = (char *) cur;

   parser_attr *attr;
   Eina_Bool instring = EINA_FALSE;
   Eina_Bool necessary = EINA_FALSE;

   while (p >= text)
     {
        if (*p == ':')
          {
             necessary = EINA_TRUE;
             break;
          }
        if (*p == '\"') instring = !instring;
        p--;
     }
   if (!p || instring || !necessary) return NULL;

   while (p > text)
     {
        if ((*p == ';') || (*p == '.') || (*p == ' ')) break;
        p--;
     }

   if (!p) return NULL;
   if (p != text) p++;

   EINA_INLIST_FOREACH(pd->attrs, attr)
     {
        if (strstr(p, attr->keyword))
          return &attr->value;
     }

   return NULL;
}

parser_data *
parser_init()
{
   parser_data *pd = calloc(1, sizeof(parser_data));
   parser_type_init(pd);
   return pd;
}

void
parser_term(parser_data *pd)
{
   if (pd->thread) ecore_thread_cancel(pd->thread);

   Eina_List *l;
   parser_attr *attr;
   Eina_Stringshare *str;

   while(pd->attrs)
     {
        attr = EINA_INLIST_CONTAINER_GET(pd->attrs, parser_attr);
        pd->attrs = eina_inlist_remove(pd->attrs, pd->attrs);

        eina_stringshare_del(attr->keyword);

        EINA_LIST_FOREACH(attr->value.strs, l, str)
           eina_stringshare_del(str);
        eina_list_free(attr->value.strs);
        free(attr);
     }

   free(pd);
}

const char *
parser_markup_escape(parser_data *pd EINA_UNUSED, const char *str)
{
   const char *escaped = strchr(str, '>');
   if (!escaped) return str;
   if ((escaped + 1) >= (str + strlen(str))) return str;
   escaped++;

   return escaped;
}

static void
part_name_thread_blocking(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   part_name_td *td = data;

   char *utf8 = td->utf8;
   int cur_pos = td->cur_pos;

   char *p = utf8;
   char *end = utf8 + cur_pos;

   const char *quot = "\"";
   int quot_len = 1; // strlen("&quot;");
   const char *part = "part";
   int part_len = 4; //strlen("part");
   const char *parts = "parts";
   int parts_len = 5; //strlen("parts");

   int bracket = 0;
   const char *part_name = NULL;
   int part_name_len = 0;
   Eina_Bool part_in = EINA_FALSE;

   while (p <= end)
     {
        //Skip "" range
        if (*p == *quot)
          {
             p += quot_len;
             p = strstr(p, quot);
             if (!p) goto end;
             p += quot_len;
          }

        if (part_in && (*p == '{'))
          {
             bracket++;
             p++;
             continue;
          }

        if (part_in && (*p == '}') && (p < end))
          {
             bracket--;
             p++;
             if (bracket == 0) part_in = EINA_FALSE;
             continue;
          }
        if (strncmp(p, parts, parts_len))
          {
             if (!strncmp(p, part, part_len))
               {
                  p += part_len;
                  char *name_begin = strstr(p, quot);
                  if (!name_begin) goto end;
                  name_begin += quot_len;
                  p = name_begin;
                  char *name_end = strstr(p, quot);
                  if (!name_end) goto end;
                  part_name = name_begin;
                  part_name_len = name_end - name_begin;
                  p = name_end + quot_len;
                  bracket = 1;
                  part_in = EINA_TRUE;
                  continue;
               }
          }
        p++;
     }
   if (part_name)
     {
        if (bracket == 0)
          {
             part_name = NULL;
             goto end;
          }
        else
          part_name = eina_stringshare_add_length(part_name, part_name_len);
     }

end:
   if (utf8)
     {
        free(utf8);
        td->utf8 = NULL;
     }
   td->part_name = part_name;
}

static void
part_name_thread_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   part_name_td *td = data;
   Eina_Stringshare *part_name = td->part_name;
   td->cb(td->cb_data, part_name);
   td->pd->thread = NULL;
   free(td);
}

static void
part_name_thread_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   part_name_td *td = data;
   td->pd->thread = NULL;
   if (td->utf8) free(td->utf8);
   free(td);
}

void
parser_current_paragh_name_get(parser_data *pd, Evas_Object *entry)
{
   const char *PARTS = "parts";
   const char *PART = "part";
   const char *DESC = "description";
   const char *PROGS = "programs";

   Evas_Object *tb = elm_entry_textblock_get(entry);
   char *text = (char *) evas_object_textblock_text_markup_get(tb);
   if (!text) return;

   char *utf8 = elm_entry_markup_to_utf8(text);
   if (!utf8) return;

   int cur_pos = elm_entry_cursor_pos_get(entry);

   char *p = utf8;
   char *end = utf8 + cur_pos;

   while (p <= end)
     {


     }
}

void
parser_part_name_get(parser_data *pd, Evas_Object *entry, void (*cb)(void *data, Eina_Stringshare *part_name), void *data)
{
   if (pd->thread) ecore_thread_cancel(pd->thread);

   part_name_td *td = calloc(1, sizeof(part_name_td));
   if (!td) return;

   Evas_Object *tb = elm_entry_textblock_get(entry);
   char *text = (char *) evas_object_textblock_text_markup_get(tb);
   if (!text) return;

   char *utf8 = elm_entry_markup_to_utf8(text);
   if (!utf8) return;

   td->pd = pd;
   td->utf8 = utf8;
   td->cur_pos = elm_entry_cursor_pos_get(entry);
   td->cb = cb;
   td->cb_data = data;

   pd->thread = ecore_thread_run(part_name_thread_blocking,
                                 part_name_thread_end,
                                 part_name_thread_cancel,
                                 td);
}

Eina_Stringshare
*parser_group_name_get(parser_data *pd EINA_UNUSED, Evas_Object *entry)
{
   Evas_Object *tb = elm_entry_textblock_get(entry);
   char *text = (char *) evas_object_textblock_text_markup_get(tb);
   char *p = text;

   const char *quot = "&quot;";
   int quot_len = 6; // strlen("&quot;");
   const char *group = "group";
   int group_len = 5; //strlen("group");

   while (p < (text + strlen(text)))
     {
        //Skip "" range
        if (!strncmp(p, quot, quot_len))
          {
             p += quot_len;
             p = strstr(p, quot);
             if (!p) return NULL;
             p += quot_len;
          }

       if (!strncmp(p, group, group_len))
          {
             p += group_len;
             char *name_begin = strstr(p, quot);
             if (!name_begin) return NULL;
             name_begin += quot_len;
             p = name_begin;
             char *name_end = strstr(p, quot);
             if (!name_end) return NULL;
             return eina_stringshare_add_length(name_begin,
                                                name_end - name_begin);
             break;
          }
        p++;
     }
   return NULL;
}


