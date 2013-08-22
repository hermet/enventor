#include <Elementary.h>
#include "common.h"

struct config_s
{
   const char *edc_path;
   const char *edj_path;

   Eina_List *edc_img_path_list;
   Eina_List *edc_snd_path_list;
   Eina_Strbuf *edc_img_path_buf; //pre-stored image paths for edc compile.
   Eina_Strbuf *edc_snd_path_buf; //pre-stored sound paths for edc compile.
   float font_size;

   void (*update_cb)(void *data, option_data *od);
   void *update_cb_data;
   Evas_Coord_Size view_size;


   Eina_Bool stats_bar : 1;
   Eina_Bool linenumber : 1;
   Eina_Bool part_highlight : 1;
   Eina_Bool dummy_swallow : 1;
   Eina_Bool auto_indent : 1;
};

void
option_view_size_set(option_data *od, Evas_Coord w, Evas_Coord h)
{
   od->view_size.w = w;
   od->view_size.h = h;
}

void
option_view_size_get(option_data *od, Evas_Coord *w, Evas_Coord *h)
{
   if (w) *w = od->view_size.w;
   if (h) *h = od->view_size.h;
}

static void
option_edj_path_update(option_data *od)
{
   //apply edj path also
   char buf[PATH_MAX];
   char edj_path[PATH_MAX];

   char *ext = strstr(od->edc_path, ".edc");
   const char *file = ecore_file_file_get(od->edc_path);
   if (ext && file)
     snprintf(buf, (ext - file) + 1, "%s", file);
   else
     strncpy(buf, file, sizeof(buf));
   sprintf(edj_path, "%s/%s.edj", ecore_file_dir_get(od->edc_path), buf);

   eina_stringshare_replace(&od->edj_path, edj_path);
}

void
option_edc_path_set(option_data *od, const char *edc_path)
{
   eina_stringshare_replace(&od->edc_path, edc_path);
   option_edj_path_update(od);
}

option_data *
option_init(const char *edc_path, const char *edc_img_path,
            const char *edc_snd_path)
{
   option_data *od = calloc(1, sizeof(option_data));

   od->edc_path = eina_stringshare_add(edc_path);
   option_edj_path_update(od);
   option_edc_img_path_set(od, edc_img_path);
   option_edc_snd_path_set(od, edc_snd_path);

   od->font_size = 1.0f;
   od->linenumber = EINA_TRUE;
   od->part_highlight = EINA_TRUE;
   od->dummy_swallow = EINA_TRUE;
   od->auto_indent = EINA_TRUE;

   return od;
}

void
option_term(option_data *od)
{
   eina_stringshare_del(od->edc_path);
   eina_stringshare_del(od->edj_path);

   Eina_List *l;
   Eina_Stringshare *str;

   //free the image paths
   EINA_LIST_FOREACH(od->edc_img_path_list, l, str)
     eina_stringshare_del(str);
   eina_list_free(od->edc_img_path_list);

   //free the sound paths
   EINA_LIST_FOREACH(od->edc_snd_path_list, l, str)
     eina_stringshare_del(str);
   eina_list_free(od->edc_snd_path_list);

   free(od);
}

void
option_edc_snd_path_set(option_data *od, const char *edc_snd_path)
{
   //Free the existing paths
   Eina_List *l;
   const char *s;
   EINA_LIST_FOREACH(od->edc_snd_path_list, l, s)
     eina_stringshare_del(s);
   od->edc_snd_path_list = eina_list_free(od->edc_snd_path_list);

   if (od->edc_snd_path_buf) eina_strbuf_free(od->edc_snd_path_buf);
   od->edc_snd_path_buf = eina_strbuf_new();

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
             od->edc_snd_path_list = eina_list_append(od->edc_snd_path_list,
                                                      append);
             eina_strbuf_append(od->edc_snd_path_buf, " -sd ");
             eina_strbuf_append(od->edc_snd_path_buf, append);
             lex++;
          }
        else
          {
             append = eina_stringshare_add(edc_snd_path);
             od->edc_snd_path_list = eina_list_append(od->edc_snd_path_list,
                                                      append);
             eina_strbuf_append(od->edc_snd_path_buf, " -sd ");
             eina_strbuf_append(od->edc_snd_path_buf, append);
          }

        edc_snd_path = lex;
     }
}


