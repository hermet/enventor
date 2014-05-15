#include <Elementary.h>
#include "common.h"

#define TUPLE_SET(table, hash_key) \
   { \
      cnt = sizeof(table) / sizeof(color_input); \
      inarray = eina_inarray_new(sizeof(color_tuple), 0); \
      for (i = 0; i < cnt; i++) \
        { \
           tuple = malloc(sizeof(color_tuple)); \
           tuple->key = eina_stringshare_add(table[i].key); \
           tuple->col = cd->cols[table[i].col_id]; \
           eina_inarray_push(inarray, tuple); \
        } \
      eina_hash_add(cd->color_hash, hash_key, inarray); \
   }

#define COL_NUM 8

typedef struct color_tuple
{
   Eina_Stringshare *key;
   Eina_Stringshare *col;
} color_tuple;

typedef struct color_input
{
   char *key;
   int col_id;
} color_input;


// Here Color Inputs should be removed here. but put in the configure file.
static color_input color_input_a[6] = {
   {"action", 2}, {"after", 2}, {"align", 2}, {"aspect_preference", 2},
   {"aspect", 2}, {"anim", 6}
};
static color_input color_input_b[4] =
{
   {"backface_cull", 2}, {"base", 2}, {"border_scale", 2}, {"border", 2}
};
static color_input color_input_c[9] =
{
   {"collections", 1}, {"center", 2}, {"clip_to", 2}, {"color2", 2},
   {"color3", 2}, {"color_class", 2}, {"color", 2}, {"cancel_anim", 6},
   {"cancel_timer", 6}
};
static color_input color_input_d[2] =
{
   {"data", 1}, {"description", 1}
};
static color_input color_input_e[4] =
{
   {"effect", 2}, {"ellipsis", 2}, {"entry_mode", 2}, {"else", 7}
};
static color_input color_input_f[4] =
{
   {"fill", 1}, {"fixed", 2}, {"focal", 2}, {"font", 2}
};
static color_input color_input_g[3] =
{
   {"group", 1}, {"get_float", 6}, {"get_int", 6}
};
static color_input color_input_i[7] =
{
   {"images", 1}, {"ignore_flags", 2}, {"inherit", 2}, {"item", 2},
   {"if", 7}, {"image:", 2},  {"image", 1}
};
static color_input color_input_m[5] =
{
   {"map", 1}, {"max", 2}, {"min", 2}, {"mouse_events", 2}, {"multiline", 2}
};
static color_input color_input_n[3] =
{
   {"name", 2}, {"normal", 2}, {"new", 7}
};
static color_input color_input_o[3] =
{
   {"origin", 1}, {"offset", 2}, {"on", 2}
};
static color_input color_input_p[8] =
{
   {"parts", 1}, {"part", 1}, {"programs", 1}, {"program", 1},
   {"perspective:", 2}, {"perspective_on", 2}, {"public", 7},
   {"perspective", 1}
};
static color_input color_input_r[6] =
{
   {"rel1", 1}, {"rel2", 1}, {"rotation", 1}, {"relative", 2},
   {"repeat_events", 2}, {"run_program", 6}
};
static color_input color_input_s[15] =
{
   {"script", 1}, {"styles", 1}, {"scale", 2}, {"select_mode", 2},
   {"signal", 2}, {"state", 2}, {"style", 2}, {"smooth", 2}, {"source", 2},
   {"set_float", 6}, {"set_int", 6}, {"set_state", 6}, {"set_tween_state", 6},
   {"size:", 2}, {"size", 1}
};
static color_input color_input_t[11] =
{
   {"tag", 2}, {"target", 2}, {"to_x", 2}, {"to_y", 2}, {"to", 2},
   {"transition", 2}, {"type", 2}, {"tween", 2}, {"timer", 6}, {"text:", 2},
   {"text", 1}
};
static color_input color_input_x[1] =
{
   {"x:", 2}
};
static color_input color_input_y[1] =
{
   {"y:", 2}
};
static color_input color_input_z[2] =
{
   {"z:", 2}, {"zplane", 2}
};
static color_input color_input_A[3] =
{
   {"ACCELERATE_FACTOR", 3}, {"ACCELERATE", 3}, {"ACTION_STOP", 3}
};
static color_input color_input_B[3] =
{
   {"BOTH", 3}, {"BOUNCE", 3}, {"BOX", 3}
};
static color_input color_input_C[2] =
{
   {"COMP", 3}, {"CURRENT", 3}
};
static color_input color_input_D[3] =
{
   {"DECELERATE_FACTOR", 3}, {"DECELERATE", 3}, {"DIVISOR_INTERP", 3}
};
static color_input color_input_E[3] =
{
   {"EDITABLE", 3}, {"EXPLICIT", 3}, {"EXTERNAL", 3}
};
static color_input color_input_F[2] =
{
   {"FAR_SHADOW", 3}, {"FAR_SOFT_SHADOW", 3}
};
static color_input color_input_G[3] =
{
   {"GLOW", 3}, {"GRADIENT", 3}, {"GROUP", 3}
};
static color_input color_input_H[1] =
{
   {"HORIZONTAL", 1}
};
static color_input color_input_I[1] =
{
   {"IMAGE", 3}
};
static color_input color_input_L[2] =
{
   {"LINEAR", 3}, {"LOSSY", 3}
};
static color_input color_input_N[1] =
{
   {"NONE", 3}
};
static color_input color_input_O[4] =
{
   {"ON_HOLD", 3}, {"OUTLINE_SOFT_SHADOW", 3}, {"OUTLINE_SHADOW", 3},
   {"OUTLINE", 3}
};
static color_input color_input_P[3] =
{
   {"PLAIN", 3}, {"PROGRAM", 3}, {"PROXY", 3}
};
static color_input color_input_R[2] =
{
   {"RAW", 3}, {"RECT", 3}
};
static color_input color_input_S[10] =
{
   {"SHADOW", 3}, {"SIGNAL_EMIT", 3}, {"SINUSOIDAL_FACTOR", 3},
   {"SINUSOIDAL", 3}, {"SOFT_OUTLINE", 3}, {"SOFT_SHADOW", 3}, {"SPACER", 3},
   {"SPRING", 3}, {"STATE_SET", 3}, {"SWALLOW", 3}
};
static color_input color_input_T[3] =
{
   {"TABLE", 3}, {"TEXTBLOCK", 3}, {"TEXT", 3}
};
static color_input color_input_U[1] =
{
   {"USER", 3}
};
static color_input color_input_V[1] =
{
   {"VERTICAL", 3}
};
static color_input color_input_lp[1] =
{
   {"{", 0}
};
static color_input color_input_rp[1] =
{
   {"}", 0}
};
static color_input color_input_lt[1] =
{
   {"[", 0}
};
static color_input color_input_rt[1] =
{
   {"]", 0}
};
static color_input color_input_sc[1] =
{
   {";", 0}
};
static color_input color_input_co[1] =
{
   {":", 0}
};

