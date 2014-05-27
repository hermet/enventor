#include <Elementary.h>
#include "common.h"

#define COL_NUM 6

typedef struct color_tuple
{
   Eina_Stringshare *key;
   Eina_Stringshare *col;
} color_tuple;

typedef struct color
{
   char *val;
   Eina_List *keys;
} color;

typedef struct syntax_color_group
{
   char *string;
   char *comment;
   char *macro;
   color colors[COL_NUM];
} syntax_color_group;

struct syntax_color_s
{
   Eina_Strbuf *strbuf;
   Eina_Strbuf *cachebuf;
   Eina_Hash *color_hash;
   Eina_Stringshare *col_string;
   Eina_Stringshare *col_comment;
   Eina_Stringshare *col_macro;
   Eina_Stringshare *cols[COL_NUM];
   Eina_List *macros;
   Ecore_Thread *thread;

   Eina_Bool ready: 1;
};

static Eet_Data_Descriptor *edd_scg = NULL;
static Eet_Data_Descriptor *edd_color = NULL;
static syntax_color_group *scg = NULL;

static void
hash_free_cb(void *data)
{
   Eina_Inarray *inarray = data;
   color_tuple *tuple;
   EINA_INARRAY_FOREACH(inarray, tuple)
     eina_stringshare_del(tuple->key);
   eina_inarray_free(inarray);
}

static void
eddc_init()
{
   Eet_Data_Descriptor_Class eddc;
   eet_eina_stream_data_descriptor_class_set(&eddc, sizeof(eddc),
                                             "syntax_color_group",
                                             sizeof(syntax_color_group));
   edd_scg = eet_data_descriptor_stream_new(&eddc);

   eet_eina_stream_data_descriptor_class_set(&eddc, sizeof(eddc), "color",
                                             sizeof(color));
   edd_color = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_scg, syntax_color_group, "string",
                                 string, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_scg, syntax_color_group, "comment",
                                 comment, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_scg, syntax_color_group, "macro",
                                 macro, EET_T_STRING);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_color, color, "val", val, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(edd_color, color, "keys", keys);

   EET_DATA_DESCRIPTOR_ADD_ARRAY(edd_scg, syntax_color_group, "colors",
                                 colors, edd_color);
}

static void
eddc_term()
{
   eet_data_descriptor_free(edd_scg);
   eet_data_descriptor_free(edd_color);
}

static void
color_load(color_data *cd)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/color/color.eet", elm_app_data_dir_get());

   Eet_File *ef = eet_open(buf, EET_FILE_MODE_READ);
   if (ef)
     {
        scg = eet_data_read(ef, edd_scg, "color");
        if (!scg) EINA_LOG_ERR("Failed to read syntax color group.");
        eet_close(ef);
     }
   else EINA_LOG_ERR("Failed to open color data file \"%s\"", buf);
}

static void
color_table_init(color_data *cd)
{
   color_tuple *tuple;
   int i;
   Eina_List *l;
   char *key;
   char tmp[2];
   Eina_Inarray *inarray;

   if (!scg) return;

   cd->col_string = eina_stringshare_add(scg->string);
   //free(scg->string);
   cd->col_comment = eina_stringshare_add(scg->comment);
   //free(scg->comment);
   cd->col_macro = eina_stringshare_add(scg->macro);
   //free(scg->macro);

   cd->color_hash = eina_hash_string_small_new(hash_free_cb);

   for (i = 0; i < COL_NUM; i++)
     {
        cd->cols[i] = eina_stringshare_add(scg->colors[i].val);
        //free(scg->colors[i].val);

        EINA_LIST_FOREACH(scg->colors[i].keys, l, key)
          {
             tmp[0] = key[0];
             tmp[1] = '\0';

             inarray = eina_hash_find(cd->color_hash, tmp);
             if (!inarray)
               {
                  inarray = eina_inarray_new(sizeof(color_tuple), 20);
                  eina_hash_add(cd->color_hash, tmp, inarray);
               }

             tuple = malloc(sizeof(color_tuple));
             tuple->col = cd->cols[i];
             tuple->key = eina_stringshare_add(key);
             //free(key);
             eina_inarray_push(inarray, tuple);
          }
        eina_list_free(scg->colors[i].keys);
     }

   free(scg);
   scg = NULL;
}

