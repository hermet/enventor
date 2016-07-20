#include "common.h"

typedef enum
{
   FILE_BROWSER_FILE_TYPE_DIR = 0,
   FILE_BROWSER_FILE_TYPE_EDC,
   FILE_BROWSER_FILE_TYPE_IMAGE,
   FILE_BROWSER_FILE_TYPE_SOUND,
   FILE_BROWSER_FILE_TYPE_FONT,
   FILE_BROWSER_FILE_TYPE_OTHERS
} File_Browser_File_Type;

typedef struct file_browser_file_s brows_file;
struct file_browser_file_s
{
   Eina_List *sub_file_list; //NULL if file type is not directory.

   char *path;
   char *name;
   File_Browser_File_Type type;

   Elm_Object_Item *it;

   Eina_Bool main: 1;   //Is it main edc file?
};

typedef enum
{
   FILE_BROWSER_MODE_DEFAULT = 0,
   FILE_BROWSER_MODE_SEARCH
} File_Browser_Mode;

typedef struct file_browser_s
{
   brows_file *workspace;       //workspace directory
   Eina_List *search_file_list; /* list of searched files. This is only used for
                                   showing search results. */

   Evas_Object *base_layout;
   Evas_Object *search_entry;
   Evas_Object *genlist;
   Evas_Object *show_all_check;

   Elm_Genlist_Item_Class *itc;
   Elm_Genlist_Item_Class *search_itc;

   File_Browser_Mode mode;
} brows_data;

static brows_data *g_bd = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void brows_file_free(brows_file *file);
static void brows_file_list_free(Eina_List *file_list);
static Eina_List *sub_brows_file_list_create(brows_file *file);
static void refresh_btn_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED);

static void
gl_clicked_double_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                     void *event_info)
{
   Elm_Object_Item *it = event_info;
   brows_file *file = elm_object_item_data_get(it);
   if (!file)
     {
        EINA_LOG_ERR("No item data??");
        return;
     }

   if (!file->path)
     {
        EINA_LOG_ERR("No item file path??");
        return;
     }

   /* Open a double clicked edc file. */

   //skip non edc file.
   if (!eina_str_has_extension(file->path, "edc")) return;

   unsigned int selected_file_len = strlen(file->path);

   Enventor_Item *eit;
   const char *it_file_path;

   //Case 1. main file.
   eit = file_mgr_main_item_get();
   if (eit)
     {
        it_file_path = enventor_item_file_get(eit);
        if (!it_file_path)
          {
             EINA_LOG_ERR("No main item file path??");
             return;
          }
        //Ok, This selected file is already openend, let's activate the item.
        if (!strcmp(file->name, ecore_file_file_get(it_file_path)))
          {
             file_mgr_file_focus(eit);
             return;
          }
     }

   //Case 2. sub files.
   Eina_List *sub_items =
      (Eina_List *)enventor_object_sub_items_get(base_enventor_get());
   Eina_List *l;
   EINA_LIST_FOREACH(sub_items, l, eit)
     {
        it_file_path = enventor_item_file_get(eit);
        if (!it_file_path) continue;

        //Let's check if the file is already opened.
        if (selected_file_len != strlen(it_file_path)) continue;

        //Ok, This selected file is already openend, let's activate the item.
        if (!strcmp(file->path, it_file_path))
          {
             file_mgr_file_focus(eit);
             return;
          }
     }

   //This selected file hasn't been opened yet, so let's open this file newly.
   file_mgr_sub_file_add(file->path);
}

//Set file->it as NULL when genlist item is deleted.
static void
gl_item_del_cb(void *data, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   brows_file *file = data;
   if (!file) return;

   file->it = NULL;
}

static Elm_Object_Item *
file_genlist_item_append(brows_file *file, Elm_Object_Item *parent_it,
                         Elm_Genlist_Item_Type it_type)
{
   brows_data *bd = g_bd;
   if (!bd) return NULL;

   if (!file) return NULL;


   Elm_Genlist_Item_Class *itc = bd->itc;
   if (bd->mode == FILE_BROWSER_MODE_SEARCH)
     itc = bd->search_itc;

   if (file->it) elm_object_item_del(file->it);

   file->it = elm_genlist_item_append(bd->genlist,
                                      itc,         /* item class */
                                      file,        /* item data */
                                      parent_it,   /* parent */
                                      it_type,     /* item type */
                                      NULL,        /* select cb */
                                      file);       /* select cb data */

   //Set file->it as NULL when genlist item is deleted.
   elm_object_item_del_cb_set(file->it, gl_item_del_cb);

   elm_genlist_item_expanded_set(file->it, EINA_FALSE);

   return file->it;
}

