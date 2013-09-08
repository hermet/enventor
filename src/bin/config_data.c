#include <Elementary.h>
#include "common.h"

struct config_s
{
   const char *edc_path;
   const char *edj_path;

   Eina_List *edc_img_path_list;
   Eina_List *edc_snd_path_list;
   Eina_List *edc_fnt_path_list;
   Eina_List *edc_data_path_list;
   Eina_Strbuf *edc_img_path_buf; //pre-stored image paths for edc compile.
   Eina_Strbuf *edc_snd_path_buf; //pre-stored sound paths for edc compile.
   Eina_Strbuf *edc_fnt_path_buf; //pre-stored font paths for edc compile.
   Eina_Strbuf *edc_data_path_buf; //pre-stored data paths for edc compile.

   float font_size;

   void (*update_cb)(void *data, config_data *cd);
   void *update_cb_data;
   Evas_Coord_Size view_size;


   Eina_Bool stats_bar : 1;
   Eina_Bool linenumber : 1;
   Eina_Bool part_highlight : 1;
   Eina_Bool dummy_swallow : 1;
   Eina_Bool auto_indent : 1;
};

void
config_view_size_set(config_data *cd, Evas_Coord w, Evas_Coord h)
{
   cd->view_size.w = w;
   cd->view_size.h = h;
}

void
config_view_size_get(config_data *cd, Evas_Coord *w, Evas_Coord *h)
{
   if (w) *w = cd->view_size.w;
   if (h) *h = cd->view_size.h;
}

static void
config_edj_path_update(config_data *cd)
{
   //apply edj path also
   char buf[PATH_MAX];
   char edj_path[PATH_MAX];

   char *ext = strstr(cd->edc_path, ".edc");
   const char *file = ecore_file_file_get(cd->edc_path);
   if (ext && file)
     snprintf(buf, (ext - file) + 1, "%s", file);
   else
     strncpy(buf, file, sizeof(buf));
   sprintf(edj_path, "%s/%s.edj", ecore_file_dir_get(cd->edc_path), buf);

   eina_stringshare_replace(&cd->edj_path, edj_path);
}

void
config_edc_path_set(config_data *cd, const char *edc_path)
{
   eina_stringshare_replace(&cd->edc_path, edc_path);
   config_edj_path_update(cd);
}

config_data *
config_init(const char *edc_path, const char *edc_img_path,
            const char *edc_snd_path, const char *edc_fnt_path,
            const char *edc_data_path)
{
   config_data *cd = calloc(1, sizeof(config_data));

   cd->edc_path = eina_stringshare_add(edc_path);
   config_edj_path_update(cd);
   config_edc_img_path_set(cd, edc_img_path);
   config_edc_snd_path_set(cd, edc_snd_path);
   config_edc_fnt_path_set(cd, edc_fnt_path);
   config_edc_data_path_set(cd, edc_data_path);

   cd->font_size = 1.0f;
   cd->linenumber = EINA_TRUE;
   cd->part_highlight = EINA_TRUE;
   cd->dummy_swallow = EINA_TRUE;
   cd->auto_indent = EINA_TRUE;

   return cd;
}

void
config_term(config_data *cd)
{
   eina_stringshare_del(cd->edc_path);
   eina_stringshare_del(cd->edj_path);

   Eina_List *l;
   Eina_Stringshare *str;

   //free the image paths
   EINA_LIST_FOREACH(cd->edc_img_path_list, l, str)
     eina_stringshare_del(str);
   eina_list_free(cd->edc_img_path_list);

   //free the sound paths
   EINA_LIST_FOREACH(cd->edc_snd_path_list, l, str)
     eina_stringshare_del(str);
   eina_list_free(cd->edc_snd_path_list);

   //free the font paths
   EINA_LIST_FOREACH(cd->edc_fnt_path_list, l, str)
     eina_stringshare_del(str);
   eina_list_free(cd->edc_fnt_path_list);

   //free the data paths
   EINA_LIST_FOREACH(cd->edc_data_path_list, l, str)
     eina_stringshare_del(str);
   eina_list_free(cd->edc_data_path_list);

   if (cd->edc_img_path_buf) eina_strbuf_free(cd->edc_img_path_buf);
   if (cd->edc_snd_path_buf) eina_strbuf_free(cd->edc_snd_path_buf);
   if (cd->edc_fnt_path_buf) eina_strbuf_free(cd->edc_fnt_path_buf);
   if (cd->edc_data_path_buf) eina_strbuf_free(cd->edc_data_path_buf);

   free(cd);
}

