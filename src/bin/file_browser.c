#include "common.h"

typedef enum
{
   FILE_BROWSER_FILE_TYPE_DIR = 0,
   FILE_BROWSER_FILE_TYPE_FILE
} File_Browser_File_Type;

typedef struct file_browser_file_s brows_file;
struct file_browser_file_s
{
   char *path;
   char *name;

   File_Browser_File_Type type;

   Eina_List *sub_file_list; //NULL if file type is not directory.

   Elm_Object_Item *it;
};

typedef struct file_browser_s
{
   brows_file *col_edc;   //collections edc
   brows_file *workspace; //workspace directory

   Evas_Object *genlist;
   Elm_Genlist_Item_Class *itc;
   Elm_Genlist_Item_Class *group_itc;
} brows_data;

static brows_data *g_bd = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void brows_file_list_free(Eina_List *file_list);

static void
gl_file_selected_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   brows_file *file = data;
   Elm_Object_Item *it = event_info;
}

static Elm_Object_Item *
file_genlist_item_append(brows_file *file, Elm_Object_Item *parent_it,
                         Elm_Genlist_Item_Type it_type)
{
   brows_data *bd = g_bd;
   if (!bd) return NULL;

   if (!file) return NULL;

   Elm_Object_Item *it =
      elm_genlist_item_append(bd->genlist,
                              bd->itc,             /* item class */
                              file,                /* item data */
                              parent_it,           /* parent */
                              it_type,             /* item type */
                              gl_file_selected_cb, /* select cb */
                              file);               /* select cb data */

   char it_str[EINA_PATH_MAX];
   snprintf(it_str, EINA_PATH_MAX, "%p", it);
   evas_object_data_set(bd->genlist, it_str, file);

   elm_genlist_item_expanded_set(it, EINA_FALSE);

   return it;
}

static char *
gl_file_text_get_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    const char *part EINA_UNUSED)
{
   brows_file *file = data;
   return strdup(file->name);
}

static Evas_Object *
gl_file_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
   brows_file *file = data;

   if (!strcmp(part, "elm.swallow.icon"))
     {
        Evas_Object *img = elm_image_add(obj);

        if (ecore_file_is_dir(file->path))
          elm_image_file_set(img, EDJE_PATH, "folder");
        else
          elm_image_file_set(img, EDJE_PATH, "file");

        return img;
     }
   else return NULL;
}

static char *
gl_group_text_get_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     const char *part EINA_UNUSED)
{
   char *file_name = data;
   return strdup(file_name);
}

static void
gl_exp_req(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *it = event_info;
   elm_genlist_item_expanded_set(it, EINA_TRUE);
}

static void
gl_con_req(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *it = event_info;
   elm_genlist_item_expanded_set(it, EINA_FALSE);
}

static void
gl_exp(void *data, Evas_Object *obj, void *event_info)
{
   brows_data *bd = data;
   if (!bd) return;

   Elm_Object_Item *it = event_info;

   char it_str[EINA_PATH_MAX];
   snprintf(it_str, EINA_PATH_MAX, "%p", it);
   brows_file *file = evas_object_data_get(obj, it_str);
   if (!file) return;

   if (file->sub_file_list)
     {
        Eina_List *l = NULL;
        brows_file *sub_file = NULL;
        EINA_LIST_FOREACH(file->sub_file_list, l, sub_file)
          {
             Elm_Genlist_Item_Type type = ELM_GENLIST_ITEM_NONE;
             if (sub_file->sub_file_list)
               type = ELM_GENLIST_ITEM_TREE;
             sub_file->it = file_genlist_item_append(sub_file, file->it, type);

             if (type == ELM_GENLIST_ITEM_TREE)
               gl_exp_req(NULL, NULL, sub_file->it);
          }
     }
}

static void
gl_con(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *it = event_info;
   elm_genlist_item_subitems_clear(it);
}

/* Find sub files and Create a list of brows_file. */
static Eina_List *
sub_file_list_create(brows_file *file)
{
   brows_data *bd = g_bd;
   if (!bd) return NULL;

   if (!file) return NULL;
   if (file->type != FILE_BROWSER_FILE_TYPE_DIR) return NULL;

   Eina_List *sub_file_list = NULL;

   Eina_List *sub_file_name_list = ecore_file_ls(file->path);
   Eina_List *l = NULL;
   char *sub_file_name = NULL;
   char *dir_path = file->path;
   EINA_LIST_FOREACH(sub_file_name_list, l, sub_file_name)
     {
        brows_file *sub_file = calloc(1, sizeof(brows_file));

        int sub_file_path_len = strlen(dir_path) + strlen(sub_file_name) + 2;
        char *sub_file_path = calloc(1, sizeof(char) * (sub_file_path_len));
        snprintf(sub_file_path, sub_file_path_len, "%s/%s", dir_path,
                 sub_file_name);
        sub_file->path = sub_file_path;
        sub_file->name = strdup(sub_file_name);

        if (ecore_file_is_dir(sub_file_path))
          sub_file->type = FILE_BROWSER_FILE_TYPE_DIR;
        else
          sub_file->type = FILE_BROWSER_FILE_TYPE_FILE;

        sub_file->sub_file_list = sub_file_list_create(sub_file);

        sub_file->it = NULL;

        sub_file_list = eina_list_append(sub_file_list, sub_file);
     }

   EINA_LIST_FREE(sub_file_name_list, sub_file_name)
     {
        free(sub_file_name);
     }

   return sub_file_list;
}