static char *
main_file_text_get(brows_file *file)
{
   char path[PATH_MAX];
   snprintf(path, sizeof(path), "%s [main]", file->name);
   return  strdup(path);
}

static char *
gl_file_text_get_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    const char *part EINA_UNUSED)
{
   brows_file *file = data;
   if (!file->main) return strdup(file->name);
   return main_file_text_get(file);
}

static Evas_Object *
gl_file_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
   brows_data *bd = g_bd;
   if (!bd) return NULL;

   brows_file *file = data;

   if (!strcmp(part, "elm.swallow.icon"))
     {
        Evas_Object *img = elm_image_add(obj);

        if (file->type == FILE_BROWSER_FILE_TYPE_DIR)
          elm_image_file_set(img, EDJE_PATH, "folder");
        else if (file->type == FILE_BROWSER_FILE_TYPE_EDC)
          elm_image_file_set(img, EDJE_PATH, "brows_logo");
        else if (file->type == FILE_BROWSER_FILE_TYPE_IMAGE)
          elm_image_file_set(img, EDJE_PATH, "brows_image");
        else if (file->type == FILE_BROWSER_FILE_TYPE_SOUND)
          elm_image_file_set(img, EDJE_PATH, "brows_sound");
        else if (file->type == FILE_BROWSER_FILE_TYPE_FONT)
          elm_image_file_set(img, EDJE_PATH, "brows_font");
        else
          elm_image_file_set(img, EDJE_PATH, "file");

        return img;
     }
   else
     {
        //Refresh button for the most top directory
        if (file == bd->workspace && !strcmp(part, "elm.swallow.end"))
          {
             //This wrapper box is used for non-scaled button tooltip.
             Evas_Object *box = elm_box_add(obj);
             elm_object_tooltip_text_set(box, "Refresh Workspace");

             //Button
             Evas_Object *btn = elm_button_add(box);
             evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND,
                                              EVAS_HINT_EXPAND);
             evas_object_size_hint_align_set(btn, EVAS_HINT_FILL,
                                             EVAS_HINT_FILL);
             elm_object_scale_set(btn, 0.6);
             evas_object_smart_callback_add(btn, "clicked",
                                            refresh_btn_clicked_cb, NULL);
             evas_object_show(btn);

             //Icon
             Evas_Object *img = elm_image_add(btn);
             elm_image_file_set(img, EDJE_PATH, "refresh");
             elm_object_content_set(btn, img);

             elm_box_pack_end(box, btn);

             return box;
          }
     }

   return NULL;
}

static char *
gl_search_text_get_cb(void *data, Evas_Object *obj EINA_UNUSED,
                      const char *part)
{
   brows_file *file = data;

   if (!strcmp(part, "elm.text"))
     return strdup(file->name);
   else if (!strcmp(part, "elm.text.sub"))
     return strdup(file->path);

   return NULL;
}

static Evas_Object *
gl_search_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
   return gl_file_content_get_cb(data, obj, part);
}

static void
gl_expanded_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                       void *event_info)
{
   brows_data *bd = g_bd;
   if (!bd) return;
   if (bd->mode == FILE_BROWSER_MODE_SEARCH) return;

   Elm_Object_Item *it = event_info;
   elm_genlist_item_expanded_set(it, EINA_TRUE);
}

static void
gl_contract_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   brows_data *bd = g_bd;
   if (!bd) return;
   if (bd->mode == FILE_BROWSER_MODE_SEARCH) return;

   Elm_Object_Item *it = event_info;
   elm_genlist_item_expanded_set(it, EINA_FALSE);
}