struct syntax_color_s
{
   Eina_Strbuf *strbuf;
   Eina_Hash *color_hash;
   Eina_Stringshare *cols[COL_NUM];
};

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
color_table_init(color_data *cd)
{
   cd->color_hash = eina_hash_string_small_new(hash_free_cb);
   color_tuple *tuple;
   Eina_Inarray *inarray;
   int i;
   int cnt;

   TUPLE_SET(color_input_a, "a");
   TUPLE_SET(color_input_b, "b");
   TUPLE_SET(color_input_c, "c");
   TUPLE_SET(color_input_d, "d");
   TUPLE_SET(color_input_e, "e");
   TUPLE_SET(color_input_f, "f");
   TUPLE_SET(color_input_g, "g");
   TUPLE_SET(color_input_i, "i");
   TUPLE_SET(color_input_m, "m");
   TUPLE_SET(color_input_n, "n");
   TUPLE_SET(color_input_o, "o");
   TUPLE_SET(color_input_p, "p");
   TUPLE_SET(color_input_r, "r");
   TUPLE_SET(color_input_s, "s");
   TUPLE_SET(color_input_t, "t");
   TUPLE_SET(color_input_x, "x");
   TUPLE_SET(color_input_y, "y");
   TUPLE_SET(color_input_z, "z");
   TUPLE_SET(color_input_A, "A");
   TUPLE_SET(color_input_B, "B");
   TUPLE_SET(color_input_C, "C");
   TUPLE_SET(color_input_D, "D");
   TUPLE_SET(color_input_E, "E");
   TUPLE_SET(color_input_F, "F");
   TUPLE_SET(color_input_G, "G");
   TUPLE_SET(color_input_H, "H");
   TUPLE_SET(color_input_I, "I");
   TUPLE_SET(color_input_L, "L");
   TUPLE_SET(color_input_N, "N");
   TUPLE_SET(color_input_O, "O");
   TUPLE_SET(color_input_P, "P");
   TUPLE_SET(color_input_R, "R");
   TUPLE_SET(color_input_S, "S");
   TUPLE_SET(color_input_T, "T");
   TUPLE_SET(color_input_U, "U");
   TUPLE_SET(color_input_V, "V");
   TUPLE_SET(color_input_lp, "{");
   TUPLE_SET(color_input_rp, "}");
   TUPLE_SET(color_input_lt, "[");
   TUPLE_SET(color_input_rt, "]");
   TUPLE_SET(color_input_sc, ";");
   TUPLE_SET(color_input_co, ":");
}

