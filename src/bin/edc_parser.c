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
   Eina_Bool instring : 1;
} parser_attr;

typedef struct cur_name_thread_data_s
{
   parser_data *pd;
   char *utf8;
   int cur_pos;
   const char *group_name;
   const char *part_name;
   void (*cb)(void *data, Eina_Stringshare *part_name,
              Eina_Stringshare *group_name);
   void *cb_data;
} cur_name_td;

void
parser_cancel(parser_data *pd)
{
   if (pd->thread) ecore_thread_cancel(pd->thread);
}

static void
parser_type_init(parser_data *pd)
{
   parser_attr *attr;

   //FIXME: construct from the configuration file.

   //Type: Constant
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
   attr->keyword = eina_stringshare_add("type:");
   attr->value.strs = types;
   attr->value.type = ATTR_VALUE_CONSTANT;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   Eina_List *comps = NULL;
   comps = eina_list_append(comps, eina_stringshare_add("RAW"));
   comps = eina_list_append(comps, eina_stringshare_add("USER"));
   comps = eina_list_append(comps, eina_stringshare_add("COMP"));
   comps = eina_list_append(comps, eina_stringshare_add("LOSSY"));

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("image:");
   attr->value.strs = comps;
   attr->value.type = ATTR_VALUE_CONSTANT;
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
   attr->keyword = eina_stringshare_add("transition:");
   attr->value.strs = trans;
   attr->value.type = ATTR_VALUE_CONSTANT;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   Eina_List *aspect = NULL;
   aspect = eina_list_append(aspect, eina_stringshare_add("NONE"));
   aspect = eina_list_append(aspect, eina_stringshare_add("VERTICAL"));
   aspect = eina_list_append(aspect, eina_stringshare_add("HORIZONTAL"));
   aspect = eina_list_append(aspect, eina_stringshare_add("BOTH"));

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("aspect_preference:");
   attr->value.strs = aspect;
   attr->value.type = ATTR_VALUE_CONSTANT;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   Eina_List *effect = NULL;
   effect = eina_list_append(effect, eina_stringshare_add("NONE"));
   effect = eina_list_append(effect, eina_stringshare_add("PLAIN"));
   effect = eina_list_append(effect, eina_stringshare_add("OUTLINE"));
   effect = eina_list_append(effect, eina_stringshare_add("SOFT_OUTLINE"));
   effect = eina_list_append(effect, eina_stringshare_add("SHADOW"));
   effect = eina_list_append(effect, eina_stringshare_add("SOFT_SHADOW"));
   effect = eina_list_append(effect, eina_stringshare_add("OUTLINE_SHADOW"));
   effect = eina_list_append(effect, eina_stringshare_add("OUTLINE_SOFT_SHADOW"));
   effect = eina_list_append(effect, eina_stringshare_add("FAR_SHADOW"));
   effect = eina_list_append(effect, eina_stringshare_add("FAR_SOFT_SHADOW"));
   effect = eina_list_append(effect, eina_stringshare_add("GLOW"));
/*
   Eina_List *action = NULL;
   action = eina_list_append(action, eina_stringshare_add("NONE"));
   action = eina_list_append(action, eina_stringshare_add("STATE_SET"));
   action = eina_list_append(action, eina_stringshare_add("ACTION_STOP"));
   action = eina_list_append(action, eina_stringshare_add("SIGNAL_EMIT"));
   action = eina_list_append(action, eina_stringshare_add("DRAG_VAL_SET"));
   action = eina_list_append(action, eina_stringshare_add("DRAG_VAL_STEP"));
   action = eina_list_append(action, eina_stringshare_add("DRAG_VAL_PAGE"));
   action = eina_list_append(action, eina_stringshare_add("SCRIPT"));
   action = eina_list_append(action, eina_stringshare_add("FOCUS_SET"));
   action = eina_list_append(action, eina_stringshare_add("FOCUS_OBJECT"));
   action = eina_list_append(action, eina_stringshare_add("PARAM_COPY"));
   action = eina_list_append(action, eina_stringshare_add("PARAM_SET"));
   action = eina_list_append(action, eina_stringshare_add("PLAY_SAMPLE"));
   action = eina_list_append(action, eina_stringshare_add("PLAY_TONE"));
   action = eina_list_append(action, eina_stringshare_add("PHYSICS_IMPULSE"));
   action = eina_list_append(action, eina_stringshare_add("PHYSICS_TORQUE_IMPULSE"));
   action = eina_list_append(action, eina_stringshare_add("PHYSICS_FORCE"));
   action = eina_list_append(action, eina_stringshare_add("PHYSICS_TORQUE"));
   action = eina_list_append(action, eina_stringshare_add("PHYSICS_FORCES_CLEAR"));
   action = eina_list_append(action, eina_stringshare_add("PHYSICS_VEL_SET"));
   action = eina_list_append(action, eina_stringshare_add("PHYSICS_ANG_VEL_SET"));
   action = eina_list_append(action, eina_stringshare_add("PHYSICS_STOP"));
   action = eina_list_append(action, eina_stringshare_add("PHYSICS_ROT_SET"));

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("action:");
   attr->value.strs = action;
   attr->value.type = ATTR_VALUE_CONSTANT;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);
*/
   //Type: Integer
   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("color:");
   attr->value.min = 0;
   attr->value.max = 255;
   attr->value.type = ATTR_VALUE_INTEGER;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("scale:");
   attr->value.min = 0;
   attr->value.max = 1;
   attr->value.type = ATTR_VALUE_INTEGER;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("fixed:");
   attr->value.min = 0;
   attr->value.max = 1;
   attr->value.type = ATTR_VALUE_INTEGER;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("size:");
   attr->value.min = 1;
   attr->value.max = 255;
   attr->value.type = ATTR_VALUE_INTEGER;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("min:");
   attr->value.min = 0;
   attr->value.max = 1000;
   attr->value.type = ATTR_VALUE_INTEGER;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("max:");
   attr->value.min = 0;
   attr->value.max = 1000;
   attr->value.type = ATTR_VALUE_INTEGER;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("mouse_events:");
   attr->value.min = 0;
   attr->value.max = 1000;
   attr->value.type = ATTR_VALUE_INTEGER;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   //Type: Float
   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("relative:");
   attr->value.min = 0.0;
   attr->value.max = 1;
   attr->value.type = ATTR_VALUE_FLOAT;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("aspect:");
   attr->value.min = 0.0;
   attr->value.max = 1.0;
   attr->value.type = ATTR_VALUE_FLOAT;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("align");
   attr->value.min = 0.0;
   attr->value.max = 1.0;
   attr->value.type = ATTR_VALUE_FLOAT;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   //Type: Part
   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("target:");
   attr->instring = EINA_TRUE;
   attr->value.type = ATTR_VALUE_PART;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("to:");
   attr->instring = EINA_TRUE;
   attr->value.type = ATTR_VALUE_PART;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("source:");
   attr->instring = EINA_TRUE;
   attr->value.type = ATTR_VALUE_PART;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   //Type: State
   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("STATE_SET");
   attr->instring = EINA_TRUE;
   attr->value.type = ATTR_VALUE_STATE;
   attr->value.program = EINA_TRUE;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("inherit:");
   attr->instring = EINA_TRUE;
   attr->value.type = ATTR_VALUE_STATE;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   //Type: Image
   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("normal:");
   attr->instring = EINA_TRUE;
   attr->value.type = ATTR_VALUE_IMAGE;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("tween:");
   attr->instring = EINA_TRUE;
   attr->value.type = ATTR_VALUE_IMAGE;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);

   //Type: Program
   attr = calloc(1, sizeof(parser_attr));
   attr->keyword = eina_stringshare_add("after:");
   attr->instring = EINA_TRUE;
   attr->value.type = ATTR_VALUE_PROGRAM;
   pd->attrs = eina_inlist_append(pd->attrs, (Eina_Inlist *) attr);
}

