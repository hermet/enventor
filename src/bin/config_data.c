#include "common.h"

typedef struct config_s
{
   const char *edc_path;
   const char *edj_path;
   const char *font_name;
   const char *font_style;

   Eina_List *edc_img_path_list;
   Eina_List *edc_snd_path_list;
   Eina_List *edc_fnt_path_list;
   Eina_List *edc_dat_path_list;
   Eina_Strbuf *edc_img_path_buf; //pre-stored image paths for edc compile.
   Eina_Strbuf *edc_snd_path_buf; //pre-stored sound paths for edc compile.
   Eina_Strbuf *edc_fnt_path_buf; //pre-stored font paths for edc compile.
   Eina_Strbuf *edc_dat_path_buf; //pre-stored data paths for edc compile.

   Eina_List *syntax_color_list;

   unsigned int version;
   float font_scale;
   double view_scale;
   double console_size;

   void (*update_cb)(void *data);
   void *update_cb_data;
   Evas_Coord view_size_w, view_size_h;
   Evas_Coord win_size_w, win_size_h;

   Eina_Bool stats_bar;
   Eina_Bool linenumber;
   Eina_Bool part_highlight;
   Eina_Bool dummy_swallow;
   Eina_Bool auto_indent;
   Eina_Bool tools;
   Eina_Bool console;
   Eina_Bool auto_complete;
   Eina_Bool view_size_configurable;
} config_data;

static config_data *g_cd = NULL;
static Eet_Data_Descriptor *edd_base = NULL;

static void
config_edj_path_update(config_data *cd)
{
   //apply edj path also
   char buf[PATH_MAX];
   Eina_Tmpstr *tmp_path;

   char *ext = strstr(cd->edc_path, ".edc");
   const char *file = ecore_file_file_get(cd->edc_path);
   if (ext && file)
     {
        char filename[PATH_MAX];
        snprintf(filename, (ext - file) + 1, "%s", file);
        snprintf(buf, sizeof(buf), "%s.edj", filename);
     }
   else
     snprintf(buf, sizeof(buf), "%s.edj", file);

   if (!eina_file_mkstemp(buf, &tmp_path))
     {
        EINA_LOG_ERR("Failed to generate tmp folder!");
        return;
     }

   eina_stringshare_replace(&cd->edj_path, tmp_path);
   eina_tmpstr_del(tmp_path);
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

   eet_data_write(ef, edd_base, "config", cd, 1);
   eet_close(ef);
}

static Eina_Strbuf *
config_paths_buf_set(Eina_List *paths, const char *prefix)
{
   Eina_List *l;
   const char *s;

   Eina_Strbuf *buf = eina_strbuf_new();

   EINA_LIST_FOREACH(paths, l, s)
     {
        eina_strbuf_append(buf, prefix);
        eina_strbuf_append(buf, s);
     }

   return buf;
}

