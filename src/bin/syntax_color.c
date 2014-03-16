#include <Elementary.h>
#include "common.h"

#define COLOR_INSERT(strbuf, src, length, cur, prev, cmp, color) \
   { \
      ret = color_markup_insert((strbuf), (src), (length), (cur), (prev), \
                                (cmp), (color)); \
      if (ret == 1) continue; \
      else if (ret == -1) goto finished; \
   } \

#define COL_NUM 8

struct syntax_color_s
{
   Eina_Strbuf *strbuf;
   Eina_Stringshare *cols[COL_NUM];
};

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

   return cd;
}

void
color_term(color_data *cd)
{
   int i;
   for(i = 0; i < COL_NUM; i++)
     eina_stringshare_del(cd->cols[i]);

   free(cd);
}

static int
color_markup_insert(Eina_Strbuf *strbuf, const char **src, int length,
                    char **cur, char **prev,  const char *cmp,
                    Eina_Stringshare *color)
{
   char buf[128];

   //FIXME: compare opposite case.
   if (strncmp(*cur, cmp, strlen(cmp))) return 0;

   eina_strbuf_append_length(strbuf, *prev, *cur - *prev);
   snprintf(buf, sizeof(buf), "<color=#%s>%s</color>", color, cmp);
   eina_strbuf_append(strbuf, buf);
   *cur += strlen(cmp);
   if (*cur > (*src + length)) return -1;
   *prev = *cur;

   return 1;
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
	1. Use Hash
	2. Apply Color only changed line.
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

        //FIXME: construct from the configuration file
        //syntax group 1
        Eina_Stringshare *col1 = cd->cols[0];
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "{", col1);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "}", col1);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "[", col1);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "]", col1);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, ";", col1);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, ":", col1);

        //syntax group 2
        Eina_Stringshare *col2 = cd->cols[1];
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "collections", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "data", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "description", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "fill", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "group", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "images", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "map", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "origin", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "parts", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "part", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "programs", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "program", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "rel1", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "rel2", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "rotation", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "script", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "styles", col2);

        //syntax group 3
        Eina_Stringshare *col3 = cd->cols[2];
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "action", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "after", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "align", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "aspect_preference", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "aspect", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "backface_cull", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "base", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "border_scale", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "border", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "center", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "clip_to", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "color2", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "color3", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "color", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "effect", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "ellipsis", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "entry_mode", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "fixed", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "focal", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "font", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "ignore_flags", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "inherit", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "item", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "max", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "min", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "mouse_events", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "multiline", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "name", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "normal", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "offset", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "on", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "perspective:", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "perspective_on", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "scale", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "select_mode", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "signal", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "state", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "style", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "relative", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "repeat_events", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "smooth", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "source", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "tag", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "target", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "to_x", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "to_y", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "to", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "transition", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "type", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "tween", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "visible", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "x:", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "y:", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "z:", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "zplane", col3);

        //syntax group 4
        Eina_Stringshare *col4 = cd->cols[3];
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "ACCELERATE_FACTOR",
                     col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "ACCELERATE", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "ACTION_STOP", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "BOTH", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "BOUNCE", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "BOX", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "COMP", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "CURRENT", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "DECELERATE_FACTOR",
                     col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "DECELERATE", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "DIVISOR_INTERP", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "EDITABLE", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "EXPLICIT", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "EXTERNAL", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "FAR_SHADOW", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "FAR_SOFT_SHADOW",
                     col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "GLOW", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "GRADIENT", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "GROUP", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "HORIZONTAL", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "IMAGE", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "LINEAR", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "LOSSY", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "NONE", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "ON_HOLD", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "OUTLINE_SOFT_SHADOW",
                     col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "OUTLINE_SHADOW", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "OUTLINE", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "PLAIN", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "PROGRAM", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "PROXY", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "RAW", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "RECT", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "SHADOW", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "SIGNAL_EMIT", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "SINUSOIDAL_FACTOR",
                     col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "SINUSOIDAL", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "SOFT_OUTLINE", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "SOFT_SHADOW", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "SPACER", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "SPRING", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "STATE_SET", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "SWALLOW", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "TABLE", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "TEXTBLOCK", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "TEXT", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "USER", col4);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "VERTICAL", col4);

        //syntax group 7
        Eina_Stringshare *col6 = cd->cols[6];
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "anim", col6);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "cancel_anim", col6);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "cancel_timer", col6);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "get_float", col6);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "get_int", col6);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "run_program", col6);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "set_float", col6);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "set_int", col6);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "set_state", col6);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "set_tween_state",
                     col6);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "timer", col6);

        //syntax group 8
        Eina_Stringshare *col7 = cd->cols[7];
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "if", col7);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "else", col7);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "new", col7);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "public", col7);

        //duplicated groups 1
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "image:", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "size:", col3);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "text:", col3);

        //duplicated groups 2
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "image", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "size", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "text", col2);
        COLOR_INSERT(strbuf, &src, length, &cur, &prev, "perspective", col2);

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