char *parser_name_get(parser_data *pd EINA_UNUSED, const char *cur)
{
   if (!cur) return NULL;

   char *p = (char *) cur;
   char *end;
   p = strstr(p, "\"");
   if (!p) return NULL;
   p++;
   end = strstr(p, "\"");
   if (!end) return NULL;
   return strndup(p, (end - p));
}

attr_value *
parser_attribute_get(parser_data *pd, const char *text, const char *cur)
{
   if (!text || !cur) return NULL;
   if ((*cur == ';') || (*cur == ':')) return NULL;

   parser_attr *attr;
   Eina_Bool instring = EINA_FALSE;
   Eina_Bool necessary = EINA_FALSE;

   char *p = (char *) cur;

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
   if (!p || !necessary) return NULL;

   while (p > text)
     {
        if ((*p == ';') || (*p == '.') || (*p == ' ')) break;
        p--;
     }

   if (!p) return NULL;
   if (p != text) p++;

   EINA_INLIST_FOREACH(pd->attrs, attr)
     {
        if ((instring == attr->instring) && strstr(p, attr->keyword))
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

   parser_attr *attr;
   Eina_Stringshare *str;

   while(pd->attrs)
     {
        attr = EINA_INLIST_CONTAINER_GET(pd->attrs, parser_attr);
        pd->attrs = eina_inlist_remove(pd->attrs, pd->attrs);

        eina_stringshare_del(attr->keyword);
        EINA_LIST_FREE(attr->value.strs, str) eina_stringshare_del(str);
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
group_name_thread_blocking(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   const char *quot = QUOT;
   const char *group = "group";
   const int quot_len = QUOT_LEN;
   const int group_len = 5;  //strlen("group");

   cur_name_td *td = data;
   char *utf8 = td->utf8;
   int cur_pos = td->cur_pos;
   char *p = utf8;
   char *end = utf8 + cur_pos;

   int bracket = 0;
   const char *group_name = NULL;
   int group_name_len = 0;

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

        if (*p == '{')
          {
             bracket++;
             p++;
             continue;
          }

        //Check whether outside of group
        if ((*p == '}') && (p < end))
          {
             bracket--;
             p++;

             if (bracket == 1) group_name = NULL;
             continue;
          }
        //Check Group in
        if (!strncmp(p, group, group_len))
          {
             p += group_len;
             char *name_begin = strstr(p, quot);
             if (!name_begin) goto end;
             name_begin += quot_len;
             p = name_begin;
             char *name_end = strstr(p, quot);
             if (!name_end) goto end;
             group_name = name_begin;
             group_name_len = name_end - name_begin;
             p = name_end + quot_len;
             bracket++;
             continue;
          }
        p++;
     }
   if (group_name)
     group_name = eina_stringshare_add_length(group_name, group_name_len);

end:
   free(utf8);
   td->utf8 = NULL;
   td->group_name = group_name;
}

static void
cur_name_thread_blocking(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   const char *quot = QUOT;
   const char *part = "part";
   const char *parts = "parts";
   const char *group = "group";
   const int quot_len = QUOT_LEN;
   const int part_len = 4; //strlen("part");
   const int parts_len = 5; //strlen("parts");
   const int group_len = 5;  //strlen("group");

   cur_name_td *td = data;
   char *utf8 = td->utf8;
   int cur_pos = td->cur_pos;
   char *p = utf8;
   char *end = utf8 + cur_pos;

   int bracket = 0;
   const char *group_name = NULL;
   const char *part_name = NULL;
   int group_name_len = 0;
   int part_name_len = 0;

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

        if (*p == '{')
          {
             bracket++;
             p++;
             continue;
          }

        //Check whether outside of part or group
        if ((*p == '}') && (p < end))
          {
             bracket--;
             p++;

             if (bracket == 1) group_name = NULL;
             else if (bracket == 3) part_name = NULL;

             continue;
          }
        //Check Part in
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
                  bracket++;
                  continue;
               }
          }
        //Check Group in
        if (!strncmp(p, group, group_len))
          {
             p += group_len;
             char *name_begin = strstr(p, quot);
             if (!name_begin) goto end;
             name_begin += quot_len;
             p = name_begin;
             char *name_end = strstr(p, quot);
             if (!name_end) goto end;
             group_name = name_begin;
             group_name_len = name_end - name_begin;
             p = name_end + quot_len;
             bracket++;
             continue;
          }
        p++;
     }
   if (part_name)
     part_name = eina_stringshare_add_length(part_name, part_name_len);
   if (group_name)
     group_name = eina_stringshare_add_length(group_name, group_name_len);

