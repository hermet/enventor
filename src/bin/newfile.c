#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

typedef struct new_data_s {
   Evas_Object *list;
   Eina_List *templates;
} new_data;

static new_data *g_nd = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
list_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   new_data *nd = data;
   free(nd);
   g_nd = NULL;
}

static int
template_sort_cb(const void *data1, const void *data2)
{
   const char *name1 = data1;
   const char *name2 = data2;

   if (name1[0] > name2[0]) return 1;
   else return -1;
}

static void
file_dir_list_cb(const char *name, const char *path EINA_UNUSED, void *data)
{
   new_data *nd = data;

   char *ext = strrchr(name, '.');
   if (!ext || strcmp(ext, ".edc")) return;

   char *file_name = ecore_file_strip_ext(name);
   nd->templates = eina_list_sorted_insert(nd->templates,
                                           template_sort_cb,
                                           eina_stringshare_add(file_name));
   free(file_name);
}

static void
list_item_selected_cb(void *data, Evas_Object *obj EINA_UNUSED,
                      void *event_info)
{
   Evas_Object *layout = data;
   Elm_Object_Item *it = event_info;
   char *name = (char *)elm_object_item_text_get(it);

   /* empty is real empty. cannot load the edj. so replace the empty to minimum
      to show the preview layout. */
   if (!strcmp("empty", name)) name = "minimum";

   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/templates/%s.edj", elm_app_data_dir_get(),
            name);

   elm_layout_file_set(layout, buf, "main");
}

static void
templates_get(new_data *nd)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/templates", elm_app_data_dir_get());

   if (!ecore_file_is_dir(buf))
     {
        EINA_LOG_ERR(_("Cannot find templates folder! \"%s\""), buf);
        return;
     }

   eina_file_dir_list(buf, EINA_FALSE, file_dir_list_cb, nd);
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
newfile_set(Eina_Bool template_new)
{
   new_data *nd = g_nd;
   EINA_SAFETY_ON_NULL_RETURN(nd);

   Elm_Object_Item *it = elm_list_selected_item_get(nd->list);
   EINA_SAFETY_ON_NULL_RETURN(it);

   Eina_Bool success = EINA_TRUE;
   char buf[PATH_MAX];
   char path[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s/templates/%s.edc",
            elm_app_data_dir_get(), elm_object_item_text_get(it));
   if (template_new && config_input_path_get())
     snprintf(path, sizeof(path), "%s", config_input_path_get());
   else
     {
        Eina_Tmpstr *tmp_path;
        eina_file_mkstemp(DEFAULT_EDC_FORMAT, &tmp_path);
        sprintf(path, "%s", (const char *)tmp_path);
        eina_tmpstr_del(tmp_path);
        config_input_path_set(path);

        Eina_List *list = eina_list_append(NULL, config_output_path_get());
        enventor_object_path_set(base_enventor_get(), ENVENTOR_PATH_TYPE_EDJ,
                                 list);
        eina_list_free(list);
     }
   success = eina_file_copy(buf, path,
                            EINA_FILE_COPY_DATA, NULL, NULL);
   if (!success)
     {
        EINA_LOG_ERR(_("Cannot find file! \"%s\""), buf);
        return;
     }
   file_mgr_main_file_set(path);
   file_mgr_reset();
   file_browser_main_file_unset();
}

void
newfile_default_set(Eina_Bool default_edc)
{
   if (!default_edc) return;

   Eina_Bool success = EINA_TRUE;
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s/templates/Basic.edc",
            elm_app_data_dir_get());
   success = eina_file_copy(buf,config_input_path_get(),
                            EINA_FILE_COPY_DATA, NULL, NULL);
   if (!success)
     {
        EINA_LOG_ERR(_("Cannot find file! \"%s\""), buf);
        return;
     }
}

Evas_Object *
newfile_create(Evas_Object *parent, Evas_Smart_Cb selected_cb, void *data)
{
   new_data *nd = g_nd;
   if (!nd)
     {
        nd = calloc(1, sizeof(new_data));
        if (!nd)
          {
             mem_fail_msg();
             return NULL;
          }
        g_nd = nd;
     }

   //Grid
   Evas_Object *grid = elm_grid_add(parent);

   //Preview Layout
   Evas_Object *layout = elm_layout_add(grid);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(layout);

   elm_grid_pack(grid, layout, 2, 2, 46, 98);

   templates_get(nd);

   //List
   Evas_Object *list = elm_list_add(grid);
   elm_object_focus_set(list, EINA_TRUE);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(list, "activated", selected_cb, data);

   Eina_List *l;
   Eina_Stringshare *name;
   Elm_Object_Item *it;
   Eina_Bool first_item = EINA_TRUE;

   EINA_LIST_FOREACH(nd->templates, l, name)
     {
        it = elm_list_item_append(list, name, NULL, NULL, list_item_selected_cb,
                                  layout);
        if (first_item)
          {
             elm_list_item_selected_set(it, EINA_TRUE);
             first_item = EINA_FALSE;
          }
     }
   evas_object_event_callback_add(list, EVAS_CALLBACK_DEL, list_del_cb, nd);
   evas_object_show(list);
   elm_object_focus_set(list, EINA_TRUE);

   elm_grid_pack(grid, list, 50, 0, 50, 100);

   EINA_LIST_FREE(nd->templates, name)
     eina_stringshare_del(name);

   nd->list = list;

   return grid;
}