static void
macro_key_push(color_data *cd, char *str, int len)
{
   char *key = str;

   //cutoff "()" from the macro name
   char *cut = strchr(key, '(');
   if (cut) key = strndup(str, cut - str);

   char tmp[2];
   tmp[0] = key[0];
   tmp[1] = '\0';

   Eina_Inarray *inarray = eina_hash_find(cd->color_hash, tmp);
   if (!inarray)
     {
        inarray = eina_inarray_new(sizeof(color_tuple), 20);
        eina_hash_add(cd->color_hash, tmp, inarray);
     }

   color_tuple *tuple = malloc(sizeof(color_tuple));
   tuple->col = cd->col_macro;
   tuple->key = eina_stringshare_add(key);
   eina_inarray_push(inarray, tuple);

   cd->macros = eina_list_append(cd->macros, eina_stringshare_add(tuple->key));

   if (cut) free(key);
}

static void
init_thread_blocking(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   color_data *cd = data;

   eddc_init();
   color_load(cd);
   eddc_term();
   color_table_init(cd);

   cd->thread = NULL;
   cd->ready = EINA_TRUE;
}

color_data *
color_init(Eina_Strbuf *strbuf)
{
   color_data *cd = malloc(sizeof(color_data));
   cd->strbuf = strbuf;
   cd->cachebuf = eina_strbuf_new();
   cd->thread = ecore_thread_run(init_thread_blocking, NULL, NULL, cd);
   cd->macros = NULL;

   return cd;
}

void
color_term(color_data *cd)
{
   ecore_thread_cancel(cd->thread);

   eina_hash_free(cd->color_hash);
   eina_strbuf_free(cd->cachebuf);

   eina_stringshare_del(cd->col_string);
   eina_stringshare_del(cd->col_comment);
   eina_stringshare_del(cd->col_macro);

   Eina_Stringshare *macro;
   EINA_LIST_FREE(cd->macros, macro) eina_stringshare_del(macro);

   int i;
   for(i = 0; i < COL_NUM; i++)
     eina_stringshare_del(cd->cols[i]);

   free(cd);
}

static Eina_Bool
color_markup_insert_internal(Eina_Strbuf *strbuf, const char **src, int length,
                             char **cur, char **prev,  const char *cmp,
                             Eina_Stringshare *color)
{
   char buf[128];

   eina_strbuf_append_length(strbuf, *prev, *cur - *prev);
   snprintf(buf, sizeof(buf), "<color=#%s>%s</color>", color, cmp);
   eina_strbuf_append(strbuf, buf);
   *cur += strlen(cmp);
   if (*cur > (*src + length)) return EINA_FALSE;
   *prev = *cur;

   return EINA_TRUE;
}

static int
markup_skip(Eina_Strbuf *strbuf, const char **src, int length, char **cur,
            char **prev)
{
   if ((*cur)[0] != '<') return 0;

   eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));
   (*cur)++;

   if (*cur > (*src + length)) return -1;
   *prev = *cur;

   *cur = strchr(*prev, '>');

   if (*cur)
     {
        (*cur)++;
        *prev = *cur;
        return 1;
     }
   else
     {
        *prev = *cur;
        return -1;
     }

   return 1;
}

static int
tab_skip(Eina_Strbuf *strbuf, const char **src, int length, char **cur,
        char **prev)
{
   if (strncmp(*cur, TAB, TAB_LEN)) return 0;
   eina_strbuf_append_length(strbuf, *prev, (*cur - *prev + TAB_LEN));
   *cur += TAB_LEN;
   if (*cur > (*src + length)) return -1;
   *prev = *cur;

   return 1;
}

static int
br_skip(Eina_Strbuf *strbuf, const char **src, int length, char **cur,
        char **prev)
{
   if (strncmp(*cur, EOL, EOL_LEN)) return 0;
   eina_strbuf_append_length(strbuf, *prev, (*cur - *prev + EOL_LEN));
   *cur += EOL_LEN;
   if (*cur > (*src + length)) return -1;
   *prev = *cur;

   return 1;
}

