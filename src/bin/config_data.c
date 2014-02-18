#include <Elementary.h>
#include "common.h"

const char *IMGPATH = "imgpath";
const char *SNDPATH = "sndpath";
const char *FNTPATH = "fntpath";
const char *DATPATH = "datpath";
const char *FNTSIZE = "fntsize";
const char *VIEWSCALE = "viewscale";
const char *STATSBAR = "statsbar";
const char *LINENUM = "linenum";
const char *HIGHLIGHT = "highlight";
const char *SWALLOW = "swallow";
const char *INDENT = "indent";

typedef struct config_s
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
   double view_scale;

   void (*update_cb)(void *data);
   void *update_cb_data;
   Evas_Coord_Size view_size;

   Eina_Bool stats_bar : 1;
   Eina_Bool linenumber : 1;
   Eina_Bool part_highlight : 1;
   Eina_Bool dummy_swallow : 1;
   Eina_Bool auto_indent : 1;
   Eina_Bool hotkeys : 1;
} config_data;

static config_data *g_cd = NULL;

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


static Eina_Bool
config_load(config_data *cd)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/enventor/config.eet",
            efreet_config_home_get());
   Eet_File *ef = eet_open(buf, EET_FILE_MODE_READ);
   if (!ef)
      {
         EINA_LOG_ERR("Cannot load a config file \"%s\"", buf);
         return EINA_FALSE;
      }

   int size;
   char *ret;

   ret = eet_read(ef, IMGPATH, &size);
   if (size > 0) { config_edc_img_path_set(ret); free(ret); }
   ret = eet_read(ef, SNDPATH, &size);
   if (size > 0) { config_edc_snd_path_set(ret); free(ret); }
   ret = eet_read(ef, FNTPATH, &size);
   if (size > 0) { config_edc_fnt_path_set(ret); free(ret); }
   ret = eet_read(ef, DATPATH, &size);
   if (size > 0) { config_edc_data_path_set(ret); free(ret); }
   ret = eet_read(ef, FNTSIZE, &size);
   if (size > 0) { cd->font_size = atof(ret); free(ret); }
   ret = eet_read(ef, VIEWSCALE, &size);
   if (size > 0) { cd->view_scale = atof(ret); free(ret); }
   ret = eet_read(ef, STATSBAR, &size);
   if (size > 0) { cd->stats_bar = (Eina_Bool) atoi(ret); free(ret); }
   ret = eet_read(ef, LINENUM, &size);
   if (size > 0) { cd->linenumber = (Eina_Bool) atoi(ret); free(ret); }
   ret = eet_read(ef, HIGHLIGHT, &size);
   if (size > 0) { cd->part_highlight = (Eina_Bool) atoi(ret); free(ret); }
   ret = eet_read(ef, SWALLOW, &size);
   if (size > 0) { cd->dummy_swallow = (Eina_Bool) atoi(ret); free(ret); }
   ret = eet_read(ef, INDENT, &size);
   if (size > 0) { cd->auto_indent = (Eina_Bool) atoi(ret); free(ret); }

   eet_close(ef);

   return EINA_TRUE;
}

static void
edc_paths_write(Eet_File *ef, const char *key, Eina_List *paths,
              Eina_Strbuf *strbuf)
{
   Eina_List *l;
   char *path;

   eina_strbuf_reset(strbuf);

   EINA_LIST_FOREACH(paths, l, path)
     {
        eina_strbuf_append(strbuf, path);
        eina_strbuf_append(strbuf, ";");
     }

   const char *str = eina_strbuf_string_get(strbuf);
   eet_write(ef, key, str, strlen(str) + 1, 0);
}