void
option_edc_img_path_set(option_data *od, const char *edc_img_path)
{
   //Free the existing paths
   Eina_List *l;
   const char *s;
   EINA_LIST_FOREACH(od->edc_img_path_list, l, s)
     eina_stringshare_del(s);
   od->edc_img_path_list = eina_list_free(od->edc_img_path_list);

   if (od->edc_img_path_buf) eina_strbuf_free(od->edc_img_path_buf);
   od->edc_img_path_buf = eina_strbuf_new();

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
             od->edc_img_path_list = eina_list_append(od->edc_img_path_list,
                                                      append);
             eina_strbuf_append(od->edc_img_path_buf, " -id ");
             eina_strbuf_append(od->edc_img_path_buf, append);
             lex++;
          }
        else
          {
             append = eina_stringshare_add(edc_img_path);
             od->edc_img_path_list = eina_list_append(od->edc_img_path_list,
                                                      append);
             eina_strbuf_append(od->edc_img_path_buf, " -id ");
             eina_strbuf_append(od->edc_img_path_buf, append);
          }

        edc_img_path = lex;
     }
}

void
option_apply(option_data *od)
{
   if (od->update_cb) od->update_cb(od->update_cb_data, od);
}

const Eina_List *
option_edc_img_path_list_get(option_data *od)
{
   return od->edc_img_path_list;
}

const Eina_List *
option_edc_snd_path_list_get(option_data *od)
{
   return od->edc_snd_path_list;
}

const char *
option_edc_img_path_get(option_data *od)
{
   if (!od->edc_img_path_buf) return NULL;
   return eina_strbuf_string_get(od->edc_img_path_buf);
}

const char *
option_edc_snd_path_get(option_data *od)
{
   if (!od->edc_snd_path_buf) return NULL;
   return eina_strbuf_string_get(od->edc_snd_path_buf);
}

const char *
option_edc_path_get(option_data *od)
{
   return od->edc_path;
}

const char *
option_edj_path_get(option_data *od)
{
   return od->edj_path;
}

Eina_Bool
option_linenumber_get(option_data *od)
{
   return od->linenumber;
}

Eina_Bool
option_stats_bar_get(option_data *od)
{
   return od->stats_bar;
}

void
option_linenumber_set(option_data *od, Eina_Bool enabled)
{
   od->linenumber = enabled;
}

void
option_stats_bar_set(option_data *od, Eina_Bool enabled)
{
   od->stats_bar = enabled;
}

void
option_update_cb_set(option_data *od, void (*cb)(void *data, option_data *od),
                     void *data)
{
   od->update_cb = cb;
   od->update_cb_data = data;
}

Eina_Bool
option_part_highlight_get(option_data *od)
{
   return od->part_highlight;
}

void
option_part_highlight_set(option_data *od, Eina_Bool highlight)
{
   od->part_highlight = highlight;
}

Eina_Bool
option_dummy_swallow_get(option_data *od)
{
   return od->dummy_swallow;
}

void
option_dummy_swallow_set(option_data *od, Eina_Bool dummy_swallow)
{
   od->dummy_swallow = dummy_swallow;
}

Eina_Bool
option_auto_indent_get(option_data *od)
{
   return od->auto_indent;
}

void
option_font_size_set(option_data *od, float font_size)
{
   od->font_size = font_size;
}

float
option_font_size_get(option_data *od)
{
   return od->font_size;
}

void
option_auto_indent_set(option_data *od, Eina_Bool auto_indent)
{
   od->auto_indent = auto_indent;
}