static int
comment_apply(Eina_Strbuf *strbuf, const char **src, int length, char **cur,
              char **prev, const Eina_Stringshare *color,
              Eina_Bool *inside_comment)
{
   if (!(*inside_comment))
     {
        if ((*cur)[0] != '/') return 0;
        if ((*cur) + 1 > ((*src) + length)) return -1;
        if ((*cur)[1] != '*') return 0;

        eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));

        char buf[128];
        snprintf(buf, sizeof(buf), "<color=#%s>/*", color);
        eina_strbuf_append(strbuf, buf);

        int cmp_size = 2;     //strlen("/*");

        *cur += cmp_size;

        if (*cur > (*src + length))
          {
             *inside_comment = EINA_TRUE;
             return -1;
          }

        *prev = *cur;

        *cur = strstr(*prev, "*/");

        if (*cur)
          {
             eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));
             eina_strbuf_append(strbuf, "*/</color>");
             *cur += cmp_size;
             *prev = *cur;
             return 0;
          }

        eina_strbuf_append(strbuf, *prev);
        *prev = *cur;

        *inside_comment = EINA_TRUE;
        return -1;
     }
   else
     {
        if ((*cur)[0] != '*') return 0;
        if ((*cur) + 1 > ((*src) + length)) return -1;
        if ((*cur)[1] != '/') return 0;

        eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));
        eina_strbuf_append(strbuf, "*/</color>");

        int cmp_size = 2;     //strlen("*/");

        *cur += cmp_size;
        *inside_comment = EINA_FALSE;

        if (*cur > (*src + length)) return -1;
        *prev = *cur;
        return 1;
     }

   return -1;
}

static int
comment2_apply(Eina_Strbuf *strbuf, const char **src, int length, char **cur,
               char **prev, const Eina_Stringshare *color,
               Eina_Bool *inside_comment)
{
   if (*inside_comment) return 0;
   if ((*cur)[0] != '/') return 0;
   if (((*cur) + 1) > ((*src) + length)) return -1;
   if ((*cur)[1] != '/') return 0;

   eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));

   char buf[128];
   snprintf(buf, sizeof(buf), "<color=#%s>//", color);
   eina_strbuf_append(strbuf, buf);

   int cmp_size = 2;    //strlen("//");
   *cur += cmp_size;

   if (*cur > (*src + length))
     {
        eina_strbuf_append(strbuf, "</color>");
        return -1;
     }

   *prev = *cur;

   *cur = strstr(*prev, EOL);

   if (*cur)
     {
        eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));
        eina_strbuf_append(strbuf, "</color><br/>");
        *cur += EOL_LEN;
        *prev = *cur;
        return 1;
     }

   eina_strbuf_append(strbuf, *prev);
   *prev = *cur;

   eina_strbuf_append(strbuf, "</color>");
   return -1;
}

static int
string_apply(Eina_Strbuf *strbuf, char **cur, char **prev,
             const Eina_Stringshare *color, Eina_Bool inside_string)
{
   //escape string: " ~ "
   if ((*cur)[0] != QUOT_C) return 0;

   char buf[128];

   if (!inside_string)
     {
        snprintf(buf, sizeof(buf), "<color=#%s>", color);
        eina_strbuf_append(strbuf, buf);
     }

   eina_strbuf_append_length(strbuf, *prev, (*cur - *prev) + 1);

   if (inside_string)
     {
        snprintf(buf, sizeof(buf), "</color>");
        eina_strbuf_append(strbuf, buf);
     }

   (*cur)++;
   *prev = *cur;

   return 1;
}

