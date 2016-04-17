#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "common.h"

typedef struct build_setting_s
{
   Evas_Object *layout;
   Evas_Object *img_path_entry;
   Evas_Object *snd_path_entry;
   Evas_Object *fnt_path_entry;
   Evas_Object *dat_path_entry;

} build_setting_data;

static build_setting_data *g_bsd = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static Evas_Object *
entry_create(Evas_Object *parent)
{
   Evas_Object *entry = elm_entry_add(parent);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   evas_object_show(entry);

   return entry;
}

static void
img_path_entry_update(Evas_Object *entry, Eina_List *edc_img_paths)
{
   elm_entry_entry_set(entry, NULL);

   Eina_List *l;
   char *edc_img_path;
   EINA_LIST_FOREACH(edc_img_paths, l, edc_img_path)
     {
        elm_entry_entry_append(entry, edc_img_path);
        elm_entry_entry_append(entry, ";");
     }
}

static void
fnt_path_entry_update(Evas_Object *entry, Eina_List *edc_fnt_paths)
{
   elm_entry_entry_set(entry, NULL);

   Eina_List *l;
   char *edc_fnt_path;
   EINA_LIST_FOREACH(edc_fnt_paths, l, edc_fnt_path)
     {
        elm_entry_entry_append(entry, edc_fnt_path);
        elm_entry_entry_append(entry, ";");
     }
}

static void
dat_path_entry_update(Evas_Object *entry, Eina_List *edc_dat_paths)
{
   elm_entry_entry_set(entry, NULL);

   Eina_List *l;
   char *edc_dat_path;
   EINA_LIST_FOREACH(edc_dat_paths, l, edc_dat_path)
     {
        elm_entry_entry_append(entry, edc_dat_path);
        elm_entry_entry_append(entry, ";");
     }
}

static void
snd_path_entry_update(Evas_Object *entry, Eina_List *edc_snd_paths)
{
   elm_entry_entry_set(entry, NULL);

   Eina_List *l;
   char *edc_snd_path;
   EINA_LIST_FOREACH(edc_snd_paths, l, edc_snd_path)
     {
        elm_entry_entry_append(entry, edc_snd_path);
        elm_entry_entry_append(entry, ";");
     }
}


/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/
void
build_setting_focus_set(build_setting_data *bsd)
{
   if (!bsd) return;
   elm_object_focus_set(bsd->img_path_entry, EINA_TRUE);
}

void
build_setting_config_set(build_setting_data *bsd)
{
   if (!bsd) return;

   config_img_path_set(elm_object_text_get(bsd->img_path_entry));
   config_snd_path_set(elm_object_text_get(bsd->snd_path_entry));
   config_fnt_path_set(elm_object_text_get(bsd->fnt_path_entry));
   config_dat_path_set(elm_object_text_get(bsd->dat_path_entry));
}

void
build_setting_reset(build_setting_data *bsd)
{
   if (!bsd) return;

   img_path_entry_update(bsd->img_path_entry,
                         (Eina_List *)config_img_path_list_get());
   snd_path_entry_update(bsd->snd_path_entry,
                         (Eina_List *)config_snd_path_list_get());
   fnt_path_entry_update(bsd->fnt_path_entry,
                         (Eina_List *)config_fnt_path_list_get());
   dat_path_entry_update(bsd->dat_path_entry,
                         (Eina_List *)config_dat_path_list_get());
}

Evas_Object *
build_setting_content_get(build_setting_data *bsd, Evas_Object *parent)
{
   if (!bsd) return NULL;
   if (bsd->layout) return bsd->layout;

   //Layout
   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "build_setting_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);

   //Image Path Entry
   Evas_Object *img_path_entry = entry_create(layout);
   img_path_entry_update(img_path_entry,
                         (Eina_List *)config_img_path_list_get());
   elm_object_focus_set(img_path_entry, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.img_path_entry",
                               img_path_entry);
   elm_layout_text_set(layout, "img_path_guide", _("Image Paths:"));

   //Sound Path Entry
   Evas_Object *snd_path_entry = entry_create(layout);
   snd_path_entry_update(snd_path_entry,
                         (Eina_List *)config_snd_path_list_get());
   elm_object_part_content_set(layout, "elm.swallow.snd_path_entry",
                               snd_path_entry);
   elm_layout_text_set(layout, "snd_path_guide", _("Sound Paths:"));

   //Font Path Entry
   Evas_Object *fnt_path_entry = entry_create(layout);
   fnt_path_entry_update(fnt_path_entry,
                         (Eina_List *)config_fnt_path_list_get());
   elm_object_part_content_set(layout, "elm.swallow.fnt_path_entry",
                               fnt_path_entry);
   elm_layout_text_set(layout, "fnt_path_guide", _("Font Paths:"));

   //Data Path Entry
   Evas_Object *dat_path_entry = entry_create(layout);
   dat_path_entry_update(dat_path_entry,
                         (Eina_List *)config_dat_path_list_get());
   elm_object_part_content_set(layout, "elm.swallow.dat_path_entry",
                               dat_path_entry);
   elm_layout_text_set(layout, "dat_path_guide", _("Data Paths:"));

   bsd->layout = layout;
   bsd->img_path_entry = img_path_entry;
   bsd->snd_path_entry = snd_path_entry;
   bsd->fnt_path_entry = fnt_path_entry;
   bsd->dat_path_entry = dat_path_entry;

   return layout;
}

build_setting_data *
build_setting_init(void)
{
   build_setting_data *bsd = calloc(1, sizeof(build_setting_data));
   if (!bsd)
     {
        EINA_LOG_ERR(_("Failed to allocate Memory!"));
        return NULL;
     }
   return bsd;
}

void
build_setting_term(build_setting_data *bsd)
{
   if (!bsd) return;

   free(bsd);

   g_bsd = NULL;
}