static config_data *
config_load(void)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/enventor/config.eet",
            efreet_config_home_get());

   config_data *cd = NULL;
   Eet_File *ef = eet_open(buf, EET_FILE_MODE_READ);
   if (ef)
     {
        cd = eet_data_read(ef, edd_base, "config");
        eet_close(ef);
     }
   else EINA_LOG_WARN("Cannot load a config file \"%s\"", buf);

   //failed to load config file, create default structure.
   if (!cd)
     {
        cd = calloc(1, sizeof(config_data));
        if (!cd)
          {
             EINA_LOG_ERR("Failed to allocate Memory!");
             return NULL;
          }
     }
   // loaded config is not compatile with current version of Enventor
   if (cd->version < ENVENTOR_CONFIG_VERSION)
     {
        cd->edc_img_path_list = NULL;
        cd->edc_snd_path_list = NULL;
        cd->edc_fnt_path_list = NULL;
        cd->edc_dat_path_list = NULL;
        cd->font_scale = 1.0f;
        cd->view_scale = 1;
        cd->console_size = DEFAULT_CONSOLE_SIZE;
        cd->stats_bar = EINA_TRUE;
        cd->linenumber = EINA_TRUE;
        cd->part_highlight = EINA_TRUE;
        cd->dummy_swallow = EINA_TRUE;
        cd->auto_indent = EINA_TRUE;
        cd->tools = EINA_TRUE;
        cd->console = EINA_TRUE;
        cd->auto_complete = EINA_TRUE;
        cd->view_size_configurable = EINA_FALSE;
        cd->version = ENVENTOR_CONFIG_VERSION;
     }

   g_cd = cd;

   if (!cd->edc_img_path_list)
     {
        sprintf(buf, "%s/images", elm_app_data_dir_get());
        config_edc_img_path_set(buf);
     }
   else cd->edc_img_path_buf =
     config_paths_buf_set(cd->edc_img_path_list, " -id ");

   if (!cd->edc_snd_path_list)
     {
        sprintf(buf, "%s/sounds", elm_app_data_dir_get());
        config_edc_snd_path_set(buf);
     }
   else cd->edc_snd_path_buf =
     config_paths_buf_set(cd->edc_snd_path_list, " -sd ");

   if (!cd->edc_fnt_path_list)
     {
        sprintf(buf, "%s/fonts", elm_app_data_dir_get());
        config_edc_fnt_path_set(buf);
     }
   else cd->edc_fnt_path_buf =
     config_paths_buf_set(cd->edc_fnt_path_list, " -fd ");

   if (!cd->edc_dat_path_list)
     {
        sprintf(buf, "%s/data", elm_app_data_dir_get());
        config_edc_dat_path_set(buf);
     }
   else cd->edc_dat_path_buf =
     config_paths_buf_set(cd->edc_dat_path_list, " -dd ");

   if (!cd->syntax_color_list)
     {
        Enventor_Syntax_Color_Type color_type = ENVENTOR_SYNTAX_COLOR_STRING;
        for (; color_type < ENVENTOR_SYNTAX_COLOR_LAST; color_type++)
          cd->syntax_color_list = eina_list_append(cd->syntax_color_list, NULL);
     }

   return cd;
}

static void
eddc_init(void)
{
   Eet_Data_Descriptor_Class eddc;
   eet_eina_stream_data_descriptor_class_set(&eddc, sizeof(eddc),
                                             "config", sizeof(config_data));
   edd_base = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(edd_base, config_data,
                                       "edc_img_path_list", edc_img_path_list);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(edd_base, config_data,
                                       "edc_snd_path_list", edc_snd_path_list);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(edd_base, config_data,
                                       "edc_fnt_path_list", edc_fnt_path_list);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(edd_base, config_data,
                                       "edc_dat_path_list", edc_dat_path_list);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(edd_base, config_data,
                                       "syntax_color_list", syntax_color_list);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "version", version, EET_T_UINT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "font_name", font_name,
                                 EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "font_style", font_style,
                                 EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "font_scale", font_scale,
                                 EET_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "view_scale",
                                 view_scale, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "console_size",
                                 console_size, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "view_size_w",
                                 view_size_w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "view_size_h",
                                 view_size_h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "win_size_w",
                                 win_size_w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "win_size_h",
                                 win_size_h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "stats_bar", stats_bar,
                                 EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "linenumber",
                                 linenumber, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "part_highlight",
                                 part_highlight, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "dummy_swallow",
                                 dummy_swallow, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "auto_indent",
                                 auto_indent, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "tools",
                                 tools, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "console",
                                 console, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "auto_complete",
                                    auto_complete, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data,
                                 "view_size_configurable",
                                 view_size_configurable, EET_T_UCHAR);
}

void
config_edc_path_set(const char *edc_path)
{
   config_data *cd = g_cd;
   eina_stringshare_replace(&cd->edc_path, edc_path);
   config_edj_path_update(cd);
}

void
config_init(const char *edc_path, const char *edj_path,
            Eina_List *edc_img_path, Eina_List *edc_snd_path,
            Eina_List *edc_fnt_path, Eina_List *edc_dat_path)
{
   Eina_Stringshare *s;
   eddc_init();

   config_data *cd = config_load();
   g_cd = cd;

   if (edc_path[0]) config_edc_path_set(edc_path);
   if (edj_path[0])
      eina_stringshare_replace(&cd->edj_path, edj_path);

   if (edc_img_path)
     g_cd->edc_img_path_list = edc_img_path;

   if (edc_snd_path)
     g_cd->edc_snd_path_list = edc_snd_path;

   if (edc_fnt_path)
     g_cd->edc_fnt_path_list = edc_fnt_path;

   if (edc_dat_path)
     g_cd->edc_dat_path_list = edc_dat_path;

}

