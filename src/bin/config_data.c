#include "common.h"

typedef struct config_s
{
   const char *input_path;
   const char *output_path;
   const char *workspace_path;
   const char *font_name;
   const char *font_style;

   Eina_List *img_path_list;
   Eina_List *snd_path_list;
   Eina_List *fnt_path_list;
   Eina_List *dat_path_list;
   Eina_Strbuf *img_path_buf; //pre-stored image paths for compile.
   Eina_Strbuf *snd_path_buf; //pre-stored sound paths for compile.
   Eina_Strbuf *fnt_path_buf; //pre-stored font paths for compile.
   Eina_Strbuf *dat_path_buf; //pre-stored data paths for compile.

   Eina_List *syntax_color_list;

   unsigned int version;
   float font_scale;
   double view_scale;
   double editor_size;
   double console_size;

   void (*update_cb)(void *data);
   void *update_cb_data;
   Evas_Coord view_size_w, view_size_h;
   Evas_Coord win_size_w, win_size_h;

   Eina_Bool stats_bar;
   Eina_Bool linenumber;
   Eina_Bool part_highlight;
   Eina_Bool dummy_parts;
   Eina_Bool outline;
   Eina_Bool mirror_mode;
   Eina_Bool auto_indent;
   Eina_Bool tools;
   Eina_Bool console;
   Eina_Bool auto_complete;
   Eina_Bool smart_undo_redo;
   Eina_Bool file_browser;
   Eina_Bool file_browser_loaded;
   Eina_Bool edc_navigator;
   Eina_Bool red_alert;
   Eina_Bool file_tab;
} config_data;

static config_data *g_cd = NULL;
static Eet_Data_Descriptor *edd_base = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
config_edj_path_update(config_data *cd)
{
   //apply edj path also
   char buf[PATH_MAX];
   Eina_Tmpstr *tmp_path;

   char *ext = strstr(cd->input_path, ".edc");
   const char *file = ecore_file_file_get(cd->input_path);
   if (ext && file)
     {
        char filename[PATH_MAX];
        snprintf(filename, (ext - file) + 1, "%s", file);
        snprintf(buf, sizeof(buf), "%s_XXXXXX.edj", filename);
     }
   else
     snprintf(buf, sizeof(buf), "%s_XXXXXX.edj", file);

   if (!eina_file_mkstemp(buf, &tmp_path))
     {
        EINA_LOG_ERR(_("Failed to generate tmp folder!"));
        return;
     }

   eina_stringshare_replace(&cd->output_path, tmp_path);
   eina_tmpstr_del(tmp_path);
}