void
config_edc_snd_path_set(config_data *cd, const char *edc_snd_path)
{
   //Free the existing paths
   Eina_List *l;
   const char *s;
   EINA_LIST_FOREACH(cd->edc_snd_path_list, l, s)
     eina_stringshare_del(s);
   cd->edc_snd_path_list = eina_list_free(cd->edc_snd_path_list);

   if (cd->edc_snd_path_buf) eina_strbuf_free(cd->edc_snd_path_buf);
   cd->edc_snd_path_buf = eina_strbuf_new();

   //parse paths by ';'
   const char *lex;
   Eina_Stringshare *append;

   while(edc_snd_path && (strlen(edc_snd_path) > 0))
     {
        lex = strstr(edc_snd_path, ";");
        if (lex)
          {
             append = eina_stringshare_add_length(edc_snd_path,
                                                  (lex - edc_snd_path));
             cd->edc_snd_path_list = eina_list_append(cd->edc_snd_path_list,
                                                      append);
             eina_strbuf_append(cd->edc_snd_path_buf, " -sd ");
             eina_strbuf_append(cd->edc_snd_path_buf, append);
             lex++;
          }
        else
          {
             append = eina_stringshare_add(edc_snd_path);
             cd->edc_snd_path_list = eina_list_append(cd->edc_snd_path_list,
                                                      append);
             eina_strbuf_append(cd->edc_snd_path_buf, " -sd ");
             eina_strbuf_append(cd->edc_snd_path_buf, append);
          }

        edc_snd_path = lex;
     }
}

void
config_edc_data_path_set(config_data *cd, const char *edc_data_path)
{
   //Free the existing paths
   Eina_List *l;
   const char *s;
   EINA_LIST_FOREACH(cd->edc_data_path_list, l, s)
     eina_stringshare_del(s);
   cd->edc_data_path_list = eina_list_free(cd->edc_data_path_list);

   if (cd->edc_data_path_buf) eina_strbuf_free(cd->edc_data_path_buf);
   cd->edc_data_path_buf = eina_strbuf_new();

   //parse paths by ';'
   const char *lex;
   Eina_Stringshare *append;

   while(edc_data_path && (strlen(edc_data_path) > 0))
     {
        lex = strstr(edc_data_path, ";");
        if (lex)
          {
             append = eina_stringshare_add_length(edc_data_path,
                                                  (lex - edc_data_path));
             cd->edc_data_path_list = eina_list_append(cd->edc_data_path_list,
                                                      append);
             eina_strbuf_append(cd->edc_data_path_buf, " -fd ");
             eina_strbuf_append(cd->edc_data_path_buf, append);
             lex++;
          }
        else
          {
             append = eina_stringshare_add(edc_data_path);
             cd->edc_data_path_list = eina_list_append(cd->edc_data_path_list,
                                                      append);
             eina_strbuf_append(cd->edc_data_path_buf, " -fd ");
             eina_strbuf_append(cd->edc_data_path_buf, append);
          }

        edc_data_path = lex;
     }
}

void
config_edc_fnt_path_set(config_data *cd, const char *edc_fnt_path)
{
   //Free the existing paths
   Eina_List *l;
   const char *s;
   EINA_LIST_FOREACH(cd->edc_fnt_path_list, l, s)
     eina_stringshare_del(s);
   cd->edc_fnt_path_list = eina_list_free(cd->edc_fnt_path_list);

   if (cd->edc_fnt_path_buf) eina_strbuf_free(cd->edc_fnt_path_buf);
   cd->edc_fnt_path_buf = eina_strbuf_new();

   //parse paths by ';'
   const char *lex;
   Eina_Stringshare *append;

   while(edc_fnt_path && (strlen(edc_fnt_path) > 0))
     {
        lex = strstr(edc_fnt_path, ";");
        if (lex)
          {
             append = eina_stringshare_add_length(edc_fnt_path,
                                                  (lex - edc_fnt_path));
             cd->edc_fnt_path_list = eina_list_append(cd->edc_fnt_path_list,
                                                      append);
             eina_strbuf_append(cd->edc_fnt_path_buf, " -fd ");
             eina_strbuf_append(cd->edc_fnt_path_buf, append);
             lex++;
          }
        else
          {
             append = eina_stringshare_add(edc_fnt_path);
             cd->edc_fnt_path_list = eina_list_append(cd->edc_fnt_path_list,
                                                      append);
             eina_strbuf_append(cd->edc_fnt_path_buf, " -fd ");
             eina_strbuf_append(cd->edc_fnt_path_buf, append);
          }

        edc_fnt_path = lex;
     }
}