end:
   free(utf8);
   td->utf8 = NULL;
   td->part_name = part_name;
   td->group_name = group_name;
}

static void
cur_name_thread_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   cur_name_td *td = data;
   td->cb(td->cb_data, td->part_name, td->group_name);
   td->pd->thread = NULL;
   free(td);
}

static void
cur_name_thread_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   cur_name_td *td = data;
   td->pd->thread = NULL;
   free(td->utf8);
   free(td);
}

const char *
parser_paragh_name_get(parser_data *pd EINA_UNUSED, Evas_Object *entry)
{
   //FIXME: list up groups
#define GROUP_CNT 13
   typedef struct _group_info
   {
      char *str;
      int len;
   } group_info;

   group_info group_list[GROUP_CNT] =
     {
        { "collections", 11 },
        { "description", 11 },
        { "fill", 4 },
        { "group", 5 },
        { "images", 6 },
        { "map", 3 },
        { "origin", 6 },
        { "parts", 5 },
        { "part", 4 },
        { "programs", 8 },
        { "program", 7 },
        { "rel1", 4 },
        { "rel2", 4 }
     };

   const char *text = elm_entry_entry_get(entry);
   if (!text) return NULL;

   char *utf8 = elm_entry_markup_to_utf8(text);
   if (!utf8) return NULL;

   int cur_pos = elm_entry_cursor_pos_get(entry);
   if (cur_pos < 1) return NULL;

   const char *quot = QUOT;
   int quot_len = QUOT_LEN;
   char *cur = utf8;
   char *end = cur + cur_pos;
   char *stack[20];
   int depth = 0;

   //1. Figure out depth.
   while (cur <= end)
     {
        //Skip "" range
        if (*cur == *quot)
          {
             cur += quot_len;
             cur = strstr(cur, quot);
             if (!cur) return NULL;
             cur += quot_len;
          }

        if (*cur == '{')
          {
             stack[depth] = cur;
             depth++;
          }
        else if (*cur == '}')
          {
             if (depth > 0) depth--;
          }
        cur++;
     }

   if (depth == 0) return NULL;

   //2. Parse the paragraph Name
   cur = stack[depth - 1];
   int i;
   while (cur > utf8)
     {
        cur--;
        for (i = 0; i < GROUP_CNT; i++)
          {
             group_info *gi = &group_list[i];
             if (!strncmp(cur, gi->str, gi->len))
               return gi->str;
          }
     }

   return NULL;
}

