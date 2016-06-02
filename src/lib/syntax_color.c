#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <Enventor.h>
#include "enventor_private.h"

#define MAX_COL_NUM 6

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
   char *count;
   color colors[MAX_COL_NUM];
} syntax_color_group;

typedef struct syntax_color_source
{
   Eina_Hash *color_hash;
   Eina_Stringshare *col_string;
   Eina_Stringshare *col_comment;
   Eina_Stringshare *col_macro;
   Eina_Stringshare *cols[MAX_COL_NUM];
   int color_cnt;

} syntax_color_source;

struct syntax_color_s
{
   Eina_Strbuf *strbuf;
   Eina_Strbuf *cachebuf;
   Eina_List *macros;
   Ecore_Thread *thread;
   syntax_color_source *col_src;

   Eina_Bool ready: 1;
};

typedef struct color_hash_foreach_data
{
   Eina_Stringshare *cur_col;
   Eina_Stringshare *new_col;
} color_hash_foreach_data;

static Eet_Data_Descriptor *edd_scg = NULL;
static Eet_Data_Descriptor *edd_color = NULL;
static syntax_color_group *scg = NULL;

//We could share this color source through editor instances.
static syntax_color_source g_color_src;
static int init_count = 0;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

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
eddc_init(void)
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
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_scg, syntax_color_group, "count",
                                 count, EET_T_STRING);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_color, color, "val", val, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(edd_color, color, "keys", keys);

   EET_DATA_DESCRIPTOR_ADD_ARRAY(edd_scg, syntax_color_group, "colors",
                                 colors, edd_color);
}

static void
eddc_term(void)
{
   eet_data_descriptor_free(edd_scg);
   eet_data_descriptor_free(edd_color);
}

static void
color_load()
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/color/edc.eet", elm_app_data_dir_get());

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
color_table_init(void)
{
   color_tuple tuple;
   int i;
   Eina_List *l;
   char *key;
   char tmp[2];
   Eina_Inarray *inarray;

   if (!scg) return;
   syntax_color_source *color_src = &g_color_src;

   color_src->col_string = eina_stringshare_add(scg->string);
   //free(scg->string);
   color_src->col_comment = eina_stringshare_add(scg->comment);
   //free(scg->comment);
   color_src->col_macro = eina_stringshare_add(scg->macro);
   //free(scg->macro);
   color_src->color_cnt = atoi(scg->count);
   //free(scg->count);

   color_src->color_hash = eina_hash_string_small_new(hash_free_cb);

   for (i = 0; i < color_src->color_cnt; i++)
     {
        color_src->cols[i] = eina_stringshare_add(scg->colors[i].val);
        //free(scg->colors[i].val);

        EINA_LIST_FOREACH(scg->colors[i].keys, l, key)
          {
             tmp[0] = key[0];
             tmp[1] = '\0';

             inarray = eina_hash_find(color_src->color_hash, tmp);
             if (!inarray)
               {
                  inarray = eina_inarray_new(sizeof(color_tuple), 20);
                  eina_hash_add(color_src->color_hash, tmp, inarray);
               }

             tuple.col = color_src->cols[i];
             tuple.key = eina_stringshare_add(key);
             eina_inarray_push(inarray, &tuple);
          }
        eina_list_free(scg->colors[i].keys);
     }

   free(scg);
   scg = NULL;
}

static void
macro_key_push(color_data *cd, char *str)
{
   char *key = str;
   syntax_color_source *col_src = cd->col_src;

   //cutoff "()" from the macro name
   char *cut = strchr(key, '(');
   if (cut) key = strndup(str, cut - str);

   char tmp[2];
   tmp[0] = key[0];
   tmp[1] = '\0';

   Eina_Inarray *inarray = eina_hash_find(col_src->color_hash, tmp);
   if (!inarray)
     {
        inarray = eina_inarray_new(sizeof(color_tuple), 20);
        eina_hash_add(col_src->color_hash, tmp, inarray);
     }

   color_tuple tuple;
   tuple.col = col_src->col_macro;
   tuple.key = eina_stringshare_add(key);
   eina_inarray_push(inarray, &tuple);

   cd->macros = eina_list_append(cd->macros, eina_stringshare_add(tuple.key));

   if (cut) free(key);
}