color_data *
color_init(Eina_Strbuf *strbuf)
{
   color_data *cd = malloc(sizeof(color_data));
   cd->strbuf = strbuf;

   cd->cols[0] = eina_stringshare_add("656565");
   cd->cols[1] = eina_stringshare_add("2070D0");
   cd->cols[2] = eina_stringshare_add("72AAD4");
   cd->cols[3] = eina_stringshare_add("D4D42A");
   cd->cols[4] = eina_stringshare_add("00B000");
   cd->cols[5] = eina_stringshare_add("D42A2A");
   cd->cols[6] = eina_stringshare_add("00FFFF");
   cd->cols[7] = eina_stringshare_add("D78700");

   color_table_init(cd);

   return cd;
}

void
color_term(color_data *cd)
{
   eina_hash_free(cd->color_hash);

   int i;
   for(i = 0; i < COL_NUM; i++)
     eina_stringshare_del(cd->cols[i]);

   free(cd);
}

static Eina_Bool
color_markup_insert(Eina_Strbuf *strbuf, const char **src, int length,
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
   int cmp_size = 6;    //strlen("<tab/>");
   if (strncmp(*cur, "<tab/>", cmp_size)) return 0;
   eina_strbuf_append_length(strbuf, *prev, (*cur - *prev + cmp_size));
   *cur += cmp_size;
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
sharp_apply(Eina_Strbuf *strbuf, const char **src, int length, char **cur,
            char **prev, const Eina_Stringshare *color)
{
   if ((*cur)[0] != '#') return 0;

   eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));

   char buf[128];
   snprintf(buf, sizeof(buf), "<color=#%s>#", color);
   eina_strbuf_append(strbuf, buf);

   int cmp_size = 1;    //strlen("#");
   *cur += cmp_size;

   if (*cur > (*src + length))
     {
        eina_strbuf_append(strbuf, "</color>");
        return -1;
     }

   *prev = *cur;

   char *space = strstr(*prev, " ");
   char *eol = strstr(*prev, EOL);

   if (space < eol)
     {
        *cur = space;
        cmp_size = 1; //strlen(" ");
     }
   else
     {
        *cur = eol;
        cmp_size = EOL_LEN;
     }

   if (*cur)
     {
        eina_strbuf_append_length(strbuf, *prev, (*cur - *prev));
        if (space < eol) eina_strbuf_append(strbuf, "</color> ");
        else eina_strbuf_append(strbuf, "</color><br/>");
        *cur += cmp_size;
        *prev = *cur;
        return 1;
     }

   eina_strbuf_append(strbuf, *prev);
   *prev = *cur;

   eina_strbuf_append(strbuf, "</color>");
   return -1;
}