Eina_Stringshare*
parser_cur_name_fast_get(Evas_Object *entry, const char *scope)
{
   const char *quot = QUOT;
   const int quot_len = QUOT_LEN;
   const int scope_len = strlen(scope);

   const char *text = elm_entry_entry_get(entry);
   if (!text) return NULL;

   char *utf8 = elm_entry_markup_to_utf8(text);
   if (!utf8) return NULL;

   int cur_pos = elm_entry_cursor_pos_get(entry);

   char *p = utf8;
   char *end = utf8 + cur_pos;

   int bracket = 0;
   const char *name = NULL;
   int name_len = 0;

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

        if (*p == '{')
          {
             bracket++;
             p++;
             continue;
          }

        //Check whether outside of scope
        if ((*p == '}') && (p < end))
          {
             bracket--;
             p++;

             if (bracket == 1) name = NULL;
             continue;
          }
        //Check Scope in
        if (!strncmp(p, scope, scope_len))
          {
             p += scope_len;
             char *name_begin = strstr(p, quot);
             if (!name_begin) goto end;
             name_begin += quot_len;
             p = name_begin;
             char *name_end = strstr(p, quot);
             if (!name_end) goto end;
             name = name_begin;
             name_len = name_end - name_begin;
             p = name_end + quot_len;
             bracket++;
             continue;
          }
        p++;
     }
   if (name) name = eina_stringshare_add_length(name, name_len);