static int
macro_apply(Eina_Strbuf *strbuf, const char **src, int length, char **cur,
            char **prev, const Eina_Stringshare *color, color_data *cd)
{
   if ((*cur)[0] != '#') return 0;

   char *space = strchr(*cur, ' ');
   const char *eol = strstr(*cur, EOL);

   if (!eol) eol = (*src) + length;
   if (!space) space = (char *) eol;

   //Let's find the macro name
   while ((*space == ' ') && (space != eol)) space++;
   char *macro_begin = space;
   char *macro_end = strchr(space, ' ');

   //Excetional case 1
   if (!macro_end) macro_end = (char *) eol;
   //Exceptional case 2
   else if (macro_end > eol) macro_end = (char *) eol;
   //Let's check the macro function case
   else
   {
     int macro_len = macro_end - macro_begin;
     char *macro = alloca(macro_len);
     strncpy(macro, macro_begin, macro_len);

     //Check how many "(", ")" pairs are exists
     int bracket_inside = 0;
     while (macro_len >= 0)
       {
          if (macro[macro_len] == '(') bracket_inside++;
          else if (macro[macro_len] == ')') bracket_inside--;
          macro_len--;
       }
     if (bracket_inside > 0)
       {
          while (bracket_inside > 0)
            {
               macro_end = strchr(macro_end, ')');
               bracket_inside--;
            }
          if (!macro_end) macro_end = (char *) eol;
          else if (macro_end > eol) macro_end = (char *) eol;
          else macro_end++;
       }
   }

   //#define, #ifdef, #if, #...
   eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));

   char buf[128];
   snprintf(buf, sizeof(buf), "<color=#%s>#", color);
   eina_strbuf_append(strbuf, buf);

   int cmp_size = 1;    //strlen("#");
   *cur += cmp_size;

   *prev = *cur;
   *cur = macro_end;

   eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));
   eina_strbuf_append(strbuf, "</color>");

   //push the macro to color table but only not numeric case.
   if ((macro_end > macro_begin) &&
       ((macro_begin[0] < '0') || (macro_begin[0] > '9')))
     {
        char *macro = strndup(macro_begin, (macro_end - macro_begin));
        macro_key_push(cd, macro, (macro_end - macro_begin));
        free(macro);
     }

   *prev = *cur;

   return 1;

nospace:
   (*cur)++;
   eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));
   *prev = *cur;
   return 1;
}

const char *
color_cancel(color_data *cd, const char *src, int length, int from_pos,
             int to_pos, char **from, char **to)
{
   if (!src || length < 1) return NULL;
   Eina_Strbuf *strbuf = cd->strbuf;
   eina_strbuf_reset(strbuf);

   const char *str = NULL;
   char *prev = (char *) src;
   char *cur = (char *) src;
   int line = 1;
   Eina_Bool find_from, find_to;

   //if the from_pos equals -1, we wanna full text area of syntax color
   if (from_pos == -1)
     {
        find_from = EINA_FALSE;
        find_to = EINA_FALSE;
     }
   else
     {
        find_from = EINA_TRUE;
        find_to = EINA_TRUE;
     }

   while (cur && (cur <= (src + length)))
     {
        if (find_from && (line == from_pos))
          {
             from_pos = eina_strbuf_length_get(strbuf);
             find_from = EINA_FALSE;
          }
        if (find_to && (line == to_pos))
          {
             to_pos = eina_strbuf_length_get(strbuf);
             find_to = EINA_FALSE;
          }

        //escape EOL: <br/>
        if (br_skip(strbuf, &src, length, &cur, &prev) == 1)
        {
           line++;
           continue;
        }

        //escape TAB: <tab/>
        if (tab_skip(strbuf, &src, length, &cur, &prev) == 1)
          continue;

        //escape markups: <..> ~ </..> 
        if (markup_skip(strbuf, &src, length, &cur, &prev) == 1)
          continue;

        cur++;
     }

   //Same with origin source.
   if (prev == src)
     str = src;
   //Some color syntax is applied.
   else
     {
        //append leftovers.
        if (prev + 1 < cur) eina_strbuf_append(strbuf, prev);
        str = eina_strbuf_string_get(strbuf);
     }

   //Exceptional Handling
   if (find_from) from_pos = 0;
   if (find_to) to_pos = eina_strbuf_length_get(strbuf);

   if (from_pos != -1)
     {
        *from = ((char *) str) + from_pos;
        *to = ((char *) str) + to_pos;
     }

   return str;
}

static void
macro_keys_free(color_data *cd)
{
   Eina_Stringshare *macro;
   Eina_Inarray *inarray;
   color_tuple *tuple;
   char key[2];

   EINA_LIST_FREE(cd->macros, macro)
     {
        key[0] = macro[0];
        key[1] = '\0';
        inarray = eina_hash_find(cd->color_hash, key);

        if (inarray)
          {
             EINA_INARRAY_REVERSE_FOREACH(inarray, tuple)
               {
                  if (strlen(macro) != strlen(tuple->key)) continue;
                  if (!strcmp(macro, tuple->key))
                    {
                       eina_inarray_pop(inarray);
                       break;
                    }
               }
          }
     }
}