static void
config_save(config_data *cd)
{
   char buf[PATH_MAX];

   //Create config home directory if it doesn't exist.
   if (!ecore_file_exists(efreet_config_home_get()))
     {
        Eina_Bool success = ecore_file_mkdir(efreet_config_home_get());
        if (!success)
          {
             EINA_LOG_ERR(_("Cannot create a config folder \"%s\""), efreet_config_home_get());
             return;
          }
     }

   snprintf(buf, sizeof(buf), "%s/enventor", efreet_config_home_get());

   //Create enventor config folder if it doesn't exist.
   if (!ecore_file_exists(buf))
     {
        Eina_Bool success = ecore_file_mkdir(buf);
        if (!success)
          {
             EINA_LOG_ERR(_("Cannot create a config folder \"%s\""), buf);
             return;
          }
     }

   //Save config file.
   snprintf(buf, sizeof(buf), "%s/enventor/config.eet",
            efreet_config_home_get());
   Eet_File *ef = eet_open(buf, EET_FILE_MODE_WRITE);
   if (!ef)
     {
        EINA_LOG_ERR(_("Cannot save a config file \"%s\""), buf);
        return;
     }

   //Restore loaded file browser config.
   cd->file_browser = cd->file_browser_loaded;

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
   else EINA_LOG_WARN(_("Cannot load a config file \"%s\""), buf);

   //failed to load config file, create default structure.
   if (!cd)
     {
        cd = calloc(1, sizeof(config_data));
        if (!cd)
          {
             EINA_LOG_ERR(_("Failed to allocate Memory!"));
             return NULL;
          }
     }
   // loaded config is not compatile with current version of Enventor
   if (!ef || cd->version < ENVENTOR_CONFIG_VERSION)
     {
        cd->img_path_list = NULL;
        cd->snd_path_list = NULL;
        cd->fnt_path_list = NULL;
        cd->dat_path_list = NULL;
        cd->font_scale = 1;
        cd->view_scale = 1;
        cd->view_size_w = 300;
        cd->view_size_h = 300;
        cd->win_size_w = WIN_DEFAULT_W;
        cd->win_size_h = WIN_DEFAULT_H;
        cd->editor_size = DEFAULT_EDITOR_SIZE;
        cd->console_size = DEFAULT_CONSOLE_SIZE;
        cd->stats_bar = EINA_TRUE;
        cd->linenumber = EINA_TRUE;
        cd->part_highlight = EINA_TRUE;
        cd->dummy_parts = EINA_TRUE;
        cd->outline = EINA_FALSE;
        cd->mirror_mode = EINA_FALSE;
        cd->auto_indent = EINA_TRUE;
        cd->tools = EINA_TRUE;
        cd->console = EINA_TRUE;
        cd->auto_complete = EINA_TRUE;
        cd->version = ENVENTOR_CONFIG_VERSION;
        cd->smart_undo_redo = EINA_FALSE;
        cd->file_browser = EINA_FALSE;
        cd->edc_navigator = EINA_TRUE;
        cd->red_alert = EINA_TRUE;
        cd->file_tab = EINA_FALSE;
     }

   g_cd = cd;

   if (!cd->img_path_list)
     {
        snprintf(buf, sizeof(buf), "%s/images", elm_app_data_dir_get());
        config_img_path_set(buf);
     }
   else cd->img_path_buf =
     config_paths_buf_set(cd->img_path_list, " -id ");

   if (!cd->snd_path_list)
     {
        snprintf(buf, sizeof(buf), "%s/sounds", elm_app_data_dir_get());
        config_snd_path_set(buf);
     }
   else cd->snd_path_buf =
     config_paths_buf_set(cd->snd_path_list, " -sd ");

   if (!cd->fnt_path_list)
     {
        snprintf(buf, sizeof(buf), "%s/fonts", elm_app_data_dir_get());
        config_fnt_path_set(buf);
     }
   else cd->fnt_path_buf =
     config_paths_buf_set(cd->fnt_path_list, " -fd ");

   if (!cd->dat_path_list)
     {
        snprintf(buf, sizeof(buf), "%s/data", elm_app_data_dir_get());
        config_dat_path_set(buf);
     }
   else cd->dat_path_buf =
     config_paths_buf_set(cd->dat_path_list, " -dd ");

   if (!cd->syntax_color_list)
     {
        Enventor_Syntax_Color_Type color_type = ENVENTOR_SYNTAX_COLOR_STRING;
        for (; color_type < ENVENTOR_SYNTAX_COLOR_LAST; color_type++)
          cd->syntax_color_list = eina_list_append(cd->syntax_color_list, NULL);
     }

#ifdef _WIN32
   const char monospace_font[] = "Courier New";
#elif __APPLE__
   const char monospace_font[] = "Menlo";
#else
   const char monospace_font[] = "Ubuntu Mono";
#endif
   if (!cd->font_name)
     eina_stringshare_replace(&cd->font_name, monospace_font);
   if (!cd->font_style)
     eina_stringshare_replace(&cd->font_style, "Regular");

   //Store loaded file browser config.
   cd->file_browser_loaded = cd->file_browser;

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
                                       "img_path_list", img_path_list);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(edd_base, config_data,
                                       "snd_path_list", snd_path_list);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(edd_base, config_data,
                                       "fnt_path_list", fnt_path_list);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(edd_base, config_data,
                                       "dat_path_list", dat_path_list);
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
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "editor_size",
                                 editor_size, EET_T_DOUBLE);
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
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "dummy_parts",
                                 dummy_parts, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "outline",
                                 outline, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "mirror_mode",
                                 mirror_mode, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "auto_indent",
                                 auto_indent, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "tools",
                                 tools, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "console",
                                 console, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "auto_complete",
                                    auto_complete, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "smart_undo_redo",
                                    smart_undo_redo, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "file_browser",
                                    file_browser, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "edc_navigator",
                                    edc_navigator, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "red_alert",
                                    red_alert, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd_base, config_data, "file_tab", file_tab,
                                 EET_T_UCHAR);
}