const char *
color_cancel(color_data *cd, const char *src, int length)
{
   if (!src || length < 1) return NULL;
   Eina_Strbuf *strbuf = cd->strbuf;
   eina_strbuf_reset(strbuf);

   const char *str = NULL;
   char *prev = (char *) src;
   char *cur = (char *) src;

   while (cur && (cur <= (src + length)))
     {
        //escape EOL: <br/>
        if (br_skip(strbuf, &src, length, &cur, &prev) == 1)
          continue;

        //escape EOL: <tab/>
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
   return str;
}

static int
bracket_escape(Eina_Strbuf *strbuf, char **cur, char **prev)
{
   if ((*cur)[0] != '&') return 0;
   int cmp_size = 4;

   if (!strncmp(*cur, "&lt;", cmp_size))
     {
        eina_strbuf_append_length(strbuf, *prev, *cur - *prev);
        eina_strbuf_append(strbuf, "&lt;");
        *cur += cmp_size;
        *prev = *cur;
        return 1;
     }
   else if (!strncmp(*cur, "&gt;", cmp_size))
     {
        eina_strbuf_append_length(strbuf, *prev, *cur - *prev);
        eina_strbuf_append(strbuf, "&gt;");
        *cur += cmp_size;
        *prev = *cur;
        return 1;
     }
   return 0;
}

/* 
	OPTIMIZATION POINT 
	1. Apply Color only changed line.
*/
const char *
color_apply(color_data *cd, const char *src, int length)
{
   Eina_Bool inside_string = EINA_FALSE;
   Eina_Bool inside_comment = EINA_FALSE;

   if (!src || (length < 1)) return NULL;

   Eina_Strbuf *strbuf = cd->strbuf;
   eina_strbuf_reset(strbuf);

   const char *str = NULL;
   char *prev = (char *) src;
   char *cur = (char *) src;
   int ret;
   Eina_Inarray *inarray;
   color_tuple *tuple;

   while (cur && (cur <= (src + length)))
     {
        //escape empty string
        if (cur[0] == ' ')
          {
             eina_strbuf_append_length(strbuf, prev, cur - prev);
             eina_strbuf_append_char(strbuf, ' ');
             ++cur;
             prev = cur;
             continue;
          }

        //handle comment: /* ~ */
        ret = comment_apply(strbuf, &src, length, &cur, &prev, cd->cols[4],
                            &inside_comment);
        if (ret == 1) continue;
        else if (ret == -1) goto finished;

        //handle comment: //
        ret = comment2_apply(strbuf, &src, length, &cur, &prev, cd->cols[4],
                             &inside_comment);
        if (ret == 1) continue;
        else if (ret == -1) goto finished;

        //escape string: " ~ "
        if (cur[0] == QUOT_C)
          {
             eina_strbuf_append_length(strbuf, prev, cur - prev);
             eina_strbuf_append_char(strbuf, QUOT_C);
             cur++;
             prev = cur;
             inside_string = !inside_string;
             continue;
          }
        if (inside_string || inside_comment)
          {
             cur++;
             continue;
          }

        //FIXME: This might be textblock problem. should be removed here.
        //escape <> bracket.
        ret = bracket_escape(strbuf, &cur, &prev);
        if (ret == 1) continue;

        //handle comment: #
        ret = sharp_apply(strbuf, &src, length, &cur, &prev, cd->cols[5]);
        if (ret == 1) continue;
        else if (ret == -1) goto finished;

        char key[2];
        key[0] = cur[0];
        key[1] = '\0';
        inarray = eina_hash_find(cd->color_hash, key);

        //Found tuple list. Search in detail.
        if (inarray)
          {
             Eina_Bool found = EINA_FALSE;

             EINA_INARRAY_FOREACH(inarray, tuple)
               {
                  if (!strncmp(cur, tuple->key, strlen(tuple->key)))
                    {
                       ret = color_markup_insert(strbuf, &src, length, &cur,
                                                 &prev, tuple->key, tuple->col);
                       if (ret)
                         {
                            found = EINA_TRUE;
                            break;
                         }
                       else goto finished;
                    }
               }
             if (found) continue;
          }
        cur++;
     }

   //Same with origin source.
   if (prev == src)
     str = src;
   //Some color syntax is applied.
   else
     {
finished:
        //append leftovers.
        if (prev + 1 < cur) eina_strbuf_append(strbuf, prev);
        str = eina_strbuf_string_get(strbuf);
     }

   return str;
}