static int
color_markup_insert(Eina_Strbuf *strbuf, const char **src, int length, char **cur,
                    char **prev, color_data *cd)
{
   const char *SYMBOLS = " {}[];:.()!<>=&|";
   Eina_Bool symbol = EINA_FALSE;

   if (strchr(SYMBOLS, (*cur)[0])) symbol = EINA_TRUE;

   if (!symbol && (*cur > *src))
     {
        if (!strchr(SYMBOLS, *(*cur -1))) return 0;
     }

   char tmp[2];
   tmp[0] = (*cur)[0];
   tmp[1] = '\0';

   Eina_Inarray *inarray = eina_hash_find(cd->color_hash, tmp);
   if (!inarray) return 0;

   //Found tuple list. Search in detail.
   color_tuple *tuple;
   int len;

   EINA_INARRAY_FOREACH(inarray, tuple)
     {
        len = strlen(tuple->key);
        char *p = *cur + len;
        if (!strncmp(*cur, tuple->key, len))
          {
             if (p <= (*src + length))
               {
                  if (!symbol &&
                      /* Exceptional Case. For duplicated keywords, it
                         subdivides with '.' ' '. See the config.src */
                      (*(p - 1) != '.') &&
                      (*(p - 1) != ' '))
                    {
                       if (!strchr(SYMBOLS, *p)) return 0;
                    }
                  if (color_markup_insert_internal(strbuf, src, length, cur,
                                                   prev, tuple->key,
                                                   tuple->col))
                    return 1;
                  else return -1;
               }
          }
     }
   return 0;
}

const char *
color_apply(color_data *cd, const char *src, int length, char *from, char *to)
{
   Eina_Bool inside_string = EINA_FALSE;
   Eina_Bool inside_comment = EINA_FALSE;

   if (!src || (length < 1)) return NULL;

   Eina_Strbuf *strbuf = cd->cachebuf;
   eina_strbuf_reset(strbuf);

   const char *str = NULL;
   char *prev = (char *) src;
   char *cur = (char *) src;
   int ret;

   while (cur && (cur <= (src + length)))
     {
        //escape empty string
        if (!from || (cur >= from))
          {
             if (cur[0] == ' ')
               {
                  if (cur > prev)
                    eina_strbuf_append_length(strbuf, prev, (cur - prev) + 1);
                  else
                    eina_strbuf_append_char(strbuf, ' ');
                  ++cur;
                  prev = cur;
                  continue;
               }
          }

        //handle comment: /* ~ */
        ret = comment_apply(strbuf, &src, length, &cur, &prev, cd->col_comment,
                            &inside_comment);
        if (ret == 1) continue;
        else if (ret == -1) goto finished;

        //handle comment: //
        if (!from || (cur >= from))
          {
             ret = comment2_apply(strbuf, &src, length, &cur, &prev,
                                  cd->col_comment, &inside_comment);
             if (ret == 1) continue;
             else if (ret == -1) goto finished;
          }

        //escape string: " ~ "
        ret = string_apply(strbuf, &cur, &prev, cd->col_string, inside_string);
        if (ret == 1)
          {
             inside_string = !inside_string;
             continue;
          }

        if (inside_string || inside_comment)
          {
             cur++;
             continue;
          }

        //handle comment: preprocessors, #
        ret = macro_apply(strbuf, &src, length, &cur, &prev, cd->col_macro, cd);
        if (ret == 1) continue;

        //apply color markup
        if (!from || (cur >= from))
          {
             ret = color_markup_insert(strbuf, &src, length, &cur, &prev, cd);
             if (ret == 1) continue;
             else if (ret == -1) goto finished;
          }

        cur++;
        if (to && (cur > to)) goto finished;
     }

   //Same with origin source.
   if (prev == src)
     str = src;
   //Some color syntax is applied.
   else
     {
finished:
        //append leftovers.
        if (prev < cur) eina_strbuf_append(strbuf, prev);
        str = eina_strbuf_string_get(strbuf);
     }

   macro_keys_free(cd);

   return str;
}

Eina_Bool
color_ready(color_data *cd)
{
   return cd->ready;
}