void
config_input_path_set(const char *input_path)
{
   config_data *cd = g_cd;
   eina_stringshare_replace(&cd->input_path, input_path);
   config_edj_path_update(cd);
}

Eina_Bool
config_init(const char *input_path, const char *output_path,
            const char *workspace_path,
            Eina_List *img_path, Eina_List *snd_path,
            Eina_List *fnt_path, Eina_List *dat_path)
{
   eddc_init();

   config_data *cd = config_load();
   if (!cd) return EINA_FALSE;
   g_cd = cd;

   if (input_path[0]) config_input_path_set(input_path);
   if (output_path[0]) eina_stringshare_replace(&cd->output_path, output_path);

   if (workspace_path[0])
     eina_stringshare_replace(&cd->workspace_path, workspace_path);
   else
     g_cd->file_browser = EINA_FALSE;

   if (img_path)
     g_cd->img_path_list = img_path;

   if (snd_path)
     g_cd->snd_path_list = snd_path;

   if (fnt_path)
     g_cd->fnt_path_list = fnt_path;

   if (dat_path)
     g_cd->dat_path_list = dat_path;

   return EINA_TRUE;
}

void
config_term(void)
{
   config_data *cd = g_cd;

   config_save(cd);

   eina_stringshare_del(cd->input_path);
   eina_stringshare_del(cd->output_path);
   eina_stringshare_del(cd->workspace_path);

   Eina_Stringshare *str;
   EINA_LIST_FREE(cd->img_path_list, str) eina_stringshare_del(str);
   EINA_LIST_FREE(cd->snd_path_list, str) eina_stringshare_del(str);
   EINA_LIST_FREE(cd->fnt_path_list, str) eina_stringshare_del(str);
   EINA_LIST_FREE(cd->dat_path_list, str) eina_stringshare_del(str);

   EINA_LIST_FREE(cd->syntax_color_list, str) eina_stringshare_del(str);

   if (cd->img_path_buf) eina_strbuf_free(cd->img_path_buf);
   if (cd->snd_path_buf) eina_strbuf_free(cd->snd_path_buf);
   if (cd->fnt_path_buf) eina_strbuf_free(cd->fnt_path_buf);
   if (cd->dat_path_buf) eina_strbuf_free(cd->dat_path_buf);

   eet_data_descriptor_free(edd_base);
   free(cd);
}

void
config_snd_path_set(const char *snd_path)
{
   config_data *cd = g_cd;

   //Free the existing paths
   const char *s;
   EINA_LIST_FREE(cd->snd_path_list, s) eina_stringshare_del(s);

   if (cd->snd_path_buf) eina_strbuf_free(cd->snd_path_buf);
   cd->snd_path_buf = eina_strbuf_new();

   //parse paths by ';'
   const char *lex;
   Eina_Stringshare *append;

   while(snd_path && (strlen(snd_path) > 0))
     {
        lex = strstr(snd_path, ";");
        if (lex)
          {
             append = eina_stringshare_add_length(snd_path,
                                                  (lex - snd_path));
             cd->snd_path_list = eina_list_append(cd->snd_path_list,
                                                      append);
             eina_strbuf_append(cd->snd_path_buf, " -sd ");
             eina_strbuf_append(cd->snd_path_buf, append);
             lex++;
          }
        else
          {
             append = eina_stringshare_add(snd_path);
             cd->snd_path_list = eina_list_append(cd->snd_path_list,
                                                      append);
             eina_strbuf_append(cd->snd_path_buf, " -sd ");
             eina_strbuf_append(cd->snd_path_buf, append);
          }

        snd_path = lex;
     }
}

void
config_dat_path_set(const char *dat_path)
{
   config_data *cd = g_cd;

   //Free the existing paths
   const char *s;
   EINA_LIST_FREE(cd->dat_path_list, s) eina_stringshare_del(s);

   if (cd->dat_path_buf) eina_strbuf_free(cd->dat_path_buf);
   cd->dat_path_buf = eina_strbuf_new();

   //parse paths by ';'
   const char *lex;
   Eina_Stringshare *append;

   while(dat_path && (strlen(dat_path) > 0))
     {
        lex = strstr(dat_path, ";");
        if (lex)
          {
             append = eina_stringshare_add_length(dat_path,
                                                  (lex - dat_path));
             cd->dat_path_list = eina_list_append(cd->dat_path_list,
                                                      append);
             eina_strbuf_append(cd->dat_path_buf, " -dd ");
             eina_strbuf_append(cd->dat_path_buf, append);
             lex++;
          }
        else
          {
             append = eina_stringshare_add(dat_path);
             cd->dat_path_list = eina_list_append(cd->dat_path_list,
                                                      append);
             eina_strbuf_append(cd->dat_path_buf, " -dd ");
             eina_strbuf_append(cd->dat_path_buf, append);
          }

        dat_path = lex;
     }
}