static void
brows_file_free(brows_file *file)
{
   if (!file) return;

   if (file->path) free(file->path);
   if (file->name) free(file->name);

   if (file->sub_file_list)
     brows_file_list_free(file->sub_file_list);
}

static void
brows_file_list_free(Eina_List *file_list)
{
   if (!file_list) return;

   brows_file *file = NULL;
   EINA_LIST_FREE(file_list, file)
     {
        brows_file_free(file);
     }
}

static brows_file *
file_set_internal(const char *file_path, File_Browser_File_Type file_type)
{
   brows_data *bd = g_bd;
   if (!bd) return;

   if (!file_path) return;

   brows_file *file = calloc(1, sizeof(brows_file));
   if (!file)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }

   file->path = ecore_file_realpath(file_path);
   file->name = strdup(ecore_file_file_get(file->path));

   file->type = file_type;

   if (file_type == FILE_BROWSER_FILE_TYPE_DIR)
     file->sub_file_list = sub_file_list_create(file);
   else
     file->sub_file_list = NULL;

   Elm_Genlist_Item_Type it_type = ELM_GENLIST_ITEM_NONE;
   if (file->sub_file_list)
     it_type = ELM_GENLIST_ITEM_TREE;
   file->it = file_genlist_item_append(file, NULL, it_type);

   return file;
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

/* Set workspace directory. */
void
file_browser_workspace_set(const char *workspace_path)
{
   brows_data *bd = g_bd;
   if (!bd) return;

   if (!workspace_path) return;
   if (!ecore_file_exists(workspace_path)) return;
   if (!ecore_file_is_dir(workspace_path)) return;

   if (bd->workspace)
     {
        if (!strcmp(workspace_path, bd->workspace->path))
          return;

        brows_file_free(bd->workspace);
        bd->workspace = NULL;
     }

   Elm_Object_Item *group_it =
      elm_genlist_item_append(bd->genlist,
                              bd->group_itc,         /* item class */
                              "Workspace",           /* item data */
                              NULL,                  /* parent */
                              ELM_GENLIST_ITEM_NONE, /* item type */
                              NULL,                  /* select_cb */
                              NULL);                 /* select_cb data */
   elm_genlist_item_select_mode_set(group_it,
                                    ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

   brows_file *workspace = file_set_internal(workspace_path,
                                             FILE_BROWSER_FILE_TYPE_DIR);
   if (!workspace) return;
   bd->workspace = workspace;

   if (workspace->sub_file_list)
     gl_exp_req(NULL, NULL, workspace->it);
}

/* Set "collections" edc file. */
void
file_browser_edc_file_set(const char *edc_file)
{
   brows_data *bd = g_bd;
   if (!bd) return;

   if (!edc_file) return;
   if (!ecore_file_exists(edc_file)) return;

   char *ext = strrchr(edc_file, '.');
   if (!ext || (strlen(ext) != 4) || strncmp(ext, ".edc", 4))
     return;

   if (bd->col_edc)
     {
        if (!strcmp(edc_file, bd->col_edc->path))
          return;

        brows_file_free(bd->col_edc);
        bd->col_edc = NULL;
     }

   Elm_Object_Item *group_it =
      elm_genlist_item_append(bd->genlist,
                              bd->group_itc,         /* item class */
                              "Collections EDC",     /* item data */
                              NULL,                  /* parent */
                              ELM_GENLIST_ITEM_NONE, /* item type */
                              NULL,                  /* select_cb */
                              NULL);                 /* select_cb data */
   elm_genlist_item_select_mode_set(group_it,
                                    ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

   brows_file *edc = file_set_internal(edc_file, FILE_BROWSER_FILE_TYPE_FILE);
   if (!edc) return;
   bd->col_edc = edc;
}

Evas_Object *
file_browser_init(Evas_Object *parent)
{
   brows_data *bd = calloc(1, sizeof(brows_data));
   if (!bd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }
   g_bd = bd;

   Evas_Object *genlist = elm_genlist_add(parent);
   elm_object_focus_allow_set(genlist, EINA_FALSE);

   evas_object_smart_callback_add(genlist, "expand,request", gl_exp_req, NULL);
   evas_object_smart_callback_add(genlist, "contract,request", gl_con_req,
                                  NULL);
   evas_object_smart_callback_add(genlist, "expanded", gl_exp, bd);
   evas_object_smart_callback_add(genlist, "contracted", gl_con, NULL);

   //Item Class
   Elm_Genlist_Item_Class *itc;
   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_file_text_get_cb;
   itc->func.content_get = gl_file_content_get_cb;
   bd->itc = itc;

   //Group Index Item Class
   Elm_Genlist_Item_Class *group_itc;
   group_itc = elm_genlist_item_class_new();
   group_itc->item_style = "group_index";
   group_itc->func.text_get = gl_group_text_get_cb;
   bd->group_itc = group_itc;

   bd->genlist = genlist;

   return genlist;
}

void
file_browser_term(void)
{
   brows_data *bd = g_bd;
   if (!bd) return;

   if (bd->col_edc) brows_file_free(bd->col_edc);
   if (bd->workspace) brows_file_free(bd->workspace);

   elm_genlist_item_class_free(bd->itc);
   elm_genlist_item_class_free(bd->group_itc);

   free(bd);
   g_bd = NULL;
}
