#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include "enventor_private.h"

const char ATTR_PREPEND_COLON[] = ":";
const char ATTR_PREPEND_NONE[] = "";
const char ATTR_APPEND_SEMICOLON[] = ";";
const char ATTR_APPEND_TRANSITION_TIME[] = " 1.0;";
const char ATTR_APPEND_TRANSITION_TIME_FACTOR[] = " 1.0 0.0;";
const char ATTR_APPEND_TRANSITION_TIME_FACTOR2[] = " 1.0 0.0 0.0;";
const char ATTR_APPEND_TRANSITION_TIME_FACTOR4[] = " 1.0 0.0 0.0 0.0 0.0;";

typedef struct parser_attr_s
{
   Eina_Stringshare *keyword;
   const char *context;
   attr_value value;
} parser_attr;

typedef struct cur_context_thread_data_s
{
   Ecore_Thread *thread;
   char *utf8;
   int cur_pos;
   const char *group_name;
   const char *part_name;
   const char *state_name;
   double state_value;
   void (*cb)(void *data, Eina_Stringshare *state_name, double state_value,
              Eina_Stringshare *part_name, Eina_Stringshare *group_name);
   void *cb_data;
   parser_data *pd;
   Eina_Bool collections: 1;  //It contains collections?
} cur_context_td;

typedef struct type_init_thread_data_s
{
   Eina_Inarray *attrs;
   Ecore_Thread *thread;
   parser_data *pd;
} type_init_td;

typedef struct bracket_thread_data_s
{
   int pos;
   char *text;
   Bracket_Update_Cb update_cb;
   void *data;
   Ecore_Thread *thread;
   int left;
   int right;
   parser_data *pd;

} bracket_td;


struct parser_s
{
   Eina_Inarray *attrs;
   cur_context_td *cntd;
   type_init_td *titd;
   bracket_td *btd;
};


/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
cur_context_thread_blocking(void *data, Ecore_Thread *thread)
{
#define PART_SYNTAX_CNT 14

   const char *GROUP = "group";
   const int GROUP_LEN = 5;
   const char *PARTS = "parts";
   const int PARTS_LEN = 5;
   const char *PART[PART_SYNTAX_CNT] = { "part", "image", "textblock",
        "swallow", "rect", "group", "spacer", "proxy", "text", "gradient",
        "box", "table", "external", "vector" };
   const int PART_LEN[PART_SYNTAX_CNT] =
     { 4, 5, 9, 6, 4, 5, 6, 5, 4, 8, 3, 5, 8, 6};

   const char *DESC[2] = { "desc", "description" };
   const int DESC_LEN[2] = { 4, 11 };
   const char *STATE = "state";
   const char *DEF_STATE_NAME = "default";
   const int DEF_STATE_LEN = 7;

   cur_context_td *td = data;
   char *utf8 = td->utf8;
   int cur_pos = td->cur_pos;
   char *p = utf8;
   char *end = utf8 + cur_pos;
   int i;
   Eina_Bool inside_parts = EINA_FALSE;
   Eina_Bool collections = td->collections;

   int bracket = 0;
   const char *group_name = NULL;
   const char *part_name = NULL;
   int group_name_len = 0;
   int part_name_len = 0;

   const char *desc_name = NULL;
   int desc_name_len = 0;
   const char *value = NULL;
   int value_len = 0;
   double value_convert = 0.0;

   if (!collections) bracket = 1;

   td->part_name = NULL;
   td->group_name = NULL;
   td->state_name = NULL;

   while (p && p <= end)
     {
        if (ecore_thread_check(thread)) break;

        //Skip "" range
        if (!strncmp(p, QUOT_UTF8, QUOT_UTF8_LEN))
          {
             p += QUOT_UTF8_LEN;
             p = strstr(p, QUOT_UTF8);
             if (!p) return;
             p += QUOT_UTF8_LEN;
             continue;
          }

        //Enter one depth into Bracket.
        if (*p == '{')
          {
             bracket++;
             p++;
             continue;
          }
        //Check inside comment
        if (*p == '/')
          {
             if (p[1] == '/')
               {
                 p = strchr(p, '\n');
                 continue;
               }
             else if (p[1] == '*')
               {
                 p = strstr(p, "*/");
                 continue;
               }
          }
        //Skip #if ~ #endif
        if (!strncmp(p, "#if", 3))
          {
             p = strstr(p, "#endif");
             if (!p) return;
             p += 6; //strlen(#endif)
             continue;
          }

        //Skip #define
        if (!strncmp(p, "#define", 7))
          {
             //escape "\", "ie, #define .... \"
             p += 7; //strlen(#define)

             while (p <= end)
               {
                  char *eol = strstr(p, "\n");
                  if (!eol) return;

                  char *slash = strstr(p, "\\");

                  p = eol + 1;

                  if (!slash || (eol < slash))
                    break;
               }
             continue;
          }
        //Check whether outside of description or part or group
        if ((*p == '}') && (p < end))
          {
             bracket--;
             p++;
             if (bracket == 1) group_name = NULL;
             else if (bracket == 2 && inside_parts == EINA_TRUE)
               inside_parts = EINA_FALSE;
             else if (bracket == 3) part_name = NULL;
             else if (bracket == 4) desc_name = NULL;
             continue;
          }
        //check block "Parts" in
        if (bracket == 2)
          {
             if (!strncmp(p, PARTS, PARTS_LEN))
               {
                 inside_parts = EINA_TRUE;
                 p = strstr(p, "{");
                 if (!p) return;
                 continue;
               }
         }
        //Check Part in
        if (bracket == 3)
          {
             int part_idx = -1;
              //part ? image ? swallow ? text ? rect ?
             for (i = 0; i < PART_SYNTAX_CNT; i++)
                {
                   if (!strncmp(p, PART[i], PART_LEN[i]))
                     {
                        part_idx = i;
                        break;
                     }
                }

              //we got a part!
              if (part_idx != -1)
                {
                   p += PART_LEN[part_idx];
                   char *name_begin = strstr(p, QUOT_UTF8);
                   if (!name_begin) return;
                   name_begin += QUOT_UTF8_LEN;
                   p = name_begin;
                   char *name_end = strstr(p, QUOT_UTF8);
                   if (!name_end) return;
                   part_name = name_begin;
                   part_name_len = name_end - name_begin;
                   p = name_end + QUOT_UTF8_LEN;
                   bracket++;
                   continue;
                }
          }
        //Check Description in
        if (bracket == 4)
          {
             //description? or desc?
             int desc_idx = -1;
             if (!strncmp(p, DESC[1], DESC_LEN[1])) desc_idx = 1;
             else if (!strncmp(p, DESC[0], DESC_LEN[0])) desc_idx = 0;

             //we got a description!
             if (desc_idx != -1)
               {
                  /* By default state will be */
                  desc_name = DEF_STATE_NAME;
                  /* recognized as "default" 0.0*/
                  desc_name_len = DEF_STATE_LEN;
                  value_convert = 0;

                  /* skip keyword */
                  p += DESC_LEN[desc_idx];
                  p = strstr(p, "{");
                  if (!p) return;
                  /*Limit size of text for processing*/
                  char *end_brace = strstr(p, "}");
                  if (!end_brace) return;

                  /* proccessing for "description" keyword with "state"
                     attribute */
                  if (desc_idx == 1)
                    {
                       char *state = strstr(p, STATE);
                       /* if name of state didn't find, */
                       if (!state || state > end_brace)
                         {
                            /* description will recognized as default 0.0*/
                            continue;
                         }
                       else
                         {
                            /*5 is strlen("state");*/
                            p += 5;
                         }
                    }

                  char *name_begin = strstr(p, QUOT_UTF8);
                  if (!name_begin)
                     continue;
                  char *end_range = strstr(p, ";");
                  if (!end_range) return;

                  /* if string placed outside desc block*/
                  if ((name_begin > end_brace) || (name_begin > end_range) ||
                      (end_range > end_brace))
                    continue;

                  /* Exception cases like: desc {image.normal: "img";} */
                  int alpha_present = 0;
                  char *string_itr;
                  for (string_itr = name_begin;
                       (string_itr > p) && (!alpha_present); string_itr--)
                    alpha_present = isalpha((int)*string_itr);

                  if (alpha_present && desc_idx == 0)
                    continue;

                  /*Extract state name and value */
                  name_begin += QUOT_UTF8_LEN;
                  p = name_begin;
                  char *name_end = strstr(p, QUOT_UTF8);
                  if (!name_end) return;
                  desc_name = name_begin;
                  desc_name_len = name_end - name_begin;
                  p = name_end + QUOT_UTF8_LEN;
                  value = p;
                  bracket++;

                  char *value_end = strchr(value, ';');
                  char *value_buf = NULL;
                  while (value < value_end)
                    {
                       if (isdigit(*value) || *value == '.')
                         {
                            value_len = value_end - value;
                            value_buf = (char *)calloc(1, value_len);
                            memcpy(value_buf, value, value_len);
                            break;
                         }
                       value++;
                    }
                  if (value_buf)
                    {
                       value_convert = atof(value_buf);
                       free(value_buf);
                    }
                  continue;
               }
          }
        //Check Group in. Probably inside of collections or the most outside.
        if (bracket == 1)
          {
             if (!strncmp(p, GROUP, GROUP_LEN))
               {
                  p += GROUP_LEN;
                  char *name_begin = strstr(p, QUOT_UTF8);
                  if (!name_begin) return;
                  name_begin += QUOT_UTF8_LEN;
                  p = name_begin;
                  char *name_end = strstr(p, QUOT_UTF8);
                  if (!name_end) return;

                  group_name = name_begin;
                  group_name_len = name_end - name_begin;
                  p = name_end + QUOT_UTF8_LEN;
                  bracket++;
                  continue;
               }
          }
        p++;
     }
   if (part_name)
     part_name = eina_stringshare_add_length(part_name, part_name_len);
   if (desc_name)
     desc_name = eina_stringshare_add_length(desc_name, desc_name_len);
   if (group_name)
     group_name = eina_stringshare_add_length(group_name, group_name_len);

   td->part_name = part_name;
   td->group_name = group_name;
   td->state_name = desc_name;
   td->state_value = value_convert;
}