static void
config_save(config_data *cd)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/enventor", efreet_config_home_get());

   //Create config folder if it doesn't exist.
   if (!ecore_file_exists(buf))
     {
        Eina_Bool success = ecore_file_mkdir(buf);
        if (!success)
          {
             EINA_LOG_ERR("Cannot create a config folder \"%s\"", buf);
             return;
          }
     }

   //Save config file.
   snprintf(buf, sizeof(buf), "%s/enventor/config.eet",
            efreet_config_home_get());
   Eet_File *ef = eet_open(buf, EET_FILE_MODE_WRITE);
   if (!ef)
     {
        EINA_LOG_ERR("Cannot save a config file \"%s\"", buf);
        return;
     }

   //TODO: Use Eet Descriptor if the attributes are getting bigger and bigger
   Eina_Strbuf *strbuf = eina_strbuf_new();
   edc_paths_write(ef, IMGPATH, cd->edc_img_path_list, strbuf);
   edc_paths_write(ef, SNDPATH, cd->edc_snd_path_list, strbuf);
   edc_paths_write(ef, FNTPATH, cd->edc_fnt_path_list, strbuf);
   edc_paths_write(ef, DATPATH, cd->edc_data_path_list, strbuf);
   eina_strbuf_free(strbuf);

   snprintf(buf, sizeof(buf), "%f", cd->font_size);
   eet_write(ef, FNTSIZE, buf, strlen(buf) + 1, 0);

   snprintf(buf, sizeof(buf), "%f", cd->view_scale);
   eet_write(ef, VIEWSCALE, buf, strlen(buf) + 1, 0);

   snprintf(buf, sizeof(buf), "%d", cd->stats_bar);
   eet_write(ef, STATSBAR, buf, strlen(buf) + 1, 0);

   snprintf(buf, sizeof(buf), "%d", cd->linenumber);
   eet_write(ef, LINENUM, buf, strlen(buf) + 1, 0);

   snprintf(buf, sizeof(buf), "%d", cd->part_highlight);
   eet_write(ef, HIGHLIGHT, buf, strlen(buf) + 1, 0);

   snprintf(buf, sizeof(buf), "%d", cd->dummy_swallow);
   eet_write(ef, SWALLOW, buf, strlen(buf) + 1, 0);

   snprintf(buf, sizeof(buf), "%d", cd->auto_indent);
   eet_write(ef, INDENT, buf, strlen(buf) + 1, 0);

   eet_close(ef);
}

void
config_edc_path_set(const char *edc_path)
{
   config_data *cd = g_cd;
   eina_stringshare_replace(&cd->edc_path, edc_path);
   config_edj_path_update(cd);
}

void
config_init(const char *edc_path, const char *edc_img_path,
            const char *edc_snd_path, const char *edc_fnt_path,
            const char *edc_data_path)
{
   config_data *cd = calloc(1, sizeof(config_data));
   g_cd = cd;

   cd->edc_path = eina_stringshare_add(edc_path);
   config_edj_path_update(cd);

   if (!config_load(cd))
     {
        //failed to load config file. set default values.
        cd->font_size = 1.0f;
        cd->view_scale = 1;
        cd->linenumber = EINA_TRUE;
        cd->part_highlight = EINA_TRUE;
        cd->dummy_swallow = EINA_TRUE;
        cd->auto_indent = EINA_TRUE;
     }

   if (edc_img_path) config_edc_img_path_set(edc_img_path);
   if (edc_snd_path) config_edc_snd_path_set(edc_snd_path);
   if (edc_fnt_path) config_edc_fnt_path_set(edc_fnt_path);
   if (edc_data_path) config_edc_data_path_set(edc_data_path);

   //hotkey is not decided yet to keep the function or not.
   cd->hotkeys = EINA_TRUE;
}