void
config_fnt_path_set(const char *fnt_path)
{
   config_data *cd = g_cd;

   //Free the existing paths
   const char *s;
   EINA_LIST_FREE(cd->fnt_path_list, s) eina_stringshare_del(s);

   if (cd->fnt_path_buf) eina_strbuf_free(cd->fnt_path_buf);
   cd->fnt_path_buf = eina_strbuf_new();

   //parse paths by ';'
   const char *lex;
   Eina_Stringshare *append;

   while(fnt_path && (strlen(fnt_path) > 0))
     {
        lex = strstr(fnt_path, ";");
        if (lex)
          {
             append = eina_stringshare_add_length(fnt_path,
                                                  (lex - fnt_path));
             cd->fnt_path_list = eina_list_append(cd->fnt_path_list,
                                                      append);
             eina_strbuf_append(cd->fnt_path_buf, " -fd ");
             eina_strbuf_append(cd->fnt_path_buf, append);
             lex++;
          }
        else
          {
             append = eina_stringshare_add(fnt_path);
             cd->fnt_path_list = eina_list_append(cd->fnt_path_list,
                                                      append);
             eina_strbuf_append(cd->fnt_path_buf, " -fd ");
             eina_strbuf_append(cd->fnt_path_buf, append);
          }

        fnt_path = lex;
     }
}

void
config_img_path_set(const char *img_path)
{
   config_data *cd = g_cd;

   //Free the existing paths
   const char *s;
   EINA_LIST_FREE(cd->img_path_list, s) eina_stringshare_del(s);

   if (cd->img_path_buf) eina_strbuf_free(cd->img_path_buf);
   cd->img_path_buf = eina_strbuf_new();

   //parse paths by ';'
   const char *lex;
   Eina_Stringshare *append;

   while(img_path && (strlen(img_path) > 0))
     {
        lex = strstr(img_path, ";");
        if (lex)
          {
             append = eina_stringshare_add_length(img_path,
                                                  (lex - img_path));
             cd->img_path_list = eina_list_append(cd->img_path_list,
                                                      append);
             eina_strbuf_append(cd->img_path_buf, " -id ");
             eina_strbuf_append(cd->img_path_buf, append);
             lex++;
          }
        else
          {
             append = eina_stringshare_add(img_path);
             cd->img_path_list = eina_list_append(cd->img_path_list,
                                                      append);
             eina_strbuf_append(cd->img_path_buf, " -id ");
             eina_strbuf_append(cd->img_path_buf, append);
          }

        img_path = lex;
     }
}

void
config_apply(void)
{
   config_data *cd = g_cd;
   if (cd->update_cb) cd->update_cb(cd->update_cb_data);
}

Eina_List *
config_img_path_list_get(void)
{
   config_data *cd = g_cd;
   return cd->img_path_list;
}

Eina_List *
config_snd_path_list_get(void)
{
   config_data *cd = g_cd;
   return cd->snd_path_list;
}

Eina_List *
config_dat_path_list_get(void)
{
   config_data *cd = g_cd;
   return cd->dat_path_list;
}

Eina_List *
config_fnt_path_list_get(void)
{
   config_data *cd = g_cd;
   return cd->fnt_path_list;
}

const char *
config_img_path_get(void)
{
   config_data *cd = g_cd;
   if (!cd->img_path_buf) return NULL;
   return eina_strbuf_string_get(cd->img_path_buf);
}

const char *
config_snd_path_get(void)
{
   config_data *cd = g_cd;
   if (!cd->snd_path_buf) return NULL;
   return eina_strbuf_string_get(cd->snd_path_buf);
}

const char *
config_dat_path_get(void)
{
   config_data *cd = g_cd;
   if (!cd->dat_path_buf) return NULL;
   return eina_strbuf_string_get(cd->dat_path_buf);
}

const char *
config_fnt_path_get(void)
{
   config_data *cd = g_cd;
   if (!cd->fnt_path_buf) return NULL;
   return eina_strbuf_string_get(cd->fnt_path_buf);
}