static void
init_thread_blocking(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   color_data *cd = data;

   //Initialize color table once.
   if (init_count == 1)
     {
        eddc_init();
        color_load();
        eddc_term();
        color_table_init();
     }

   cd->col_src = &g_color_src;
   cd->thread = NULL;
   cd->ready = EINA_TRUE;
}

static Eina_Bool
color_markup_insert_internal(Eina_Strbuf *strbuf, const char **src, int length,
                             char **cur, char **prev,  const char *cmp,
                             Eina_Stringshare *col)
{
   char buf[128];

   eina_strbuf_append_length(strbuf, *prev, *cur - *prev);
   snprintf(buf, sizeof(buf), "<color=#%s>%s</color>", col, cmp);
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
comment_apply(Eina_Strbuf *strbuf, const char **src, int length, char **cur,
              char **prev, const Eina_Stringshare *col,
              Eina_Bool *inside_comment)
{
   if (!(*inside_comment))
     {
        if ((*cur)[0] != '/') return 0;
        if ((*cur) + 1 > ((*src) + length)) return -1;
        if ((*cur)[1] != '*') return 0;

        eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));

        char buf[128];
        snprintf(buf, sizeof(buf), "<color=#%s>/*", col);
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
               char **prev, const Eina_Stringshare *col,
               Eina_Bool *inside_comment)
{
   if (*inside_comment) return 0;
   if ((*cur)[0] != '/') return 0;
   if (((*cur) + 1) > ((*src) + length)) return -1;
   if ((*cur)[1] != '/') return 0;

   eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));

   char buf[128];
   snprintf(buf, sizeof(buf), "<color=#%s>//", col);
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
             const Eina_Stringshare *col, Eina_Bool inside_string)
{
   //escape string: " ~ "
   Eina_Bool is_eol = EINA_FALSE;
   if (inside_string && !strncmp(*cur, EOL, EOL_LEN)) is_eol = EINA_TRUE;
   else if (strncmp(*cur, QUOT, QUOT_LEN)) return 0;

   char buf[128];

   eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));

   // these conditions limit string range to end of line
   // case 1: this condition checks end of line for string
   if (is_eol)
     {
        snprintf(buf, sizeof(buf), "</color>");
        eina_strbuf_append(strbuf, buf);
     }
   // case 2: this condition checks start and end for string
   else
     {
        if (!inside_string)
          snprintf(buf, sizeof(buf), "<color=#%s>%s", col, QUOT);
        else
          snprintf(buf, sizeof(buf), "%s</color>", QUOT);
        eina_strbuf_append(strbuf, buf);
        *cur += QUOT_LEN;
     }

   *prev = *cur;

   return 1;
}