Eina_Stringshare *
parser_cur_context_group_name_get(parser_data *pd EINA_UNUSED,
                                  Evas_Object *entry, Eina_Bool collections)
{
   const char *GROUP = "group";
   const int GROUP_LEN = 5;
   const char *markup = elm_entry_entry_get(entry);
   char *utf8 = elm_entry_markup_to_utf8(markup);
   char *p = utf8;
   int cur_pos = elm_entry_cursor_pos_get(entry);
   char *end = utf8 + cur_pos;
   int bracket = 0;
   const char *group_name = NULL;
   int group_name_len = 0;

   if (!collections) bracket = 1;

   while (p && (p <= end))
     {
        //Skip "" range
        if (!strncmp(p, QUOT_UTF8, QUOT_UTF8_LEN))
          {
             p += QUOT_UTF8_LEN;
             p = strstr(p, QUOT_UTF8);
             if (!p) goto end;
             p += QUOT_UTF8_LEN;
             continue;
          }
        //Enter one depth into Bracket.
        if (*p == '{')
          {
             bracket++;
             p++;
             continue;
          }
        //Check inside comment
        if (*p == '/')
          {
             if (p[1] == '/')
               {
                 p = strchr(p, '\n');
                 continue;
               }
             else if (p[1] == '*')
               {
                 p = strstr(p, "*/");
                 continue;
               }
          }
        //Skip #if ~ #endif
        if (!strncmp(p, "#if", 3))
          {
             p = strstr(p, "#endif");
             if (!p) goto end;
             p += 6; //strlen(#endif)
             continue;
          }
        //Skip #define
        if (!strncmp(p, "#define", 7))
          {
             //escape "\", "ie, #define .... \"
             p += 7; //strlen(#define)

             while (p <= end)
               {
                  char *eol = strstr(p, "\n");
                  if (!eol) goto end;

                  char *slash = strstr(p, "\\");

                  p = eol + 1;

                  if (!slash || (eol < slash))
                    break;
               }
             continue;
          }
        //Check whether outside of description or part or group
        if ((*p == '}') && (p < end))
          {
             bracket--;
             p++;
             if (bracket == 1) group_name = NULL;
             continue;
          }
        //Check Group in. Probably inside of collections or the most outside.
        if (bracket == 1)
          {
             if (!strncmp(p, GROUP, GROUP_LEN))
               {
                  p += GROUP_LEN;
                  char *name_begin = strstr(p, QUOT_UTF8);
                  if (!name_begin) goto end;
                  name_begin += QUOT_UTF8_LEN;
                  p = name_begin;
                  char *name_end = strstr(p, QUOT_UTF8);
                  if (!name_end) goto end;

                  group_name = name_begin;
                  group_name_len = name_end - name_begin;
                  p = name_end + QUOT_UTF8_LEN;
                  bracket++;
                  continue;
               }
          }
        p++;
     }
   if (group_name)
     group_name = eina_stringshare_add_length(group_name, group_name_len);

end:
   free(utf8);
   return group_name;
}