const char *
config_input_path_get(void)
{
   config_data *cd = g_cd;
   return cd->input_path;
}

const char *
config_output_path_get(void)
{
   config_data *cd = g_cd;
   return cd->output_path;
}

const char *
config_workspace_path_get(void)
{
   config_data *cd = g_cd;
   return cd->workspace_path;
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

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

Eina_Bool
config_linenumber_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->linenumber;
}

void
config_linenumber_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->linenumber = enabled;
}

Eina_Bool
config_file_tab_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->file_tab;
}

void
config_file_tab_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->file_tab = enabled;
}

Eina_Bool
config_stats_bar_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->stats_bar;
}

void
config_stats_bar_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->stats_bar = enabled;
}

void
config_update_cb_set(void (*cb)(void *data), void *data)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->update_cb = cb;
   cd->update_cb_data = data;
}

Eina_Bool
config_part_highlight_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->part_highlight;
}

void
config_part_highlight_set(Eina_Bool highlight)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->part_highlight = highlight;
}

Eina_Bool
config_dummy_parts_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->dummy_parts;
}

void
config_dummy_parts_set(Eina_Bool dummy_parts)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->dummy_parts = dummy_parts;
}

Eina_Bool
config_parts_outline_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->outline;
}

void
config_parts_outline_set(Eina_Bool outline)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->outline = outline;
}

Eina_Bool
config_mirror_mode_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->mirror_mode;
}

void
config_mirror_mode_set(Eina_Bool mirror_mode)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->mirror_mode = mirror_mode;
}

Eina_Bool
config_auto_indent_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->auto_indent;
}

Eina_Bool
config_auto_complete_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->auto_complete;
}

void
config_font_set(const char *font_name, const char *font_style)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   eina_stringshare_replace(&cd->font_name, font_name);
   eina_stringshare_replace(&cd->font_style, font_style);
}

void
config_font_get(const char **font_name, const char **font_style)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   if (font_name) *font_name = cd->font_name;
   if (font_style) *font_style = cd->font_style;
}

void
config_font_scale_set(float font_scale)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

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
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, 0);

   return cd->font_scale;
}

Eina_Bool
config_smart_undo_redo_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->smart_undo_redo;
}

void
config_smart_undo_redo_set(Eina_Bool smart_undo_redo)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->smart_undo_redo = smart_undo_redo;
}

void
config_auto_complete_set(Eina_Bool auto_complete)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->auto_complete = auto_complete;
}

void
config_auto_indent_set(Eina_Bool auto_indent)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->auto_indent = auto_indent;
}

void
config_view_scale_set(double view_scale)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

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
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, 0);

   return cd->view_scale;
}

void
config_view_size_set(Evas_Coord w, Evas_Coord h)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->view_size_w = w;
   cd->view_size_h = h;
}

void
config_view_size_get(Evas_Coord *w, Evas_Coord *h)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   if (w) *w = cd->view_size_w;
   if (h) *h = cd->view_size_h;
}

double
config_editor_size_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   return cd->editor_size;
}

void
config_editor_size_set(double size)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->editor_size = size;
}

double
config_console_size_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, 0);

   return cd->console_size;
}

void
config_console_size_set(double size)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->console_size = size;
}

void
config_win_size_set(Evas_Coord w, Evas_Coord h)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->win_size_w = w;
   cd->win_size_h = h;
}

void
config_win_size_get(Evas_Coord *w, Evas_Coord *h)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   if (w) *w = cd->win_size_w;
   if (h) *h = cd->win_size_h;
}

Eina_Bool
config_console_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->console;
}

void
config_console_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->console = enabled;
}

Eina_Bool
config_tools_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->tools;
}

void
config_tools_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->tools = enabled;
}

void
config_red_alert_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->red_alert = enabled;
}

Eina_Bool
config_red_alert_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->red_alert;
}

void
config_file_browser_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->file_browser = enabled;
   cd->file_browser_loaded = enabled;
}

Eina_Bool
config_file_browser_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->file_browser;
}

void
config_edc_navigator_set(Eina_Bool enabled)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN(cd);

   cd->edc_navigator = enabled;
}

Eina_Bool
config_edc_navigator_get(void)
{
   config_data *cd = g_cd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, EINA_FALSE);

   return cd->edc_navigator;
}