void
config_term(void)
{
   config_data *cd = g_cd;

   config_save(cd);

   eina_stringshare_del(cd->edc_path);
   eina_stringshare_del(cd->edj_path);

   Eina_Stringshare *str;
   EINA_LIST_FREE(cd->edc_img_path_list, str) eina_stringshare_del(str);
   EINA_LIST_FREE(cd->edc_snd_path_list, str) eina_stringshare_del(str);
   EINA_LIST_FREE(cd->edc_fnt_path_list, str) eina_stringshare_del(str);
   EINA_LIST_FREE(cd->edc_dat_path_list, str) eina_stringshare_del(str);

   EINA_LIST_FREE(cd->syntax_color_list, str) eina_stringshare_del(str);

   if (cd->edc_img_path_buf) eina_strbuf_free(cd->edc_img_path_buf);
   if (cd->edc_snd_path_buf) eina_strbuf_free(cd->edc_snd_path_buf);
   if (cd->edc_fnt_path_buf) eina_strbuf_free(cd->edc_fnt_path_buf);
   if (cd->edc_dat_path_buf) eina_strbuf_free(cd->edc_dat_path_buf);

   eet_data_descriptor_free(edd_base);
   free(cd);
}

void
config_edc_snd_path_set(const char *edc_snd_path)
{
   config_data *cd = g_cd;

   //Free the existing paths
   const char *s;
   EINA_LIST_FREE(cd->edc_snd_path_list, s) eina_stringshare_del(s);

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
config_edc_dat_path_set(const char *edc_dat_path)
{
   config_data *cd = g_cd;

   //Free the existing paths
   const char *s;
   EINA_LIST_FREE(cd->edc_dat_path_list, s) eina_stringshare_del(s);

   if (cd->edc_dat_path_buf) eina_strbuf_free(cd->edc_dat_path_buf);
   cd->edc_dat_path_buf = eina_strbuf_new();

   //parse paths by ';'
   const char *lex;
   Eina_Stringshare *append;

   while(edc_dat_path && (strlen(edc_dat_path) > 0))
     {
        lex = strstr(edc_dat_path, ";");
        if (lex)
          {
             append = eina_stringshare_add_length(edc_dat_path,
                                                  (lex - edc_dat_path));
             cd->edc_dat_path_list = eina_list_append(cd->edc_dat_path_list,
                                                      append);
             eina_strbuf_append(cd->edc_dat_path_buf, " -dd ");
             eina_strbuf_append(cd->edc_dat_path_buf, append);
             lex++;
          }
        else
          {
             append = eina_stringshare_add(edc_dat_path);
             cd->edc_dat_path_list = eina_list_append(cd->edc_dat_path_list,
                                                      append);
             eina_strbuf_append(cd->edc_dat_path_buf, " -dd ");
             eina_strbuf_append(cd->edc_dat_path_buf, append);
          }

        edc_dat_path = lex;
     }
}

void
config_edc_fnt_path_set(const char *edc_fnt_path)
{
   config_data *cd = g_cd;

   //Free the existing paths
   const char *s;
   EINA_LIST_FREE(cd->edc_fnt_path_list, s) eina_stringshare_del(s);

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
   const char *s;
   EINA_LIST_FREE(cd->edc_img_path_list, s) eina_stringshare_del(s);

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
config_apply(void)
{
   config_data *cd = g_cd;
   if (cd->update_cb) cd->update_cb(cd->update_cb_data);
}

Eina_List *
config_edc_img_path_list_get(void)
{
   config_data *cd = g_cd;
   return cd->edc_img_path_list;
}

Eina_List *
config_edc_snd_path_list_get(void)
{
   config_data *cd = g_cd;
   return cd->edc_snd_path_list;
}

Eina_List *
config_edc_dat_path_list_get(void)
{
   config_data *cd = g_cd;
   return cd->edc_dat_path_list;
}

Eina_List *
config_edc_fnt_path_list_get(void)
{
   config_data *cd = g_cd;
   return cd->edc_fnt_path_list;
}

const char *
config_edc_img_path_get(void)
{
   config_data *cd = g_cd;
   if (!cd->edc_img_path_buf) return NULL;
   return eina_strbuf_string_get(cd->edc_img_path_buf);
}

const char *
config_edc_snd_path_get(void)
{
   config_data *cd = g_cd;
   if (!cd->edc_snd_path_buf) return NULL;
   return eina_strbuf_string_get(cd->edc_snd_path_buf);
}

const char *
config_edc_dat_path_get(void)
{
   config_data *cd = g_cd;
   if (!cd->edc_dat_path_buf) return NULL;
   return eina_strbuf_string_get(cd->edc_dat_path_buf);
}

const char *
config_edc_fnt_path_get(void)
{
   config_data *cd = g_cd;
   if (!cd->edc_fnt_path_buf) return NULL;
   return eina_strbuf_string_get(cd->edc_fnt_path_buf);
}

const char *
config_edc_path_get(void)
{
   config_data *cd = g_cd;
   return cd->edc_path;
}

const char *
config_edj_path_get(void)
{
   config_data *cd = g_cd;
   return cd->edj_path;
}

void
config_syntax_color_set(Enventor_Syntax_Color_Type color_type,
                        const char *val)
{
   config_data *cd = g_cd;
   Eina_List *target_list;

   target_list = eina_list_nth_list(cd->syntax_color_list, color_type);
   if (!target_list) return;

   eina_stringshare_del(eina_list_data_get(target_list));
   if (val)
     eina_list_data_set(target_list, eina_stringshare_add(val));
}

const char *
config_syntax_color_get(Enventor_Syntax_Color_Type color_type)
{
   config_data *cd = g_cd;
   return (const char *) eina_list_nth(cd->syntax_color_list, color_type);
}

Eina_Bool
config_linenumber_get(void)
{
   config_data *cd = g_cd;
   return cd->linenumber;
}

Eina_Bool
config_stats_bar_get(void)
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
config_part_highlight_get(void)
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
config_dummy_swallow_get(void)
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
config_auto_indent_get(void)
{
   config_data *cd = g_cd;
   return cd->auto_indent;
}

Eina_Bool
config_auto_complete_get(void)
{
   config_data *cd = g_cd;
   return cd->auto_complete;
}

void
config_font_set(const char *font_name, const char *font_style)
{
   config_data *cd = g_cd;

   eina_stringshare_replace(&cd->font_name, font_name);
   eina_stringshare_replace(&cd->font_style, font_style);
}

void
config_font_get(const char **font_name, const char **font_style)
{
   config_data *cd = g_cd;

   if (font_name) *font_name = cd->font_name;
   if (font_style) *font_style = cd->font_style;
}

void
config_font_scale_set(float font_scale)
{
   config_data *cd = g_cd;

   if (font_scale > MAX_FONT_SCALE)
     font_scale = MAX_FONT_SCALE;
   else if (font_scale < MIN_FONT_SCALE)
     font_scale = MIN_FONT_SCALE;

   cd->font_scale = font_scale;
}

float
config_font_scale_get(void)
{
   config_data *cd = g_cd;
   return cd->font_scale;
}

void
config_auto_complete_set(Eina_Bool auto_complete)
{
   config_data *cd = g_cd;
   cd->auto_complete = auto_complete;
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
config_view_scale_get(void)
{
   config_data *cd = g_cd;
   return cd->view_scale;
}

void
config_view_size_set(Evas_Coord w, Evas_Coord h)
{
   config_data *cd = g_cd;

   cd->view_size_w = w;
   cd->view_size_h = h;
}

void
config_view_size_get(Evas_Coord *w, Evas_Coord *h)
{
   config_data *cd = g_cd;

   if (w) *w = cd->view_size_w;
   if (h) *h = cd->view_size_h;
}

void
config_view_size_configurable_set(Eina_Bool configurable)
{
   config_data *cd = g_cd;
   cd->view_size_configurable = configurable;
}

Eina_Bool
config_view_size_configurable_get(void)
{
   config_data *cd = g_cd;
   return cd->view_size_configurable;
}

double
config_console_size_get(void)
{
   config_data *cd = g_cd;
   return cd->console_size;
}

void
config_console_size_set(double size)
{
   config_data *cd = g_cd;
   cd->console_size = size;
}

void
config_win_size_set(Evas_Coord w, Evas_Coord h)
{
   config_data *cd = g_cd;

   cd->win_size_w = w;
   cd->win_size_h = h;
}

void
config_win_size_get(Evas_Coord *w, Evas_Coord *h)
{
   config_data *cd = g_cd;

   if (w) *w = cd->win_size_w;
   if (h) *h = cd->win_size_h;
}

Eina_Bool
config_console_get(void)
{
   config_data *cd = g_cd;
   return cd->console;
}

void
config_console_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   cd->console = enabled;
}

Eina_Bool
config_tools_get(void)
{
   config_data *cd = g_cd;
   return cd->tools;
}

void
config_tools_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   cd->tools = enabled;
}