static int
macro_apply(Eina_Strbuf *strbuf, const char **src, int length, char **cur,
            char **prev, const Eina_Stringshare *col, color_data *cd)
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
   snprintf(buf, sizeof(buf), "<color=#%s>#", col);
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
        macro_key_push(cd, macro);
        free(macro);
     }

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
        //Capture start line
        if (find_from && (line == from_pos))
          {
             from_pos = eina_strbuf_length_get(strbuf);
             find_from = EINA_FALSE;
          }

        if (*cur == '<')
          {
             //escape EOL: <br/>
             if (!strncmp(cur, EOL, EOL_LEN))
               {
                  //Capture end line
                  if (find_to && (line == to_pos))
                    {
                       to_pos = eina_strbuf_length_get(strbuf);
                       find_to = EINA_FALSE;
                    }

                  eina_strbuf_append_length(strbuf, prev,
                                            (cur - prev + EOL_LEN));
                  cur += EOL_LEN;
                  prev = cur;
                  line++;

                  continue;
               }
             //escape TAB: <tab/>
             if (!strncmp(cur, TAB, TAB_LEN))
               {
                  cur += TAB_LEN;
                  continue;
               }
             //escape markups: <..> ~ </..>
             if (markup_skip(strbuf, &src, length, &cur, &prev) == 1)
               continue;
          }
        cur++;
     }

   //Capture end line
   if (find_to && (line == to_pos))
     {
        to_pos = eina_strbuf_length_get(strbuf);
        find_to = EINA_FALSE;
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
   syntax_color_source *col_src = cd->col_src;

   EINA_LIST_FREE(cd->macros, macro)
     {
        key[0] = macro[0];
        key[1] = '\0';
        inarray = eina_hash_find(col_src->color_hash, key);

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
   const char *SYMBOLS = " {}[];:.()!<>=&|/";
   Eina_Bool symbol = EINA_FALSE;

   if (strchr(SYMBOLS, (*cur)[0])) symbol = EINA_TRUE;

   if (!symbol && (*cur > *src))
     {
        if (!strchr(SYMBOLS, *(*cur -1))) return 0;
     }

   syntax_color_source *col_src = cd->col_src;
   char tmp[2];
   tmp[0] = (*cur)[0];
   tmp[1] = '\0';

   Eina_Inarray *inarray = eina_hash_find(col_src->color_hash, tmp);
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

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

color_data *
color_init(Eina_Strbuf *strbuf)
{
   color_data *cd = calloc(1, sizeof(color_data));
   if (!cd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }

   init_count++;

   cd->strbuf = strbuf;
   cd->cachebuf = eina_strbuf_new();
   cd->thread = ecore_thread_run(init_thread_blocking, NULL, NULL, cd);

   /* TODO: Improve to share macro info through color instances. Might be this
      could be global static instance and could be shared with locking
      mechanism... */
   cd->macros = NULL;

   return cd;
}

void
color_term(color_data *cd)
{
   ecore_thread_cancel(cd->thread);

   Eina_Stringshare *macro;
   EINA_LIST_FREE(cd->macros, macro) eina_stringshare_del(macro);

   eina_strbuf_free(cd->cachebuf);

   free(cd);

   //release shared color source.
   if ((--init_count) == 0)
     {
        syntax_color_source *col_src = &g_color_src;

        eina_hash_free(col_src->color_hash);
        eina_stringshare_del(col_src->col_string);
        eina_stringshare_del(col_src->col_comment);
        eina_stringshare_del(col_src->col_macro);

        int i;
        for(i = 0; i < col_src->color_cnt; i++)
          eina_stringshare_del(col_src->cols[i]);
     }
}

static Eina_Bool
color_hash_foreach_cb(const Eina_Hash *hash EINA_UNUSED,
                      const void *key EINA_UNUSED, void *data, void *fdata)
{
   Eina_Inarray *inarray = data;
   color_hash_foreach_data *fd = fdata;
   color_tuple *tuple;

   EINA_INARRAY_FOREACH(inarray, tuple)
     {
        if (tuple->col == fd->cur_col)
          tuple->col = fd->new_col;
     }
   return EINA_TRUE;
}

//FIXME: Need synchronization... ?
void
color_set(color_data *cd, Enventor_Syntax_Color_Type color_type,
          const char *val)
{
   Eina_Stringshare *col;
   color_hash_foreach_data fd;
   syntax_color_source *col_src = cd->col_src;

   switch (color_type)
     {
        case ENVENTOR_SYNTAX_COLOR_STRING:
          {
             eina_stringshare_del(col_src->col_string);
             col_src->col_string = eina_stringshare_add(val);
             break;
          }
        case ENVENTOR_SYNTAX_COLOR_COMMENT:
          {
             eina_stringshare_del(col_src->col_comment);
             col_src->col_comment = eina_stringshare_add(val);
             break;
          }
        case ENVENTOR_SYNTAX_COLOR_MACRO:
          {
             eina_stringshare_del(col_src->col_macro);
             col_src->col_macro = eina_stringshare_add(val);
             break;
          }
        case ENVENTOR_SYNTAX_COLOR_SYMBOL:
          {
             col = eina_stringshare_add(val);
             fd.cur_col = col_src->cols[0];
             fd.new_col = col;
             eina_hash_foreach(col_src->color_hash, color_hash_foreach_cb, &fd);
             eina_stringshare_del(col_src->cols[0]);
             col_src->cols[0] = col;
             break;
          }
        case ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD:
          {
             col = eina_stringshare_add(val);
             fd.cur_col = col_src->cols[1];
             fd.new_col = col;
             eina_hash_foreach(col_src->color_hash, color_hash_foreach_cb, &fd);
             eina_stringshare_del(col_src->cols[1]);
             col_src->cols[1] = col;
             break;
          }
        case ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD:
          {
             col = eina_stringshare_add(val);
             fd.cur_col = col_src->cols[2];
             fd.new_col = col;
             eina_hash_foreach(col_src->color_hash, color_hash_foreach_cb, &fd);
             eina_stringshare_del(col_src->cols[2]);
             col_src->cols[2] = col;
             break;
          }
        case ENVENTOR_SYNTAX_COLOR_CONSTANT:
          {
             col = eina_stringshare_add(val);
             fd.cur_col = col_src->cols[3];
             fd.new_col = col;
             eina_hash_foreach(col_src->color_hash, color_hash_foreach_cb, &fd);
             eina_stringshare_del(col_src->cols[3]);
             col_src->cols[3] = col;
             break;
          }
        case ENVENTOR_SYNTAX_COLOR_SCRIPT_FUNC:
          {
             col = eina_stringshare_add(val);
             fd.cur_col = col_src->cols[4];
             fd.new_col = col;
             eina_hash_foreach(col_src->color_hash, color_hash_foreach_cb, &fd);
             eina_stringshare_del(col_src->cols[4]);
             col_src->cols[4] = col;
             break;
          }
        case ENVENTOR_SYNTAX_COLOR_SCRIPT_KEYWORD:
          {
             col = eina_stringshare_add(val);
             fd.cur_col = col_src->cols[5];
             fd.new_col = col;
             eina_hash_foreach(col_src->color_hash, color_hash_foreach_cb, &fd);
             eina_stringshare_del(col_src->cols[5]);
             col_src->cols[5] = col;
             break;
          }
        case ENVENTOR_SYNTAX_COLOR_LAST:  //avoiding compiler warning
          break;
     }
}

const char *
color_get(color_data *cd, Enventor_Syntax_Color_Type color_type)
{
   syntax_color_source *col_src = cd->col_src;

   switch (color_type)
     {
        case ENVENTOR_SYNTAX_COLOR_STRING:
          return (const char *) col_src->col_string;
        case ENVENTOR_SYNTAX_COLOR_COMMENT:
          return (const char *) col_src->col_comment;
        case ENVENTOR_SYNTAX_COLOR_MACRO:
          return (const char *) col_src->col_macro;
        case ENVENTOR_SYNTAX_COLOR_SYMBOL:
          return (const char *) col_src->cols[0];
        case ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD:
          return (const char *) col_src->cols[1];
        case ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD:
          return (const char *) col_src->cols[2];
        case ENVENTOR_SYNTAX_COLOR_CONSTANT:
          return (const char *) col_src->cols[3];
        case ENVENTOR_SYNTAX_COLOR_SCRIPT_FUNC:
          return (const char *) col_src->cols[4];
        case ENVENTOR_SYNTAX_COLOR_SCRIPT_KEYWORD:
          return (const char *) col_src->cols[5];
        default:
          return NULL;
     }
}

const char *
color_apply(color_data *cd, const char *src, int length, char *from, char *to)
{
   Eina_Bool inside_string = EINA_FALSE;
   Eina_Bool inside_comment = EINA_FALSE;

   if (!src || (length < 1)) return NULL;

   syntax_color_source *col_src = cd->col_src;

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

        //escape string: " ~ "
        ret = string_apply(strbuf, &cur, &prev, col_src->col_string,
                           inside_string);
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

        //handle comment: /* ~ */
        ret = comment_apply(strbuf, &src, length, &cur, &prev,
                            col_src->col_comment, &inside_comment);
        if (ret == 1) continue;
        else if (ret == -1) goto finished;

        //handle comment: //
        if (!from || (cur >= from))
          {
             ret = comment2_apply(strbuf, &src, length, &cur, &prev,
                                  col_src->col_comment, &inside_comment);
             if (ret == 1) continue;
             else if (ret == -1) goto finished;
          }

        if (*cur == '<')
          {
             //escape EOL: <br/>
             if (!strncmp(cur, EOL, EOL_LEN))
               {
                  cur += EOL_LEN;
                  continue;
               }

             //escape TAB: <tab/>
             if (!strncmp(cur, TAB, TAB_LEN))
               {
                  cur += TAB_LEN;
                  continue;
               }
          }

        //handle comment: preprocessors, #
        ret = macro_apply(strbuf, &src, length, &cur, &prev, col_src->col_macro,
                          cd);
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
