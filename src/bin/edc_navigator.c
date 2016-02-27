#include "common.h"

typedef struct edc_navigator_s
{
   Evas_Object *genlist;

   Eina_List *group_items;                 //group object item
   Eina_List *part_items;                  //part object item
   Eina_List *state_items;                 //state object item

   Eina_List *group_list;                  //group name list
   Eina_List *part_list;                   //part name list
   Eina_List *state_list;                  //state name list

   Elm_Genlist_Item_Class *group_itc;
   Elm_Genlist_Item_Class *part_itc;
   Elm_Genlist_Item_Class *state_itc;
} navi_data;

static navi_data *g_nd = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/
static void
gl_state_selected_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *it = event_info;
   //TODO: Search Current State
}

static void
states_reload(navi_data *nd, Elm_Object_Item *part_it)
{
   const char *part = elm_object_item_text_get(part_it);
   if (!part) return;

   Eina_List *l;
   Elm_Object_Item *it;

   //Remove Previous Parts
   EINA_LIST_FREE(nd->state_items, it)
     elm_object_item_del(it);

   //Append States
   Evas_Object *enventor = base_enventor_get();
   edje_edit_string_list_free(nd->state_list);
   nd->state_list = enventor_object_part_states_list_get(enventor, part);
   char *name;

   EINA_LIST_FOREACH(nd->state_list, l, name)
     {
        it = elm_genlist_item_append(nd->genlist,
                                     nd->state_itc,         /* item class */
                                     name,                  /* item data */
                                     part_it,               /* parent */
                                     ELM_GENLIST_ITEM_NONE, /* item type */
                                     gl_state_selected_cb,  /* select cb */
                                     nd);                   /* select cb data */
        nd->state_items = eina_list_append(nd->state_items, it);
     }
}

static char *
gl_text_get_cb(void *data, Evas_Object *obj EINA_UNUSED,
               const char *part EINA_UNUSED)
{
   const char *text = data;
   return strdup(text);
}

static Evas_Object *
gl_content_get_cb(void *data, Evas_Object *obj EINA_UNUSED,
                  const char *part EINA_UNUSED)
{
   return NULL;
}

static void
gl_del_cb(void *data, Evas_Object *obj EINA_UNUSED)
{

}

static void
gl_group_selected_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *it = event_info;

   //TODO: Search Current Group
}

static void
gl_part_selected_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   navi_data *nd = data;
   Elm_Object_Item *it = event_info;
   //TODO: Search Current Part

   //TODO: Add States List
   states_reload(nd, it);
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
edc_navigator_parts_reload(void)
{
   if (!config_edc_navigator_get()) return;

   navi_data *nd = g_nd;
   if (!nd) return;

   Eina_List *l;
   Elm_Object_Item *it;

   //Remove Previous Parts
   EINA_LIST_FREE(nd->part_items, it)
      elm_object_item_del(it);

   //Find a current group item
   const char *cur_group = stats_group_name_get();
   if (!cur_group) return;

   Elm_Object_Item *group_item = NULL;

   EINA_LIST_FOREACH(nd->group_items, l, it)
     {
        group_item = it;
        const char *group_name = elm_object_item_text_get(it);
        if (!group_name) continue;

        if (!strcmp(group_name, cur_group) &&
            strlen(group_name) == strlen(cur_group))
          {
             group_item = it;
             break;
          }
     }

   if (!group_item) return;

   //Append Parts
   Evas_Object *enventor = base_enventor_get();
   edje_edit_string_list_free(nd->part_list);
   nd->part_list = enventor_object_parts_list_get(enventor);
   char *name;

   EINA_LIST_FOREACH(nd->part_list, l, name)
     {
        it = elm_genlist_item_append(nd->genlist,
                                     nd->part_itc,          /* item class */
                                     name,                  /* item data */
                                     group_item,            /* parent */
                                     ELM_GENLIST_ITEM_NONE, /* item type */
                                     gl_part_selected_cb,   /* select cb */
                                     nd);                   /* select cb data */
        nd->part_items = eina_list_append(nd->part_items, it);
     }
}

void
edc_navigator_group_reload(void)
{
   if (!config_edc_navigator_get()) return;

   navi_data *nd = g_nd;
   if (!nd) return;

   //Reset Navigator resource
   nd->group_items = eina_list_free(nd->group_items);
   elm_genlist_clear(nd->genlist);
   edje_file_collection_list_free(nd->group_list);
   nd->group_list = edje_file_collection_list(config_output_path_get());

   Eina_List *l;
   char *name;
   Elm_Object_Item *it;
   const char *cur_group = stats_group_name_get();

   //Update Group
   EINA_LIST_REVERSE_FOREACH(nd->group_list, l, name)
     {
        it = elm_genlist_item_append(nd->genlist,
                                     nd->group_itc,         /* item class */
                                     name,                  /* item data */
                                     NULL,                  /* parent */
                                     ELM_GENLIST_ITEM_NONE, /* item type */
                                     gl_group_selected_cb,  /* select cb */
                                     nd);                   /* select cb data */

        nd->group_items = eina_list_append(nd->group_items, it);

        //Update Parts only if current group
        if (cur_group && !strcmp(name, cur_group) &&
            (strlen(name) == strlen(cur_group)))
           edc_navigator_parts_reload();
     }
}

Evas_Object *
edc_navigator_init(Evas_Object *parent)
{
   navi_data *nd = calloc(1, sizeof(navi_data));
   if (!nd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }
   g_nd = nd;

   Evas_Object *genlist = elm_genlist_add(parent);

   //Group Item Class
   Elm_Genlist_Item_Class *itc;

   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_text_get_cb;
   itc->func.content_get = gl_content_get_cb;
   itc->func.del = gl_del_cb;

   nd->group_itc = itc;

   //Part Item Class
   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_text_get_cb;
   itc->func.content_get = gl_content_get_cb;
   itc->func.del = gl_del_cb;

   nd->part_itc = itc;

   //State Item Class
   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_text_get_cb;
   itc->func.content_get = gl_content_get_cb;
   itc->func.del = gl_del_cb;

   nd->state_itc = itc;

   nd->genlist = genlist;

   return genlist;
}

void
edc_navigator_term(void)
{
   navi_data *nd = g_nd;
   if (!nd) return;

   eina_list_free(nd->state_items);
   eina_list_free(nd->part_items);
   eina_list_free(nd->group_items);
   edje_file_collection_list_free(nd->group_list);
   edje_edit_string_list_free(nd->part_list);
   edje_edit_string_list_free(nd->state_list);
   elm_genlist_item_class_free(nd->group_itc);
   elm_genlist_item_class_free(nd->part_itc);
   elm_genlist_item_class_free(nd->state_itc);
   free(nd);
   g_nd = NULL;
}