static void
gl_expanded_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   brows_data *bd = g_bd;
   if (!bd) return;
   if (bd->mode == FILE_BROWSER_MODE_SEARCH) return;

   Elm_Object_Item *it = event_info;

   brows_file *file = elm_object_item_data_get(it);
   if (!file) return;

   /* Basically, sub file list is not created. So if sub file list has not been
      created before, then create sub file list. */
   if (!file->sub_file_list)
     file->sub_file_list = sub_brows_file_list_create(file);

   if (file->sub_file_list)
     {
        Eina_List *l = NULL;
        brows_file *sub_file = NULL;
        EINA_LIST_FOREACH(file->sub_file_list, l, sub_file)
          {
             if (!elm_check_state_get(bd->show_all_check))
               {
                  //Skip if the file name is started with "."
                  if ((sub_file->type == FILE_BROWSER_FILE_TYPE_OTHERS) ||
                      (sub_file->name[0] == '.'))
                     continue;
               }

             Elm_Genlist_Item_Type type = ELM_GENLIST_ITEM_NONE;
             if (sub_file->type == FILE_BROWSER_FILE_TYPE_DIR)
               type = ELM_GENLIST_ITEM_TREE;
             file_genlist_item_append(sub_file, it, type);
          }
     }
}

static void
gl_contracted_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   brows_data *bd = g_bd;
   if (!bd) return;
   if (bd->mode == FILE_BROWSER_MODE_SEARCH) return;

   Elm_Object_Item *it = event_info;
   elm_genlist_item_subitems_clear(it);

   brows_file *file = elm_object_item_data_get(it);
   if (file && file->sub_file_list)
     {
        brows_file_list_free(file->sub_file_list);
        file->sub_file_list = NULL;
     }
}

static int
file_strcmp_cb(const void *data1, const void *data2)
{
   const brows_file *file1 = data1;
   const brows_file *file2 = data2;

   if (file1->type < file2->type)
     return -1;
   else if (file1->type > file2->type)
     return 1;
   else
     return strcmp(file1->name, file2->name);
}

static File_Browser_File_Type
brows_file_type_get(const char *file_path)
{
   if (!file_path) return FILE_BROWSER_FILE_TYPE_OTHERS;

   if (ecore_file_is_dir(file_path)) return FILE_BROWSER_FILE_TYPE_DIR;

   const char *file_path_end = file_path + strlen(file_path);
   char *file_ext = strrchr(file_path, '.');

   if (!file_ext) return FILE_BROWSER_FILE_TYPE_OTHERS;
   file_ext++;

   if (file_ext >= file_path_end) return FILE_BROWSER_FILE_TYPE_OTHERS;

   if (!strcmp(file_ext, "edc") || !strcmp(file_ext, "EDC"))
     return FILE_BROWSER_FILE_TYPE_EDC;

   if (!strcmp(file_ext, "png") || !strcmp(file_ext, "PNG") ||
       !strcmp(file_ext, "jpg") || !strcmp(file_ext, "JPG") ||
       !strcmp(file_ext, "jpeg") || !strcmp(file_ext, "JPEG") ||
       !strcmp(file_ext, "bmp") || !strcmp(file_ext, "BMP") ||
       !strcmp(file_ext, "gif") || !strcmp(file_ext, "GIF") ||
       !strcmp(file_ext, "svg") || !strcmp(file_ext, "SVG"))
     return FILE_BROWSER_FILE_TYPE_IMAGE;

   if (!strcmp(file_ext, "wav") || !strcmp(file_ext, "WAV") ||
       !strcmp(file_ext, "ogg") || !strcmp(file_ext, "OGG") ||
       !strcmp(file_ext, "mp3") || !strcmp(file_ext, "MP3"))
     return FILE_BROWSER_FILE_TYPE_SOUND;

   if (!strcmp(file_ext, "ttf") || !strcmp(file_ext, "TTF") ||
       !strcmp(file_ext, "fon") || !strcmp(file_ext, "FON"))
     return FILE_BROWSER_FILE_TYPE_FONT;

   return FILE_BROWSER_FILE_TYPE_OTHERS;
}

