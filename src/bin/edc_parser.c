#include <Elementary.h>
#include "common.h"

struct parser_s
{
   Eina_Inarray *attrs;
   Ecore_Thread *thread;
   Ecore_Thread *init_thread;
};

typedef struct parser_attr_s
{
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

typedef struct type_init_thread_data_s
{
   Eina_Inarray *attrs;
   parser_data *pd;
} type_init_td;

void
parser_cancel(parser_data *pd)
{
   if (pd->thread) ecore_thread_cancel(pd->thread);
}

char *
parser_name_get(parser_data *pd EINA_UNUSED, const char *cur)
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

   EINA_INARRAY_FOREACH(pd->attrs, attr)
     {
        if ((instring == attr->instring) && strstr(p, attr->keyword))
          return &attr->value;
     }

   return NULL;
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

static void
type_init_thread_blocking(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   type_init_td *td = data;
   parser_attr attr;

   td->attrs = eina_inarray_new(sizeof(parser_attr), 24);
   eina_inarray_step_set(td->attrs, sizeof(Eina_Inarray), sizeof(parser_attr),
                         4);

   //FIXME: construct from the configuration file.

   //Type: Constant
   Eina_Array *types = eina_array_new(12);
   eina_array_push(types, eina_stringshare_add("RECT"));
   eina_array_push(types, eina_stringshare_add("TEXT"));
   eina_array_push(types, eina_stringshare_add("IMAGE"));
   eina_array_push(types, eina_stringshare_add("SWALLOW"));
   eina_array_push(types, eina_stringshare_add("TEXTBLOCK"));
   eina_array_push(types, eina_stringshare_add("GRADIENT"));
   eina_array_push(types, eina_stringshare_add("GROUP"));
   eina_array_push(types, eina_stringshare_add("BOX"));
   eina_array_push(types, eina_stringshare_add("TABLE"));
   eina_array_push(types, eina_stringshare_add("EXTERNAL"));
   eina_array_push(types, eina_stringshare_add("PROXY"));
   eina_array_push(types, eina_stringshare_add("SPACER"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("type:");
   attr.value.strs = types;
   attr.value.type = ATTR_VALUE_CONSTANT;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *comps = eina_array_new(4);
   eina_array_push(comps, eina_stringshare_add("RAW"));
   eina_array_push(comps, eina_stringshare_add("USER"));
   eina_array_push(comps, eina_stringshare_add("COMP"));
   eina_array_push(comps, eina_stringshare_add("LOSSY"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("image:");
   attr.value.strs = comps;
   attr.value.type = ATTR_VALUE_CONSTANT;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *trans = eina_array_new(10);
   eina_array_push(trans, eina_stringshare_add("LINEAR"));
   eina_array_push(trans, eina_stringshare_add("ACCELERATE"));
   eina_array_push(trans, eina_stringshare_add("DECELERATE"));
   eina_array_push(trans, eina_stringshare_add("SINUSOIDAL"));
   eina_array_push(trans, eina_stringshare_add("ACCELERATE_FACTOR"));
   eina_array_push(trans, eina_stringshare_add("DECELERATE_FACTOR"));
   eina_array_push(trans, eina_stringshare_add("SINUSOIDAL_FACTOR"));
   eina_array_push(trans, eina_stringshare_add("DIVISOR_INTERP"));
   eina_array_push(trans, eina_stringshare_add("BOUNCE"));
   eina_array_push(trans, eina_stringshare_add("SPRING"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("transition:");
   attr.value.strs = trans;
   attr.value.type = ATTR_VALUE_CONSTANT;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *aspect = eina_array_new(4);
   eina_array_push(aspect, eina_stringshare_add("NONE"));
   eina_array_push(aspect, eina_stringshare_add("VERTICAL"));
   eina_array_push(aspect, eina_stringshare_add("HORIZONTAL"));
   eina_array_push(aspect, eina_stringshare_add("BOTH"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("aspect_preference:");
   attr.value.strs = aspect;
   attr.value.type = ATTR_VALUE_CONSTANT;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *effect = eina_array_new(11);
   eina_array_push(effect, eina_stringshare_add("NONE"));
   eina_array_push(effect, eina_stringshare_add("PLAIN"));
   eina_array_push(effect, eina_stringshare_add("OUTLINE"));
   eina_array_push(effect, eina_stringshare_add("SOFT_OUTLINE"));
   eina_array_push(effect, eina_stringshare_add("SHADOW"));
   eina_array_push(effect, eina_stringshare_add("SOFT_SHADOW"));
   eina_array_push(effect, eina_stringshare_add("OUTLINE_SHADOW"));
   eina_array_push(effect, eina_stringshare_add("OUTLINE_SOFT_SHADOW"));
   eina_array_push(effect, eina_stringshare_add("FAR_SHADOW"));
   eina_array_push(effect, eina_stringshare_add("FAR_SOFT_SHADOW"));
   eina_array_push(effect, eina_stringshare_add("GLOW"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("effect:");
   attr.value.strs = effect;
   attr.value.type = ATTR_VALUE_CONSTANT;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *action = eina_array_new(23);
   eina_array_push(action, eina_stringshare_add("NONE"));
   eina_array_push(action, eina_stringshare_add("STATE_SET"));
   eina_array_push(action, eina_stringshare_add("ACTION_STOP"));
   eina_array_push(action, eina_stringshare_add("SIGNAL_EMIT"));
   eina_array_push(action, eina_stringshare_add("DRAG_VAL_SET"));
   eina_array_push(action, eina_stringshare_add("DRAG_VAL_STEP"));
   eina_array_push(action, eina_stringshare_add("DRAG_VAL_PAGE"));
   eina_array_push(action, eina_stringshare_add("SCRIPT"));
   eina_array_push(action, eina_stringshare_add("FOCUS_SET"));
   eina_array_push(action, eina_stringshare_add("FOCUS_OBJECT"));
   eina_array_push(action, eina_stringshare_add("PARAM_COPY"));
   eina_array_push(action, eina_stringshare_add("PARAM_SET"));
   eina_array_push(action, eina_stringshare_add("PLAY_SAMPLE"));
   eina_array_push(action, eina_stringshare_add("PLAY_TONE"));
   eina_array_push(action, eina_stringshare_add("PHYSICS_IMPULSE"));
   eina_array_push(action, eina_stringshare_add("PHYSICS_TORQUE_IMPULSE"));
   eina_array_push(action, eina_stringshare_add("PHYSICS_FORCE"));
   eina_array_push(action, eina_stringshare_add("PHYSICS_TORQUE"));
   eina_array_push(action, eina_stringshare_add("PHYSICS_FORCES_CLEAR"));
   eina_array_push(action, eina_stringshare_add("PHYSICS_VEL_SET"));
   eina_array_push(action, eina_stringshare_add("PHYSICS_ANG_VEL_SET"));
   eina_array_push(action, eina_stringshare_add("PHYSICS_STOP"));
   eina_array_push(action, eina_stringshare_add("PHYSICS_ROT_SET"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("action:");
   attr.value.strs = action;
   attr.value.type = ATTR_VALUE_CONSTANT;
   eina_inarray_push(td->attrs, &attr);

   //Type: Integer
   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("color:");
   attr.value.min = 0;
   attr.value.max = 255;
   attr.value.type = ATTR_VALUE_INTEGER;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("color2:");
   attr.value.min = 0;
   attr.value.max = 255;
   attr.value.type = ATTR_VALUE_INTEGER;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("color3:");
   attr.value.min = 0;
   attr.value.max = 255;
   attr.value.type = ATTR_VALUE_INTEGER;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("scale:");
   attr.value.min = 0;
   attr.value.max = 1;
   attr.value.type = ATTR_VALUE_INTEGER;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("fixed:");
   attr.value.min = 0;
   attr.value.max = 1;
   attr.value.type = ATTR_VALUE_INTEGER;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("size:");
   attr.value.min = 1;
   attr.value.max = 255;
   attr.value.type = ATTR_VALUE_INTEGER;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("min:");
   attr.value.min = 0;
   attr.value.max = 1000;
   attr.value.type = ATTR_VALUE_INTEGER;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("max:");
   attr.value.min = 0;
   attr.value.max = 1000;
   attr.value.type = ATTR_VALUE_INTEGER;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("mouse_events:");
   attr.value.min = 0;
   attr.value.max = 1000;
   attr.value.type = ATTR_VALUE_INTEGER;
   eina_inarray_push(td->attrs, &attr);

   //Type: Float
   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("relative:");
   attr.value.min = 0.0;
   attr.value.max = 1;
   attr.value.type = ATTR_VALUE_FLOAT;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("aspect:");
   attr.value.min = 0.0;
   attr.value.max = 1.0;
   attr.value.type = ATTR_VALUE_FLOAT;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("align");
   attr.value.min = 0.0;
   attr.value.max = 1.0;
   attr.value.type = ATTR_VALUE_FLOAT;
   eina_inarray_push(td->attrs, &attr);

   //Type: Part
   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("target:");
   attr.instring = EINA_TRUE;
   attr.value.type = ATTR_VALUE_PART;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("to:");
   attr.instring = EINA_TRUE;
   attr.value.type = ATTR_VALUE_PART;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("source:");
   attr.instring = EINA_TRUE;
   attr.value.type = ATTR_VALUE_PART;
   eina_inarray_push(td->attrs, &attr);

   //Type: State
   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("STATE_SET");
   attr.instring = EINA_TRUE;
   attr.value.type = ATTR_VALUE_STATE;
   attr.value.program = EINA_TRUE;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("inherit:");
   attr.instring = EINA_TRUE;
   attr.value.type = ATTR_VALUE_STATE;
   eina_inarray_push(td->attrs, &attr);

   //Type: Image
   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("normal:");
   attr.instring = EINA_TRUE;
   attr.value.type = ATTR_VALUE_IMAGE;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("tween:");
   attr.instring = EINA_TRUE;
   attr.value.type = ATTR_VALUE_IMAGE;
   eina_inarray_push(td->attrs, &attr);

   //Type: Program
   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("after:");
   attr.instring = EINA_TRUE;
   attr.value.type = ATTR_VALUE_PROGRAM;
   eina_inarray_push(td->attrs, &attr);
}

static void
type_init_thread_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   type_init_td *td = data;
   td->pd->init_thread = NULL;
   td->pd->attrs = td->attrs;
   free(td);
}

static void
type_init_thread_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   type_init_td *td = data;
   td->pd->init_thread = NULL;
   free(td);
}

Eina_Stringshare *
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
               return eina_stringshare_add_length(gi->str, gi->len);
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
   if (!td)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }

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
   if (!td)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }

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

Eina_Stringshare *
parser_first_group_name_get(parser_data *pd EINA_UNUSED, Evas_Object *entry)
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
        while ((p = strstr(p, " ")))
           {
              pp = p;
              p++;
           }
        ret = eina_list_append(ret, strndup(state, pp - state));
     }
   return ret;
}

parser_data *
parser_init(void)
{
   parser_data *pd = calloc(1, sizeof(parser_data));
   if (!pd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }

   type_init_td *td = calloc(1, sizeof(type_init_td));
   if (!td)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        free(pd);
        return NULL;
     }

   td->pd = pd;
   pd->init_thread = ecore_thread_run(type_init_thread_blocking,
                                      type_init_thread_end,
                                      type_init_thread_cancel, td);
   return pd;
}

void
parser_term(parser_data *pd)
{
   ecore_thread_cancel(pd->thread);
   ecore_thread_cancel(pd->init_thread);

   parser_attr *attr;

   EINA_INARRAY_FOREACH(pd->attrs, attr)
     {
        eina_stringshare_del(attr->keyword);

        if (attr->value.strs)
          {
             while (eina_array_count(attr->value.strs))
               eina_stringshare_del(eina_array_pop(attr->value.strs));
             eina_array_free(attr->value.strs);
          }
     }

   eina_inarray_free(pd->attrs);

   free(pd);
}

static const char *
end_of_parts_block_find(const char *pos)
{
   //TODO: Process comments and quotes.
   pos = strstr(pos, "parts");
   if (!pos) return NULL;
   pos = strstr(pos, "{");
   if (!pos) return NULL;
   pos++;
   char level = 1;

   while (*pos)
     {
        if (*pos == '{') level++;
        else if (*pos == '}') level--;

        if (!level) return --pos;
        pos++;
     }
   return NULL;
}

static const char *
group_beginning_pos_get(const char* source, const char *group_name)
{
   const char* GROUP_SYNTAX_NAME = "group";
   const int quot_len = QUOT_LEN;
   const char *quot = QUOT;

   const char *pos = strstr(source, GROUP_SYNTAX_NAME);

   //TODO: Process comments and quotes.
   while (pos)
   {
      const char *name = strstr(pos, quot);
      if (!name) return NULL;
      pos = strstr(++name, quot);
      if (!pos) return NULL;
      if (!strncmp(name, group_name, strlen(group_name)))
        return pos;
      pos = strstr(++pos,  GROUP_SYNTAX_NAME);
   }

   return NULL;
}

int
parser_end_of_parts_block_pos_get(const Evas_Object *entry,
                                  const char *group_name)
{
   if (!group_name) return -1;

   const char *text = elm_entry_entry_get(entry);
   if (!text) return -1;

   char *utf8 = elm_entry_markup_to_utf8(text);
   if (!utf8) return -1;

   const char *pos = group_beginning_pos_get(utf8, group_name);
   if (!pos)
     {
        free(utf8);
        return -1;
     }

   pos = end_of_parts_block_find(++pos);
   if (!pos)
     {
        free(utf8);
        return - 1;
     }

   int ret = (pos - utf8) + 1;

   free(utf8);

   return ret;
}

Eina_Bool
parser_images_pos_get(const Evas_Object *entry, int *ret)
{
   const char* GROUP_SYNTAX_NAME = "group";
   const char* IMAGES_BLOCK_NAME = "images";
   const int IMAGES_BLOCK_NAME_LEN = 6; //strlen of "images"

   if (!ret) return EINA_FALSE;

   *ret = -1;

   const char *text = elm_entry_entry_get(entry);
   if (!text) return EINA_FALSE;

   char *utf8 = elm_entry_markup_to_utf8(text);
   if (!utf8) return EINA_FALSE;

   const char *pos = strstr(utf8, IMAGES_BLOCK_NAME);
   if (pos)
     {
        /* TODO: Remove this check and process lines of the form
           "images.image: "logo.png" COMP;" */
        if (*(pos + IMAGES_BLOCK_NAME_LEN + 1) == '.')
          return EINA_FALSE;

        pos = strstr(pos, "{\n");
        if (!pos) return EINA_FALSE;

        *ret = pos - utf8 + 2;
        return EINA_TRUE;
     }
   pos = strstr(utf8, GROUP_SYNTAX_NAME);
   if (pos)
     {
        *ret = pos - utf8;
        return EINA_FALSE;
     }
   return EINA_FALSE;
}