static void
cur_context_thread_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   cur_context_td *td = data;
   if (td->pd) td->pd->cntd = NULL;
   eina_stringshare_del(td->state_name);
   eina_stringshare_del(td->part_name);
   eina_stringshare_del(td->group_name);
   free(td->utf8);
   free(td);
}

static void
cur_context_thread_end(void *data, Ecore_Thread *thread)
{
   cur_context_td *td = data;
   td->cb(td->cb_data, td->state_name, td->state_value,  td->part_name,
          td->group_name);
   cur_context_thread_cancel(data, thread);
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


   //Context depended attributes
   Eina_Array *wh = eina_array_new(2);
   eina_array_push(wh, eina_stringshare_add("W:"));
   eina_array_push(wh, eina_stringshare_add("H:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("min");
   attr.value.strs = wh;
   attr.value.cnt = 2;
   attr.value.min = 0;
   attr.value.max = 1;
   attr.value.type = ATTR_VALUE_BOOLEAN;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   attr.context = eina_stringshare_add("text");
   eina_inarray_push(td->attrs, &attr);

   wh = eina_array_new(2);
   eina_array_push(wh, eina_stringshare_add("W:"));
   eina_array_push(wh, eina_stringshare_add("H:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("max");
   attr.value.strs = wh;
   attr.value.cnt = 2;
   attr.value.min = 0;
   attr.value.max = 1;
   attr.value.type = ATTR_VALUE_BOOLEAN;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   attr.context = eina_stringshare_add("text");
   eina_inarray_push(td->attrs, &attr);

   // Context independed attributes
   Eina_Array *trans = eina_array_new(11);
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
   eina_array_push(trans, eina_stringshare_add("CUBIC_BEZIER"));

   Eina_Array *trans_append_str = eina_array_new(11);
   eina_array_push(trans_append_str, eina_stringshare_add(ATTR_APPEND_TRANSITION_TIME));
   eina_array_push(trans_append_str, eina_stringshare_add(ATTR_APPEND_TRANSITION_TIME));
   eina_array_push(trans_append_str, eina_stringshare_add(ATTR_APPEND_TRANSITION_TIME));
   eina_array_push(trans_append_str, eina_stringshare_add(ATTR_APPEND_TRANSITION_TIME));
   eina_array_push(trans_append_str, eina_stringshare_add(ATTR_APPEND_TRANSITION_TIME_FACTOR));
   eina_array_push(trans_append_str, eina_stringshare_add(ATTR_APPEND_TRANSITION_TIME_FACTOR));
   eina_array_push(trans_append_str, eina_stringshare_add(ATTR_APPEND_TRANSITION_TIME_FACTOR));
   eina_array_push(trans_append_str, eina_stringshare_add(ATTR_APPEND_TRANSITION_TIME_FACTOR2));
   eina_array_push(trans_append_str, eina_stringshare_add(ATTR_APPEND_TRANSITION_TIME_FACTOR2));
   eina_array_push(trans_append_str, eina_stringshare_add(ATTR_APPEND_TRANSITION_TIME_FACTOR2));
   eina_array_push(trans_append_str, eina_stringshare_add(ATTR_APPEND_TRANSITION_TIME_FACTOR4));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("transition");
   attr.value.strs = trans;
   attr.value.type = ATTR_VALUE_CONSTANT;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str_array = trans_append_str;
   attr.value.use_append_str_array = EINA_TRUE;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *aspect_mode = eina_array_new(5);
   eina_array_push(aspect_mode, eina_stringshare_add("NONE"));
   eina_array_push(aspect_mode, eina_stringshare_add("NEITHER"));
   eina_array_push(aspect_mode, eina_stringshare_add("VERTICAL"));
   eina_array_push(aspect_mode, eina_stringshare_add("HORIZONTAL"));
   eina_array_push(aspect_mode, eina_stringshare_add("BOTH"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("aspect_mode");
   attr.value.strs = aspect_mode;
   attr.value.type = ATTR_VALUE_CONSTANT;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *aspect_prefer = eina_array_new(5);
   eina_array_push(aspect_prefer, eina_stringshare_add("NONE"));
   eina_array_push(aspect_prefer, eina_stringshare_add("VERTICAL"));
   eina_array_push(aspect_prefer, eina_stringshare_add("HORIZONTAL"));
   eina_array_push(aspect_prefer, eina_stringshare_add("BOTH"));
   eina_array_push(aspect_prefer, eina_stringshare_add("SOURCE"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("aspect_preference");
   attr.value.strs = aspect_prefer;
   attr.value.type = ATTR_VALUE_CONSTANT;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
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
   attr.keyword = eina_stringshare_add("effect");
   attr.value.strs = effect;
   attr.value.type = ATTR_VALUE_CONSTANT;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *action = eina_array_new(22);
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
   attr.keyword = eina_stringshare_add("action");
   attr.value.strs = action;
   attr.value.type = ATTR_VALUE_CONSTANT;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *sig = eina_array_new(15);
   eina_array_push(sig, eina_stringshare_add("\"mouse,down,*\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,down,1\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,down,2\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,down,3\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,up,*\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,up,1\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,up,2\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,up,3\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,clicked,*\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,clicked,1\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,clicked,2\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,clicked,3\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,move\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,in\""));
   eina_array_push(sig, eina_stringshare_add("\"mouse,out\""));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("signal");
   attr.value.strs = sig;
   attr.value.type = ATTR_VALUE_CONSTANT;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   //Type: Integer
   Eina_Array *rgba = eina_array_new(4);
   eina_array_push(rgba, eina_stringshare_add("R:"));
   eina_array_push(rgba, eina_stringshare_add("G:"));
   eina_array_push(rgba, eina_stringshare_add("B:"));
   eina_array_push(rgba, eina_stringshare_add("A:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("color");
   attr.value.strs = rgba;
   attr.value.cnt = 4;
   attr.value.min = 0;
   attr.value.max = 255;
   attr.value.type = ATTR_VALUE_INTEGER;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   rgba = eina_array_new(4);
   eina_array_push(rgba, eina_stringshare_add("R:"));
   eina_array_push(rgba, eina_stringshare_add("G:"));
   eina_array_push(rgba, eina_stringshare_add("B:"));
   eina_array_push(rgba, eina_stringshare_add("A:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("color2");
   attr.value.strs = rgba;
   attr.value.cnt = 4;
   attr.value.min = 0;
   attr.value.max = 255;
   attr.value.type = ATTR_VALUE_INTEGER;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   rgba = eina_array_new(4);
   eina_array_push(rgba, eina_stringshare_add("R:"));
   eina_array_push(rgba, eina_stringshare_add("G:"));
   eina_array_push(rgba, eina_stringshare_add("B:"));
   eina_array_push(rgba, eina_stringshare_add("A:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("color3");
   attr.value.strs = rgba;
   attr.value.cnt = 4;
   attr.value.min = 0;
   attr.value.max = 255;
   attr.value.type = ATTR_VALUE_INTEGER;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *scale = eina_array_new(1);
   eina_array_push(scale, eina_stringshare_add("Scale:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("scale");
   attr.value.strs = scale;
   attr.value.cnt = 1;
   attr.value.min = 0;
   attr.value.max = 1;
   attr.value.type = ATTR_VALUE_BOOLEAN;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *smooth = eina_array_new(1);
   eina_array_push(smooth, eina_stringshare_add("Smooth:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("smooth");
   attr.value.strs = smooth;
   attr.value.cnt = 1;
   attr.value.min = 0;
   attr.value.max = 1;
   attr.value.type = ATTR_VALUE_BOOLEAN;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *visible = eina_array_new(1);
   eina_array_push(visible, eina_stringshare_add("Visibility:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("visible");
   attr.value.strs = visible;
   attr.value.cnt = 1;
   attr.value.min = 0;
   attr.value.max = 1;
   attr.value.type = ATTR_VALUE_BOOLEAN;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *onoff = eina_array_new(1);
   eina_array_push(onoff, eina_stringshare_add("Map:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("on");
   attr.value.strs = onoff;
   attr.value.cnt = 1;
   attr.value.min = 0;
   attr.value.max = 1;
   attr.value.type = ATTR_VALUE_BOOLEAN;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *xy = eina_array_new(2);
   eina_array_push(xy, eina_stringshare_add("X:"));
   eina_array_push(xy, eina_stringshare_add("Y:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("fixed");
   attr.value.strs = xy;
   attr.value.cnt = 2;
   attr.value.min = 0;
   attr.value.max = 1;
   attr.value.type = ATTR_VALUE_BOOLEAN;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *size = eina_array_new(1);
   eina_array_push(size, eina_stringshare_add("Size:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("size");
   attr.value.strs = size;
   attr.value.cnt = 1;
   attr.value.min = 1;
   attr.value.max = 255;
   attr.value.type = ATTR_VALUE_INTEGER;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   wh = eina_array_new(2);
   eina_array_push(wh, eina_stringshare_add("W:"));
   eina_array_push(wh, eina_stringshare_add("H:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("min");
   attr.value.strs = wh;
   attr.value.cnt = 2;
   attr.value.min = 0;
   attr.value.max = 1000;
   attr.value.type = ATTR_VALUE_INTEGER;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   wh = eina_array_new(2);
   eina_array_push(wh, eina_stringshare_add("W:"));
   eina_array_push(wh, eina_stringshare_add("H:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("max");
   attr.value.strs = wh;
   attr.value.cnt = 2;
   attr.value.min = 0;
   attr.value.max = 1000;
   attr.value.type = ATTR_VALUE_INTEGER;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *mouse_events = eina_array_new(1);
   eina_array_push(mouse_events, eina_stringshare_add("Mouse Events:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("mouse_events");
   attr.value.strs = mouse_events;
   attr.value.cnt = 1;
   attr.value.min = 0;
   attr.value.max = 1;
   attr.value.type = ATTR_VALUE_BOOLEAN;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   xy = eina_array_new(2);
   eina_array_push(xy, eina_stringshare_add("X:"));
   eina_array_push(xy, eina_stringshare_add("Y:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("offset");
   attr.value.strs = xy;
   attr.value.cnt = 2;
   attr.value.min = -100;
   attr.value.max = 100;
   attr.value.type = ATTR_VALUE_INTEGER;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   //Type: Float
   xy = eina_array_new(2);
   eina_array_push(xy, eina_stringshare_add("X:"));
   eina_array_push(xy, eina_stringshare_add("Y:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("relative");
   attr.value.strs = xy;
   attr.value.cnt = 2;
   attr.value.min = 0.0;
   attr.value.max = 1;
   attr.value.type = ATTR_VALUE_FLOAT;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   wh = eina_array_new(2);
   eina_array_push(wh, eina_stringshare_add("Min:"));
   eina_array_push(wh, eina_stringshare_add("Max:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("aspect");
   attr.value.strs = wh;
   attr.value.cnt = 2;
   attr.value.min = 0.0;
   attr.value.max = 1.0;
   attr.value.type = ATTR_VALUE_FLOAT;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   xy = eina_array_new(2);
   eina_array_push(xy, eina_stringshare_add("X:"));
   eina_array_push(xy, eina_stringshare_add("Y:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("align");
   attr.value.strs = xy;
   attr.value.cnt = 2;
   attr.value.min = 0.0;
   attr.value.max = 1.0;
   attr.value.type = ATTR_VALUE_FLOAT;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *duration = eina_array_new(1);
   eina_array_push(duration, eina_stringshare_add("Time:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("LINEAR");
   attr.value.strs = duration;
   attr.value.cnt = 1;
   attr.value.min = 0.0;
   attr.value.max = 5.0;
   attr.value.type = ATTR_VALUE_FLOAT;
   attr.value.prepend_str = ATTR_PREPEND_NONE;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   duration = eina_array_new(1);
   eina_array_push(duration, eina_stringshare_add("Time:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("ACCELERATE");
   attr.value.strs = duration;
   attr.value.cnt = 1;
   attr.value.min = 0.0;
   attr.value.max = 5.0;
   attr.value.type = ATTR_VALUE_FLOAT;
   attr.value.prepend_str = ATTR_PREPEND_NONE;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   duration = eina_array_new(1);
   eina_array_push(duration, eina_stringshare_add("Time:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("DECELERATE");
   attr.value.strs = duration;
   attr.value.cnt = 1;
   attr.value.min = 0.0;
   attr.value.max = 5.0;
   attr.value.type = ATTR_VALUE_FLOAT;
   attr.value.prepend_str = ATTR_PREPEND_NONE;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   duration = eina_array_new(1);
   eina_array_push(duration, eina_stringshare_add("Time:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("SINUSOIDAL");
   attr.value.strs = duration;
   attr.value.cnt = 1;
   attr.value.min = 0.0;
   attr.value.max = 5.0;
   attr.value.type = ATTR_VALUE_FLOAT;
   attr.value.prepend_str = ATTR_PREPEND_NONE;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *duration_factor = eina_array_new(2);
   eina_array_push(duration_factor, eina_stringshare_add("Time:"));
   eina_array_push(duration_factor, eina_stringshare_add("Factor:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("ACCELERATE_FACTOR");
   attr.value.strs = duration_factor;
   attr.value.cnt = 2;
   attr.value.min = 0.0;
   attr.value.max = 5.0;
   attr.value.type = ATTR_VALUE_FLOAT;
   attr.value.prepend_str = ATTR_PREPEND_NONE;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   duration_factor = eina_array_new(2);
   eina_array_push(duration_factor, eina_stringshare_add("Time:"));
   eina_array_push(duration_factor, eina_stringshare_add("Factor:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("DECELERATE_FACTOR");
   attr.value.strs = duration_factor;
   attr.value.cnt = 2;
   attr.value.min = 0.0;
   attr.value.max = 5.0;
   attr.value.type = ATTR_VALUE_FLOAT;
   attr.value.prepend_str = ATTR_PREPEND_NONE;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   duration_factor = eina_array_new(2);
   eina_array_push(duration_factor, eina_stringshare_add("Time:"));
   eina_array_push(duration_factor, eina_stringshare_add("Factor:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("SINUSOIDAL_FACTOR");
   attr.value.strs = duration_factor;
   attr.value.cnt = 2;
   attr.value.min = 0.0;
   attr.value.max = 5.0;
   attr.value.type = ATTR_VALUE_FLOAT;
   attr.value.prepend_str = ATTR_PREPEND_NONE;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   Eina_Array *base_scale = eina_array_new(1);
   eina_array_push(base_scale, eina_stringshare_add("Scale:"));

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("base_scale");
   attr.value.strs = base_scale;
   attr.value.cnt = 1;
   attr.value.min = 0.01;
   attr.value.max = 10.0;
   attr.value.type = ATTR_VALUE_FLOAT;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   //Type: Part
   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("target");
   attr.value.type = ATTR_VALUE_PART;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("to");
   attr.value.type = ATTR_VALUE_PART;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("source");
   attr.value.type = ATTR_VALUE_PART;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("to_x");
   attr.value.type = ATTR_VALUE_PART;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("to_y");
   attr.value.type = ATTR_VALUE_PART;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   //Type: State
   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("STATE_SET");
   attr.value.type = ATTR_VALUE_STATE;
   attr.value.prepend_str = ATTR_PREPEND_NONE;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   attr.value.program = EINA_TRUE;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("inherit");
   attr.value.type = ATTR_VALUE_STATE;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   //Type: Image
   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("normal");
   attr.value.type = ATTR_VALUE_IMAGE;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("tween");
   attr.value.type = ATTR_VALUE_IMAGE;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);

   //Type: Program
   memset(&attr, 0x00, sizeof(parser_attr));
   attr.keyword = eina_stringshare_add("after");
   attr.value.type = ATTR_VALUE_PROGRAM;
   attr.value.prepend_str = ATTR_PREPEND_COLON;
   attr.value.append_str = ATTR_APPEND_SEMICOLON;
   eina_inarray_push(td->attrs, &attr);
}

static void
type_init_thread_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   type_init_td *td = data;
   if (td->pd)
     {
        td->pd->titd = NULL;
        td->pd->attrs = td->attrs;
     }
   free(td);
}

static void
type_init_thread_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   type_init_td *td = data;
   if (td->pd) td->pd->titd = NULL;
   free(td);
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
   if (!group_name) return NULL;

   const char* GROUP_SYNTAX_NAME = "group";
   const char *quot = QUOT_UTF8;
   const int quot_len = QUOT_UTF8_LEN;

   const char *pos = strstr(source, GROUP_SYNTAX_NAME);
   int group_name_len = strlen(group_name);

   //TODO: Process comments and quotes.
   while (pos)
   {
      const char *name = strstr(pos, quot);
      if (!name) return NULL;
      name += quot_len;
      pos = strstr(name, quot);
      if (!pos) return NULL;
      if (!strncmp(name, group_name, group_name_len))
        return pos;
      pos = strstr(++pos,  GROUP_SYNTAX_NAME);
   }

   return NULL;
}

static Eina_Bool
parser_collections_block_pos_get(const Evas_Object *entry,
                                 const char *block_name, int *ret)
{
   if (!ret) return EINA_FALSE;

   const char* GROUP_SYNTAX_NAME = "group";
   const int BLOCK_NAME_LEN = strlen(block_name);
   *ret = -1;

   const char *text = elm_entry_entry_get(entry);
   if (!text) return EINA_FALSE;

   char *utf8 = elm_entry_markup_to_utf8(text);
   if (!utf8) return EINA_FALSE;

   int cur_cursor = elm_entry_cursor_pos_get(entry);
   const char *pos = utf8 + cur_cursor;

   int len = strlen(utf8);

   /*
    * The next loop processing the text of block "group"
    * from actual cursor postion up to the block name or
    * the "group" position.
    * Returned value for the cases when the position
    * found correctly will be the first symbol of the next line.
    *
    * TODO and FIXME: possible wrong behaviour when before the
    * "group" keyword will be found part with name like "blah.group".
    */

   while (pos && (pos > utf8))
     {
        int block_pos = strncmp(block_name, pos, BLOCK_NAME_LEN);
        if (block_pos == 0)
          {
             const char *block = pos + BLOCK_NAME_LEN;
             while (block && (block < utf8 + len))
               {
                  if (*block == '.')
                    {
                       block = strchr(block, '\n');
                       *ret = block - utf8 + 1;
                       free(utf8);
                       return EINA_FALSE;
                    }
                  else if (*block == '{')
                    {
                       block = strchr(block, '\n');
                       *ret = block - utf8 + 1;
                       free(utf8);
                       return EINA_TRUE;
                    }
                  block++;
               }
             free(utf8);
             return EINA_FALSE;
          }
        int group_pos = strncmp(GROUP_SYNTAX_NAME, pos, 5);
        if (group_pos == 0)
          {
             const char *group_block = pos + 5;
             while (group_block && (group_block < utf8 + len))
               {
                  if (*group_block == '{')
                    {
                       group_block = strchr(group_block, '\n');
                       *ret = group_block - utf8 + 1;
                       free(utf8);
                       return EINA_FALSE;
                    }
                  group_block++;
               }
             free(utf8);
             return EINA_FALSE;
          }
        pos--;
      }
   free(utf8);
   return EINA_FALSE;
}

static void
bracket_thread_blocking(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   bracket_td *btd = data;
   int left_bracket = -1;
   int right_bracket = -1;
   int cur_pos = btd->pos;
   int depth = 0;
   const char *utf8 = btd->text;

   if (cur_pos == 0) return;

   int length = strlen(utf8);

   // left,  {
   if (utf8[cur_pos] == '{')
     {
        left_bracket = cur_pos;
        cur_pos++;
        while (cur_pos < length)
          {
             if (utf8[cur_pos] == '{') depth++;
             else if (utf8[cur_pos] == '}')
               {
                  if (depth) depth--;
                  else
                    {
                       right_bracket = cur_pos;
                       break;
                    }
               }
             cur_pos++;
          }
     }
   // left,  }
   else if(utf8[cur_pos] == '}')
     {
        right_bracket = cur_pos;
        cur_pos--;
        while (cur_pos)
          {
             if (utf8[cur_pos] == '}') depth++;
             else if(utf8[cur_pos] == '{')
               {
                  if(depth) depth--;
                  else
                    {
                       left_bracket = cur_pos;
                       break;
                    }
               }
             cur_pos--;
          }
     }
   // right, {
   else if(utf8[cur_pos - 1] == '{')
     {
        left_bracket = cur_pos - 1;
        while (cur_pos < length)
          {
             if (utf8[cur_pos] == '{') depth++;
             else if (utf8[cur_pos] == '}')
               {
                  if (depth) depth--;
                  else
                    {
                       right_bracket = cur_pos;
                       break;
                    }
               }
             cur_pos++;
          }
     }
   // right, }
   else if(utf8[cur_pos - 1] == '}')
     {
        right_bracket = cur_pos - 1;
        cur_pos -= 2;
        while (cur_pos)
          {
             if (utf8[cur_pos] == '}') depth++;
             else if (utf8[cur_pos] == '{')
               {
                  if(depth) depth--;
                  else
                    {
                       left_bracket = cur_pos;
                       break;
                    }
               }
             cur_pos--;
          }
     }

   if (left_bracket == -1 || right_bracket == -1)
     {
        left_bracket = -1;
        right_bracket = -1;
     }

   btd->left = left_bracket;
   btd->right = right_bracket;
}

static void
bracket_thread_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   bracket_td *btd = data;
   if (btd->pd) btd->pd->btd = NULL;
   free(btd->text);
   free(btd);
}

static void
bracket_thread_end(void *data, Ecore_Thread *thread)
{
   bracket_td *btd = data;
   btd->update_cb(btd->data, btd->left, btd->right);
   bracket_thread_cancel(data, thread);
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
parser_cancel(parser_data *pd)
{
   if (pd->cntd)
     {
        pd->cntd->pd = NULL;
        ecore_thread_cancel(pd->cntd->thread);
     }
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

   //Don't find if the name is in the next lines.
   char *eol = strstr(cur, "\n");
   if (eol && (eol < p)) return NULL;

   return eina_strndup(p, (end - p));
}

attr_value *
parser_attribute_get(parser_data *pd, const char *text, const char *cur,
                     const char *selected)
{
   if (!text || !cur) return NULL;
   if ((*cur == ';') || (*cur == ':')) return NULL;

   parser_attr *attr;
   Eina_Bool instring = EINA_FALSE;

   char *p = (char *) cur;

   while (p >= text)
     {
        if (*p == '\"') instring = !instring;
        p--;
     }
   if (instring) return NULL;

   int name_count;
   const char **cur_context = autocomp_current_context_get(&name_count);
   int i;

   EINA_INARRAY_FOREACH(pd->attrs, attr)
     {
        if (strcmp(selected, attr->keyword)) continue;
        if (!attr->context) return &attr->value;
        if (!cur_context) continue;
        for (i = 0; i < name_count; i++)
          {
             if (!strcmp(cur_context[i], attr->context))
               return &attr->value;
          }
     }

   return NULL;
}

/* Function is_numberic is refer to the following url.
   http://rosettacode.org/wiki/Determine_if_a_string_is_numeric#C */
static Eina_Bool
is_numberic(const char *str)
{
   Eina_Bool ret = EINA_FALSE;
   char *p;

   if (!str || (*str == '\0') || isspace(*str))
     return EINA_FALSE;

   double v EINA_UNUSED  = strtod(str, &p);

   if (*p == '\0') ret = EINA_TRUE;

   return ret;
}

void
parser_attribute_value_set(attr_value *value, char *cur)
{
   const char token[4] = " ;:";
   char *str = strtok(cur, token);
   int i;

   if (!str) return;
   str = strtok(NULL, token); //Skip the keyword

   //Initialize attribute values
   for (i = 0; i < value->cnt; i++)
     value->val[i] = 0;

   for (i = 0; str && (i < value->cnt); str = strtok(NULL, token))
     {
        if (!is_numberic(str)) continue;

        value->val[i] = atof(str);
        i++;
     }
}

Eina_Stringshare *
parser_paragh_name_get(parser_data *pd EINA_UNUSED, Evas_Object *entry)
{
   //FIXME: list up groups
#define GROUP_CNT 20
   typedef struct _group_info
   {
      char *str;
      int len;
   } group_info;

   group_info group_list[GROUP_CNT] =
     {
        { "collections", 11 },
        { "description", 11 },
        { "desc", 4 },
        { "fill", 4 },
        { "group", 5 },
        { "images", 6 },
        { "image", 5 },
        { "map", 3 },
        { "origin", 6 },
        { "parts", 5 },
        { "part", 4 },
        { "programs", 8 },
        { "program", 7 },
        { "rect", 4 },
        { "rel1", 4 },
        { "rel2", 4 },
        { "spacer", 6 },
        { "swallow", 7 },
        { "textblock", 9 },
        { "text", 4 }
     };

   const char *text = elm_entry_entry_get(entry);
   if (!text) return NULL;

   char *utf8 = elm_entry_markup_to_utf8(text);
   if (!utf8) return NULL;

   int cur_pos = elm_entry_cursor_pos_get(entry);
   if (cur_pos < 1) return NULL;

   const char *quot = QUOT_UTF8;
   int quot_len = QUOT_UTF8_LEN;
   char *cur = utf8;
   char *end = cur + cur_pos;
   char *stack[20];
   int depth = 0;

   //1. Figure out depth.
   while (cur <= end)
     {
        //Skip "" range
        if (!strncmp(cur, quot, quot_len))
          {
             cur += quot_len;
             cur = strstr(cur, quot);
             if (!cur)
               {
                  free(utf8);
                  return NULL;
               }
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

   if (depth == 0)
     {
        free(utf8);
        return NULL;
     }

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
               {
                  free(utf8);
                  return eina_stringshare_add_length(gi->str, gi->len);
               }
          }
     }

   free(utf8);
   return NULL;
}

Eina_Stringshare*
parser_cur_context_fast_get(Evas_Object *entry, const char *scope)
{
   const char *quot = QUOT_UTF8;
   const int quot_len = QUOT_UTF8_LEN;
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
        if (!strncmp(p, quot, quot_len))
          {
             p += quot_len;
             p = strstr(p, quot);
             if (!p) goto end;
             p += quot_len;
             continue;
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
parser_cur_context_get(parser_data *pd, Evas_Object *entry,
                       void (*cb)(void *data, Eina_Stringshare *state_name,
                       double state_value, Eina_Stringshare *part_name,
                       Eina_Stringshare *group_name), void *data,
                       Eina_Bool collections)
{
   if (pd->cntd)
     {
        pd->cntd->pd = NULL;
        ecore_thread_cancel(pd->cntd->thread);
     }

   const char *text = elm_entry_entry_get(entry);
   if (!text) return;

   char *utf8 = elm_entry_markup_to_utf8(text);
   if (!utf8) return;

   cur_context_td *td = calloc(1, sizeof(cur_context_td));
   if (!td)
     {
        free(utf8);
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }

   td->pd = pd;
   pd->cntd = td;
   td->utf8 = utf8;
   td->cur_pos = elm_entry_cursor_pos_get(entry);
   td->cb = cb;
   td->cb_data = data;
   td->collections = collections;

   td->thread = ecore_thread_run(cur_context_thread_blocking,
                                 cur_context_thread_end,
                                 cur_context_thread_cancel,
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
   const char *markup = elm_entry_entry_get(entry);
   char *utf8 = elm_entry_markup_to_utf8(markup);
   int utf8_len = strlen(utf8);
   char *p = utf8;

   const char *quot = QUOT_UTF8;
   const char *group = "group";
   const int quot_len = QUOT_UTF8_LEN;
   const int group_len = 5; //strlen("group");
   const char *group_name = NULL;

   while (p < (utf8 + utf8_len))
     {
        //Skip "" range
        if (!strncmp(p, quot, quot_len))
          {
             p += quot_len;
             p = strstr(p, quot);
             if (!p) goto end;
             p += quot_len;
             continue;
          }

        //Skip comments: /* ~ */
        if ((*p == '/') && (*(++p) == '*'))
          {
             p = strstr(p, "*/");
             if (!p) goto end;
             p += 2;
             continue;
          }

        //Skip comments: //
        if ((*p == '/') && (*(++p) == '/'))
          {
             p = strstr(p, "\n");
             if (!p) goto end;
             p++;
             continue;
          }

        //Skip #if ~ #endif
        if (!strncmp(p, "#if", 3))
          {
             p = strstr(p, "#endif");
             if (!p) goto end;
             p += 6; //strlen(#endif)
             continue;
          }

        //Skip #define
        if (!strncmp(p, "#define", 7))
          {
             //escape "\", "ie, #define .... \"
             p += 7; //strlen(#define)

             while (p < (utf8 + utf8_len))
               {
                  char *eol = strstr(p, "\n");
                  if (!eol) goto end;

                  char *slash = strstr(p, "\\");

                  p = eol + 1;

                  if (!slash || (eol < slash))
                    break;
               }
             continue;
          }

        //group?
       if (!strncmp(p, group, group_len))
          {
             p += group_len;
             char *name_begin = strstr(p, quot);
             if (!name_begin) goto end;
             name_begin += quot_len;
             p = name_begin;
             char *name_end = strstr(p, quot);
             if (!name_end) goto end;

             group_name = eina_stringshare_add_length(name_begin,
                                                      (name_end - name_begin));
             goto end;
          }
        p++;
     }

end:
   free(utf8);

   return group_name;
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
        ret = eina_list_append(ret, eina_strndup(state, pp - state));
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
   pd->titd = td;
   td->thread = ecore_thread_run(type_init_thread_blocking,
                                 type_init_thread_end,
                                 type_init_thread_cancel, td);
   return pd;
}

void
parser_term(parser_data *pd)
{
   if (pd->cntd)
     {
        ecore_thread_cancel(pd->cntd->thread);
        pd->cntd->pd = NULL;
     }
   if (pd->titd)
     {
        ecore_thread_cancel(pd->titd->thread);
        pd->titd->pd = NULL;
     }
   if (pd->btd)
     {
        ecore_thread_cancel(pd->btd->thread);
        pd->btd->pd = NULL;
     }

   if (pd->attrs)
     {
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
     }

   free(pd);
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
   return parser_collections_block_pos_get(entry, "images", ret);
}

Eina_Bool
parser_styles_pos_get(const Evas_Object *entry, int *ret)
{
   return parser_collections_block_pos_get(entry, "styles", ret);
}

Eina_Bool
parser_is_image_name(const Evas_Object *entry, const char *str)
{
   int start_pos, end_pos = -1, i;
   if (!parser_collections_block_pos_get(entry, "images", &start_pos))
     return EINA_FALSE;

   const char *text = elm_entry_entry_get(entry);
   char *utf8 = elm_entry_markup_to_utf8(text);
   int utf8_len = strlen(utf8);

   for (i = start_pos ; i < utf8_len; i++)
      if (utf8[i] == '}')
        {
           end_pos = i;
           break;
        }

   if (end_pos < 0) return EINA_FALSE;

   char *candidate_str = alloca(end_pos - start_pos + 1);
   const char *src_str = elm_entry_markup_to_utf8(str);
   strncpy(candidate_str, utf8 + start_pos, end_pos - start_pos);
   candidate_str[end_pos - start_pos] = '\0';

   if (strstr(candidate_str, src_str))
     return EINA_TRUE;
   else
     return EINA_FALSE;
}

void
parser_bracket_cancel(parser_data *pd)
{
   if (pd->btd)
     {
        ecore_thread_cancel(pd->btd->thread);
        pd->btd->pd = NULL;
        pd->btd = NULL;
     }
}

void
parser_bracket_find(parser_data *pd, Evas_Object *entry,
                    Bracket_Update_Cb func, void *data)
{
   if (pd->btd)
     {
        pd->btd->pd = NULL;
        ecore_thread_cancel(pd->btd->thread);
     }

   const char *text = elm_entry_entry_get(entry);
   char *utf8 = elm_entry_markup_to_utf8(text);
   int pos = elm_entry_cursor_pos_get(entry);

   bracket_td *btd = malloc(sizeof(bracket_td));
   pd->btd = btd;

   btd->pos = pos;
   btd->text = utf8;
   btd->update_cb = func;
   btd->data = data;
   btd->pd = pd;
   btd->thread = ecore_thread_run(bracket_thread_blocking,
                                  bracket_thread_end,
                                  bracket_thread_cancel,
                                  btd);
}

Eina_List *
parser_group_list_get(parser_data *pd EINA_UNUSED, Evas_Object *entry)
{
   const char *markup = elm_entry_entry_get(entry);
   char *utf8 = elm_entry_markup_to_utf8(markup);
   int utf8_len = strlen(utf8);
   char *p = utf8;

   const char *quot = QUOT_UTF8;
   const char *group = "group";
   const int quot_len = QUOT_UTF8_LEN;
   const int group_len = 5; //strlen("group");
   const char *group_name = NULL;
   Eina_List *group_list = NULL;

   while (p < (utf8 + utf8_len))
     {
        //Skip "" range
        if (!strncmp(p, quot, quot_len))
          {
             p += quot_len;
             p = strstr(p, quot);
             if (!p) goto end;
             p += quot_len;
             continue;
          }

        //Skip comments: /* ~ */
        if ((*p == '/') && (*(++p) == '*'))
          {
             p = strstr(p, "*/");
             if (!p) goto end;
             p += 2;
             continue;
          }

        //Skip comments: //
        if ((*p == '/') && (*(++p) == '/'))
          {
             p = strstr(p, "\n");
             if (!p) goto end;
             p++;
             continue;
          }

        //Skip #if ~ #endif
        if (!strncmp(p, "#if", 3))
          {
             p = strstr(p, "#endif");
             if (!p) goto end;
             p += 6; //strlen(#endif)
             continue;
          }

        //Skip #define
        if (!strncmp(p, "#define", 7))
          {
             //escape "\", "ie, #define .... \"
             p += 7; //strlen(#define)

             while (p < (utf8 + utf8_len))
               {
                  char *eol = strstr(p, "\n");
                  if (!eol) goto end;

                  char *slash = strstr(p, "\\");

                  p = eol + 1;

                  if (!slash || (eol < slash))
                    break;
               }
             continue;
          }

        //group?
       if (!strncmp(p, group, group_len))
         {
            p += group_len;

            if (((*p != ' ') && (*p != '{') && (*p != '\n') && (*p != '\t')))
              continue;

            p++;

            //We found a group
            p = strstr(p, quot);
            if (!p) goto end;

            p++;
            char *name_begin = p;

            p = strstr(p, quot);
            if (!p) goto end;

            char *name_end = p;

            group_name = eina_stringshare_add_length(name_begin,
                                                     name_end - name_begin);
            if (group_name)
              group_list = eina_list_append(group_list, group_name);
         }
       p++;
     }

end:
   free(utf8);

   return group_list;
}

char*
parser_group_pos_get(char *utf8, char *utf8_end,
                     const char *group_name)
{
   char *p = utf8;
   char *result = NULL;

   int group_name_len = strlen(group_name);

   //Find group
   while (utf8_end > p)
     {
        //Skip " ~ " Section
        if (*p == '\"')
          {
             ESCAPE_RET_NULL();
          }

        if (!strncmp("group", p, strlen("group")))
          {
             p = strstr((p + 5), "\"");
             if (!p) return NULL;
             p++;
             if (!strncmp(group_name, p, group_name_len))
               {
                  //Compare Elaborately
                  char *next_quote = strstr(p, "\"");
                  if (group_name_len == (next_quote - p))
                    {
                       result = p;
                       break;
                    }
                  else
                    {
                       ESCAPE_RET_NULL();
                    }
               }
             else
               {
                  ESCAPE_RET_NULL();
               }
          }
        p++;
     }

   return result;
}

char*
parser_part_pos_get(char *utf8, char *utf8_end,
                    const char* group_name,
                    const char *part_name, const char *part_type)
{
   char *p = parser_group_pos_get(utf8, utf8_end, group_name);

   //No found
   if (!p) return NULL;

   p = strstr(p, "\"");
   if (!p) return NULL;
   p++;

   char *result = NULL;

   //goto parts
   p = strstr(p, "parts");
   if (!p) return NULL;

   int part_name_len = strlen(part_name);

   //Find part
   while (utf8_end > p)
     {
        //Skip " ~ " Section
        if (*p == '\"')
          {
             ESCAPE_RET_NULL();
          }

        if (!strncmp(part_type, p, strlen(part_type)))
          {
             p = strstr((p + strlen(part_type)), "\"");
             if (!p) return NULL;
             p++;
             if (!strncmp(part_name, p, part_name_len))
               {
                  //Compare Elaborately
                  char *next_quote = strstr(p, "\"");
                  if (part_name_len == (next_quote - p))
                    {
                       result = p;
                       break;
                    }
                  else
                    {
                       ESCAPE_RET_NULL();
                    }
               }
             else
               {
                  ESCAPE_RET_NULL();
               }
          }

        //compatibility: "part"
        if (!strncmp("part", p, strlen("part")))
          {
             p = strstr((p + 4), "\"");
             if (!p) return NULL;
             p++;
             if (!strncmp(part_name, p, strlen(part_name)))
               {
                  //Compare Elaborately
                  char *next_quote = strstr(p, "\"");
                  if (part_name_len == (next_quote - p))
                    {
                       result = p;
                       break;
                    }
                  else
                    {
                       ESCAPE_RET_NULL();
                    }
               }
             else
               {
                  ESCAPE_RET_NULL();
               }
          }

        p++;
     }

   return result;
}