static brows_file *
brows_file_create(const char *file_path, Eina_Bool create_sub_file_list)
{
   if (!file_path) return NULL;
   if (!ecore_file_exists(file_path)) return NULL;

   brows_file *file = calloc(1, sizeof(brows_file));
   if (!file)
     {
        mem_fail_msg();
        return NULL;
     }

   //realpath() works only if the given file exists.
   char *file_abs_path = ecore_file_realpath(file_path);
   file->path = file_abs_path;
   file->name = strdup(ecore_file_file_get(file_abs_path));
   file->type = brows_file_type_get(file_abs_path);

   //Check whether this file is main file.
   if (file->type == FILE_BROWSER_FILE_TYPE_EDC &&
       config_input_path_get())
     {
        if (!strcmp(file->name, ecore_file_file_get(config_input_path_get())))
          file->main = EINA_TRUE;
     }

   if (create_sub_file_list)
     file->sub_file_list = sub_brows_file_list_create(file);
   else
     file->sub_file_list = NULL;

   return file;
}

/* Find sub files and Create a list of brows_file. */
static Eina_List *
sub_brows_file_list_create(brows_file *file)
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
        int sub_file_path_len = strlen(dir_path) + strlen(sub_file_name) + 2;
        char *sub_file_path = calloc(1, sizeof(char) * (sub_file_path_len));
        snprintf(sub_file_path, sub_file_path_len, "%s/%s", dir_path,
                 sub_file_name);

        //Create sub file without creating its sub file list.
        brows_file *sub_file = brows_file_create(sub_file_path, EINA_FALSE);
        free(sub_file_path);
        if (!sub_file) continue;

        sub_file_list =
           eina_list_sorted_insert(sub_file_list,
                                   (Eina_Compare_Cb)file_strcmp_cb,
                                   sub_file);
     }

   return sub_file_list;
}

static void
brows_file_free(brows_file *file)
{
   if (!file) return;

   if (file->path) free(file->path);
   if (file->name) free(file->name);

   if (file->it)
     elm_object_item_del(file->it);

   if (file->sub_file_list)
     brows_file_list_free(file->sub_file_list);

   free(file);
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

static void
file_browser_workspace_reset(void)
{
   brows_data *bd = g_bd;
   if (!bd) return;

   elm_genlist_clear(bd->genlist);

   brows_file_free(bd->workspace);
   bd->workspace = NULL;

   file_browser_workspace_set(config_workspace_path_get());
}

static void
search_file_set_internal(const char *file_path)
{
   brows_data *bd = g_bd;
   if (!bd) return;
   if (!bd->search_entry) return;

   if (!file_path) return;

   const char *search_word = elm_entry_entry_get(bd->search_entry);
   if (!search_word || !strcmp(search_word, "")) return;

   /* Create brows_file and append genlist item if current file name contains
      search word. */
   const char *file_name = ecore_file_file_get(file_path);
   if (file_name && strstr(file_name, search_word))
     {
        //Check "Show All Files" option.
        File_Browser_File_Type type = brows_file_type_get(file_path);
        if (elm_check_state_get(bd->show_all_check) ||
            ((type == FILE_BROWSER_FILE_TYPE_EDC) ||
             (type == FILE_BROWSER_FILE_TYPE_IMAGE) ||
             (type == FILE_BROWSER_FILE_TYPE_SOUND) ||
             (type == FILE_BROWSER_FILE_TYPE_FONT)))
          {
             brows_file *file = brows_file_create(file_path, EINA_FALSE);
             if (file)
               {
                  bd->search_file_list =
                     eina_list_sorted_insert(bd->search_file_list,
                                             (Eina_Compare_Cb)file_strcmp_cb,
                                             file);
               }
          }
     }

   if (!ecore_file_is_dir(file_path)) return;

   //Set sub files by calling function resursively.
   Eina_List *sub_file_name_list = ecore_file_ls(file_path);
   Eina_List *l = NULL;
   char *sub_file_name = NULL;
   const char *dir_path = file_path;
   EINA_LIST_FOREACH(sub_file_name_list, l, sub_file_name)
     {
        int sub_file_path_len = strlen(dir_path) + strlen(sub_file_name) + 2;
        char *sub_file_path = calloc(1, sizeof(char) * (sub_file_path_len));
        snprintf(sub_file_path, sub_file_path_len, "%s/%s", dir_path,
                 sub_file_name);

        search_file_set_internal(sub_file_path);
        free(sub_file_path);
     }
}

/* Create brows_file and append genlist item if the given file name contains
   search word. This function is called recursively. */
static void
search_file_set(const char *file_path)
{
   brows_data *bd = g_bd;
   if (!bd) return;
   if (!bd->search_entry) return;

   if (!file_path) return;

   const char *search_word = elm_entry_entry_get(bd->search_entry);
   if (!search_word || !strcmp(search_word, "")) return;

   search_file_set_internal(file_path);

   Eina_List *l = NULL;
   brows_file *file = NULL;
   EINA_LIST_FOREACH(bd->search_file_list, l, file)
     {
        file_genlist_item_append(file, NULL, ELM_GENLIST_ITEM_NONE);
     }
}

static void
search_entry_changed_cb(void *data EINA_UNUSED, Evas_Object *obj,
                        void *event_info EINA_UNUSED)
{
   brows_data *bd = g_bd;
   if (!bd) return;


   const char *search_word = elm_entry_entry_get(obj);
   //Default Mode
   if (!search_word || !strcmp(search_word, ""))
     {
        //Change mode first because mode is used in following functions.
        bd->mode = FILE_BROWSER_MODE_DEFAULT;

        file_browser_workspace_reset();
     }
   //Search Mode
   else
     {
        //Change mode first because mode is used in following functions.
        bd->mode = FILE_BROWSER_MODE_SEARCH;

        elm_genlist_clear(bd->genlist);

        if (bd->search_file_list)
          {
             brows_file_list_free(bd->search_file_list);
             bd->search_file_list = NULL;
          }
        search_file_set(config_workspace_path_get());
     }
}

static void
refresh_btn_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   brows_data *bd = g_bd;
   if (!bd) return;

   if (bd->mode == FILE_BROWSER_MODE_DEFAULT)
     file_browser_workspace_reset();
   else
     {
        if (bd->search_file_list)
          {
             brows_file_list_free(bd->search_file_list);
             bd->search_file_list = NULL;
          }
        search_file_set(config_workspace_path_get());
     }
}