void
config_term()
{
   config_data *cd = g_cd;

   config_save(cd);

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
config_edc_snd_path_set(const char *edc_snd_path)
{
   config_data *cd = g_cd;

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
config_edc_data_path_set(const char *edc_data_path)
{
   config_data *cd = g_cd;

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
config_edc_fnt_path_set(const char *edc_fnt_path)
{
   config_data *cd = g_cd;

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
config_edc_img_path_set(const char *edc_img_path)
{
   config_data *cd = g_cd;

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
config_apply()
{
   config_data *cd = g_cd;
   if (cd->update_cb) cd->update_cb(cd->update_cb_data);
}

Eina_List *
config_edc_img_path_list_get()
{
   config_data *cd = g_cd;
   return cd->edc_img_path_list;
}

Eina_List *
config_edc_snd_path_list_get()
{
   config_data *cd = g_cd;
   return cd->edc_snd_path_list;
}

Eina_List *
config_edc_data_path_list_get()
{
   config_data *cd = g_cd;
   return cd->edc_data_path_list;
}

Eina_List *
config_edc_fnt_path_list_get()
{
   config_data *cd = g_cd;
   return cd->edc_fnt_path_list;
}

const char *
config_edc_img_path_get()
{
   config_data *cd = g_cd;
   if (!cd->edc_img_path_buf) return NULL;
   return eina_strbuf_string_get(cd->edc_img_path_buf);
}

const char *
config_edc_snd_path_get()
{
   config_data *cd = g_cd;
   if (!cd->edc_snd_path_buf) return NULL;
   return eina_strbuf_string_get(cd->edc_snd_path_buf);
}

const char *
config_edc_data_path_get()
{
   config_data *cd = g_cd;
   if (!cd->edc_data_path_buf) return NULL;
   return eina_strbuf_string_get(cd->edc_data_path_buf);
}

const char *
config_edc_fnt_path_get()
{
   config_data *cd = g_cd;
   if (!cd->edc_fnt_path_buf) return NULL;
   return eina_strbuf_string_get(cd->edc_fnt_path_buf);
}

const char *
config_edc_path_get()
{
   config_data *cd = g_cd;
   return cd->edc_path;
}

const char *
config_edj_path_get()
{
   config_data *cd = g_cd;
   return cd->edj_path;
}

Eina_Bool
config_linenumber_get()
{
   config_data *cd = g_cd;
   return cd->linenumber;
}

Eina_Bool
config_stats_bar_get()
{
   config_data *cd = g_cd;
   return cd->stats_bar;
}

void
config_linenumber_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   cd->linenumber = enabled;
}

void
config_stats_bar_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   cd->stats_bar = enabled;
}

void
config_update_cb_set(void (*cb)(void *data), void *data)
{
   config_data *cd = g_cd;
   cd->update_cb = cb;
   cd->update_cb_data = data;
}

Eina_Bool
config_part_highlight_get()
{
   config_data *cd = g_cd;
   return cd->part_highlight;
}

void
config_part_highlight_set(Eina_Bool highlight)
{
   config_data *cd = g_cd;
   cd->part_highlight = highlight;
}

Eina_Bool
config_dummy_swallow_get()
{
   config_data *cd = g_cd;
   return cd->dummy_swallow;
}

void
config_dummy_swallow_set(Eina_Bool dummy_swallow)
{
   config_data *cd = g_cd;
   cd->dummy_swallow = dummy_swallow;
}

Eina_Bool
config_auto_indent_get()
{
   config_data *cd = g_cd;
   return cd->auto_indent;
}

void
config_font_size_set(float font_size)
{
   config_data *cd = g_cd;

   if (font_size > MAX_FONT_SIZE)
     font_size = MAX_FONT_SIZE;
   else if (font_size < MIN_FONT_SIZE)
     font_size = MIN_FONT_SIZE;

   cd->font_size = font_size;
}

float
config_font_size_get()
{
   config_data *cd = g_cd;
   return cd->font_size;
}

void
config_auto_indent_set(Eina_Bool auto_indent)
{
   config_data *cd = g_cd;
   cd->auto_indent = auto_indent;
}

void
config_view_scale_set(double view_scale)
{
   config_data *cd = g_cd;

   if (view_scale > MAX_VIEW_SCALE)
     view_scale = MAX_VIEW_SCALE;
   else if (view_scale < MIN_VIEW_SCALE)
     view_scale = MIN_VIEW_SCALE;
   cd->view_scale = view_scale;
}

double
config_view_scale_get()
{
   config_data *cd = g_cd;
   return cd->view_scale;
}

void
config_view_size_set(Evas_Coord w, Evas_Coord h)
{
   config_data *cd = g_cd;

   cd->view_size.w = w;
   cd->view_size.h = h;
}

void
config_view_size_get(Evas_Coord *w, Evas_Coord *h)
{
   config_data *cd = g_cd;

   if (w) *w = cd->view_size.w;
   if (h) *h = cd->view_size.h;
}

Eina_Bool
config_hotkeys_get()
{
   config_data *cd = g_cd;
   return cd->hotkeys;
}

void
config_hotkeys_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   cd->hotkeys = enabled;
}