end:
   free(utf8);
   return name;
}

void
parser_cur_group_name_get(parser_data *pd, Evas_Object *entry,
                          void (*cb)(void *data, Eina_Stringshare *part_name,
                          Eina_Stringshare *group_name), void *data)
{
   if (pd->thread) ecore_thread_cancel(pd->thread);

   cur_name_td *td = calloc(1, sizeof(cur_name_td));
   if (!td) return;

   const char *text = elm_entry_entry_get(entry);
   if (!text) return;

   char *utf8 = elm_entry_markup_to_utf8(text);
   if (!utf8) return;

   td->pd = pd;
   td->utf8 = utf8;
   td->cur_pos = elm_entry_cursor_pos_get(entry);
   td->cb = cb;
   td->cb_data = data;

   pd->thread = ecore_thread_run(group_name_thread_blocking,
                                 cur_name_thread_end,
                                 cur_name_thread_cancel,
                                 td);
}

void
parser_cur_name_get(parser_data *pd, Evas_Object *entry, void (*cb)(void *data,
                    Eina_Stringshare *part_name, Eina_Stringshare *group_name),
                    void *data)
{
   if (pd->thread) ecore_thread_cancel(pd->thread);

   cur_name_td *td = calloc(1, sizeof(cur_name_td));
   if (!td) return;

   const char *text = elm_entry_entry_get(entry);
   if (!text) return;

   char *utf8 = elm_entry_markup_to_utf8(text);
   if (!utf8) return;

   td->pd = pd;
   td->utf8 = utf8;
   td->cur_pos = elm_entry_cursor_pos_get(entry);
   td->cb = cb;
   td->cb_data = data;

   pd->thread = ecore_thread_run(cur_name_thread_blocking,
                                 cur_name_thread_end,
                                 cur_name_thread_cancel,
                                 td);
}

int
parser_line_cnt_get(parser_data *pd EINA_UNUSED, const char *src)
{
   if (!src) return 0;

   int cnt = 0;

   while ((src = strstr(src, EOL)))
     {
        cnt++;
        src += EOL_LEN;
     }

   return cnt;
}

Eina_Stringshare
*parser_first_group_name_get(parser_data *pd EINA_UNUSED, Evas_Object *entry)
{
   Evas_Object *tb = elm_entry_textblock_get(entry);
   char *text = (char *) evas_object_textblock_text_markup_get(tb);
   char *p = text;

   const char *quot = QUOT;
   int quot_len = QUOT_LEN;
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

Eina_List *
parser_states_filtered_name_get(Eina_List *states)
{
   Eina_List *ret = NULL;
   Eina_List *l;
   char *state;
   EINA_LIST_FOREACH(states, l, state)
     {
        char *p = state;
        char *pp = state;
        while (p = strstr(p, " "))
           {
              pp = p;
              p++;
           }
        ret = eina_list_append(ret, strndup(state, pp - state));
     }
   return ret;
}