static void
show_all_check_changed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                          void *event_info EINA_UNUSED)
{
   brows_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   if (bd->mode == FILE_BROWSER_MODE_DEFAULT)
     file_browser_workspace_reset();
   else
     {
        if (bd->search_file_list)
          {
             brows_file_list_free(bd->search_file_list);
             bd->search_file_list = NULL;
          }
        search_file_set(config_workspace_path_get());
     }
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

/* Set workspace directory. */
void
file_browser_workspace_set(const char *workspace_path)
{
   brows_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

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

   //Create file with creating its sub file list.
   brows_file *workspace = brows_file_create(workspace_path, EINA_TRUE);
   EINA_SAFETY_ON_NULL_RETURN(workspace);
   bd->workspace = workspace;

   elm_object_disabled_set(bd->base_layout, EINA_FALSE);

   Elm_Genlist_Item_Type it_type = ELM_GENLIST_ITEM_NONE;
   if (workspace->sub_file_list)
     it_type = ELM_GENLIST_ITEM_TREE;
   Elm_Object_Item *it = file_genlist_item_append(workspace, NULL, it_type);

   if (workspace->sub_file_list)
     gl_expanded_request_cb(NULL, NULL, it);
}

Evas_Object *
file_browser_init(Evas_Object *parent)
{
   brows_data *bd = g_bd;
   if (bd) return bd->base_layout;

   bd = calloc(1, sizeof(brows_data));
   if (!bd)
     {
        mem_fail_msg();
        return NULL;
     }
   g_bd = bd;

   //Base layout
   Evas_Object *base_layout = elm_layout_add(parent);
   elm_layout_file_set(base_layout, EDJE_PATH, "tools_layout");
   evas_object_size_hint_align_set(base_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(base_layout, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_show(base_layout);

   //Main Box
   Evas_Object *main_box = elm_box_add(base_layout);
   evas_object_size_hint_align_set(main_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(main_box, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   elm_object_content_set(base_layout, main_box);

   Evas_Object *sub_box;

   //Sub box for search.
   sub_box = elm_box_add(main_box);
   elm_box_padding_set(sub_box, ELM_SCALE_SIZE(5), 0);
   elm_box_horizontal_set(sub_box, EINA_TRUE);
   evas_object_size_hint_align_set(sub_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(sub_box, EVAS_HINT_EXPAND, 0);
   evas_object_show(sub_box);
   elm_box_pack_end(main_box, sub_box);

   //Search Icon
   Evas_Object *search_img = elm_image_add(sub_box);
   evas_object_size_hint_min_set(search_img, ELM_SCALE_SIZE(15),
                                 ELM_SCALE_SIZE(15));
   elm_image_file_set(search_img, EDJE_PATH, "search");
   evas_object_show(search_img);
   elm_box_pack_end(sub_box, search_img);

   //Search Entry
   Evas_Object *search_entry = elm_entry_add(sub_box);
   elm_entry_single_line_set(search_entry, EINA_TRUE);
   elm_entry_scrollable_set(search_entry, EINA_TRUE);
   elm_object_part_text_set(search_entry, "guide", "Type file name to search");
   evas_object_smart_callback_add(search_entry, "changed",
                                  search_entry_changed_cb, NULL);
   evas_object_size_hint_align_set(search_entry, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(search_entry, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_show(search_entry);
   elm_box_pack_end(sub_box, search_entry);


   //Genlist
   Evas_Object *genlist = elm_genlist_add(main_box);
   elm_object_focus_allow_set(genlist, EINA_FALSE);
   evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);

   evas_object_smart_callback_add(genlist, "expand,request",
                                  gl_expanded_request_cb, NULL);
   evas_object_smart_callback_add(genlist, "contract,request",
                                  gl_contract_request_cb, NULL);
   evas_object_smart_callback_add(genlist, "expanded", gl_expanded_cb, NULL);
   evas_object_smart_callback_add(genlist, "contracted", gl_contracted_cb,
                                  NULL);
   evas_object_smart_callback_add(genlist, "clicked,double",
                                  gl_clicked_double_cb, NULL);
   evas_object_show(genlist);
   elm_box_pack_end(main_box, genlist);

   //Show All Files Check
   Evas_Object *show_all_check = elm_check_add(main_box);
   evas_object_size_hint_align_set(show_all_check, 0, 1);
   evas_object_size_hint_weight_set(show_all_check, EVAS_HINT_EXPAND, 0);
   elm_object_text_set(show_all_check, "Show All Files");
   evas_object_smart_callback_add(show_all_check, "changed",
                                  show_all_check_changed_cb, NULL);
   evas_object_show(show_all_check);
   elm_box_pack_end(main_box, show_all_check);

   //Default Mode Item Class
   Elm_Genlist_Item_Class *itc;
   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_file_text_get_cb;
   itc->func.content_get = gl_file_content_get_cb;
   bd->itc = itc;

   //Search Mode Item Class
   Elm_Genlist_Item_Class *search_itc;
   search_itc = elm_genlist_item_class_new();
   search_itc->item_style = "double_label";
   search_itc->func.text_get = gl_search_text_get_cb;
   search_itc->func.content_get = gl_search_content_get_cb;
   bd->search_itc = search_itc;

   bd->base_layout = base_layout;
   bd->search_entry = search_entry;
   bd->genlist = genlist;
   bd->show_all_check = show_all_check;

   elm_object_disabled_set(base_layout, EINA_TRUE);

   return base_layout;
}

void
file_browser_term(void)
{
   brows_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   if (bd->workspace) brows_file_free(bd->workspace);
   if (bd->search_file_list) brows_file_list_free(bd->search_file_list);

   elm_genlist_item_class_free(bd->itc);
   elm_genlist_item_class_free(bd->search_itc);

   evas_object_del(bd->base_layout);

   free(bd);
   g_bd = NULL;
}

void
file_browser_tools_set(void)
{
   brows_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   Evas_Object *rect =
      evas_object_rectangle_add(evas_object_evas_get(bd->base_layout));
   evas_object_color_set(rect, 0, 0, 0, 0);
   elm_object_part_content_set(bd->base_layout, "elm.swallow.tools", rect);
}

void
file_browser_tools_visible_set(Eina_Bool visible)
{
   brows_data *bd = g_bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   if (visible)
     elm_object_signal_emit(bd->base_layout, "elm,state,tools,show", "");
   else
     elm_object_signal_emit(bd->base_layout, "elm,state,tools,hide", "");
}