void
config_edc_img_path_set(config_data *cd, const char *edc_img_path)
{
   //Free the existing paths
   Eina_List *l;
   const char *s;
   EINA_LIST_FOREACH(cd->edc_img_path_list, l, s)
     eina_stringshare_del(s);
   cd->edc_img_path_list = eina_list_free(cd->edc_img_path_list);

   if (cd->edc_img_path_buf) eina_strbuf_free(cd->edc_img_path_buf);
   cd->edc_img_path_buf = eina_strbuf_new();

   //parse paths by ';'
   const char *lex;
   Eina_Stringshare *append;

   while(edc_img_path && (strlen(edc_img_path) > 0))
     {
        lex = strstr(edc_img_path, ";");
        if (lex)
          {
             append = eina_stringshare_add_length(edc_img_path,
                                                  (lex - edc_img_path));
             cd->edc_img_path_list = eina_list_append(cd->edc_img_path_list,
                                                      append);
             eina_strbuf_append(cd->edc_img_path_buf, " -id ");
             eina_strbuf_append(cd->edc_img_path_buf, append);
             lex++;
          }
        else
          {
             append = eina_stringshare_add(edc_img_path);
             cd->edc_img_path_list = eina_list_append(cd->edc_img_path_list,
                                                      append);
             eina_strbuf_append(cd->edc_img_path_buf, " -id ");
             eina_strbuf_append(cd->edc_img_path_buf, append);
          }

        edc_img_path = lex;
     }
}

void
config_apply(config_data *cd)
{
   if (cd->update_cb) cd->update_cb(cd->update_cb_data, cd);
}

Eina_List *
config_edc_img_path_list_get(config_data *cd)
{
   return cd->edc_img_path_list;
}

Eina_List *
config_edc_snd_path_list_get(config_data *cd)
{
   return cd->edc_snd_path_list;
}

Eina_List *
config_edc_data_path_list_get(config_data *cd)
{
   return cd->edc_data_path_list;
}

Eina_List *
config_edc_fnt_path_list_get(config_data *cd)
{
   return cd->edc_fnt_path_list;
}

const char *
config_edc_img_path_get(config_data *cd)
{
   if (!cd->edc_img_path_buf) return NULL;
   return eina_strbuf_string_get(cd->edc_img_path_buf);
}

const char *
config_edc_snd_path_get(config_data *cd)
{
   if (!cd->edc_snd_path_buf) return NULL;
   return eina_strbuf_string_get(cd->edc_snd_path_buf);
}

const char *
config_edc_data_path_get(config_data *cd)
{
   if (!cd->edc_data_path_buf) return NULL;
   return eina_strbuf_string_get(cd->edc_data_path_buf);
}

const char *
config_edc_fnt_path_get(config_data *cd)
{
   if (!cd->edc_fnt_path_buf) return NULL;
   return eina_strbuf_string_get(cd->edc_fnt_path_buf);
}

const char *
config_edc_path_get(config_data *cd)
{
   return cd->edc_path;
}

const char *
config_edj_path_get(config_data *cd)
{
   return cd->edj_path;
}

Eina_Bool
config_linenumber_get(config_data *cd)
{
   return cd->linenumber;
}

Eina_Bool
config_stats_bar_get(config_data *cd)
{
   return cd->stats_bar;
}

void
config_linenumber_set(config_data *cd, Eina_Bool enabled)
{
   cd->linenumber = enabled;
}

void
config_stats_bar_set(config_data *cd, Eina_Bool enabled)
{
   cd->stats_bar = enabled;
}

void
config_update_cb_set(config_data *cd, void (*cb)(void *data, config_data *cd),
                     void *data)
{
   cd->update_cb = cb;
   cd->update_cb_data = data;
}

Eina_Bool
config_part_highlight_get(config_data *cd)
{
   return cd->part_highlight;
}

void
config_part_highlight_set(config_data *cd, Eina_Bool highlight)
{
   cd->part_highlight = highlight;
}

Eina_Bool
config_dummy_swallow_get(config_data *cd)
{
   return cd->dummy_swallow;
}

void
config_dummy_swallow_set(config_data *cd, Eina_Bool dummy_swallow)
{
   cd->dummy_swallow = dummy_swallow;
}

Eina_Bool
config_auto_indent_get(config_data *cd)
{
   return cd->auto_indent;
}

void
config_font_size_set(config_data *cd, float font_size)
{
   if (font_size > MAX_FONT_SIZE)
     font_size = MAX_FONT_SIZE;
   else if (font_size < MIN_FONT_SIZE)
     font_size = MIN_FONT_SIZE;

   cd->font_size = font_size;
}

float
config_font_size_get(config_data *cd)
{
   return cd->font_size;
}

void
config_auto_indent_set(config_data *cd, Eina_Bool auto_indent)
{
   cd->auto_indent = auto_indent;
}
