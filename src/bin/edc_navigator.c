#include "common.h"

typedef struct edc_navigator_s
{
   Evas_Object *genlist;
   Elm_Object_Item *programs_it;

   Eina_List *group_items;                 //group object item
   Eina_List *part_items;                  //part object item
   Eina_List *state_items;                 //state object item
   Eina_List *program_items;               //program object item

   Eina_List *group_list;                  //group name list
   Eina_List *part_list;                   //part name list
   Eina_List *state_list;                  //state name list
   Eina_List *program_list;                //program name list

   Elm_Genlist_Item_Class *group_itc;
   Elm_Genlist_Item_Class *part_itc;
   Elm_Genlist_Item_Class *state_itc;
   Elm_Genlist_Item_Class *programs_itc;
   Elm_Genlist_Item_Class *program_itc;

} navi_data;

typedef struct part_item_data_s
{
   const char *text;
   Edje_Part_Type type;
} part_item_data;

static navi_data *g_nd = NULL;

static void
gl_part_selected_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info);

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static char *
gl_text_get_cb(void *data, Evas_Object *obj EINA_UNUSED,
               const char *part EINA_UNUSED)
{
   const char *text = data;
   return strdup(text);
}

/* State Related */

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

   //FIXME: Maybe we could optimize if parts list hasn't been changed.
   EINA_LIST_FREE(nd->state_items, it) elm_object_item_del(it);
   edje_edit_string_list_free(nd->state_list);

   //Append States
   Evas_Object *enventor = base_enventor_get();
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

static Evas_Object *
gl_state_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
   if (strcmp("elm.swallow.icon", part)) return NULL;

   Evas_Object *image = elm_image_add(obj);
   elm_image_file_set(image, EDJE_PATH, "navi_state");

   return image;
}

/* Program Related */

static Evas_Object *
gl_program_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
   if (strcmp("elm.swallow.icon", part)) return NULL;

   Evas_Object *image = elm_image_add(obj);
   elm_image_file_set(image, EDJE_PATH, "navi_state");

   return image;
}


static void
gl_program_selected_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info)
{
   Elm_Object_Item *it = event_info;

   //TODO: Search Current Program
}

static void
sub_programs_reload(navi_data *nd, Elm_Object_Item *programs_it)
{
   const Eina_List *programs = elm_genlist_item_subitems_get(programs_it);

   //We already reloaded items
   if (programs) return;

   char *name;
   Eina_List *l;
   Elm_Object_Item *it;

   EINA_LIST_FOREACH(nd->program_list, l, name)
     {
        it = elm_genlist_item_append(nd->genlist,
                                     nd->program_itc,       /* item class */
                                     name,                  /* item data */
                                     programs_it,           /* parent */
                                     ELM_GENLIST_ITEM_NONE, /* item type */
                                     gl_program_selected_cb,/* select cb */
                                     nd);                   /* select cb data */
        nd->program_items = eina_list_append(nd->program_items, it);
     }
}

static void
sub_programs_remove(navi_data *nd)
{
   if (!nd->programs_it) return;

   Eina_List *l;
   Elm_Object_Item *it;
   EINA_LIST_FREE(nd->program_items, it) elm_object_item_del(it);
   edje_edit_string_list_free(nd->program_list);
   nd->program_list = NULL;
}

static void
gl_programs_del_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   navi_data *nd = g_nd;
   if (!nd) return;

   Elm_Object_Item *it = data;
   if (nd->programs_it == it) nd->programs_it = NULL;
}

/* Programs Related */

static void
gl_programs_selected_cb(void *data, Evas_Object *obj EINA_UNUSED,
                        void *event_info)
{
   navi_data *nd = data;
   Elm_Object_Item *it = event_info;

   //TODO: Search Current Programs

   sub_programs_reload(nd, it);
}

static void
programs_reload(navi_data *nd, Elm_Object_Item *group_it)
{
   //FIXME: Maybe we could optimize if programs list hasn't been changed.
   sub_programs_remove(nd);

   //Append Parts
   Evas_Object *enventor = base_enventor_get();
   nd->program_list = enventor_object_programs_list_get(enventor);

   //FIXME: Maybe we could optimize if programs list hasn't been changed.
   elm_object_item_del(nd->programs_it);
   nd->programs_it = NULL;

   if (!nd->program_list) return;

   //Programs Item
   nd->programs_it =
      elm_genlist_item_append(nd->genlist,
                              nd->programs_itc,        /* item class */
                              NULL,                    /* item data */
                              group_it,                /* parent */
                              ELM_GENLIST_ITEM_NONE,   /* item type */
                              gl_programs_selected_cb, /* select cb */
                              nd);                     /* select cb data */
   elm_object_item_data_set(nd->programs_it, nd->programs_it);
}

static char *
gl_programs_text_get_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                        const char *part EINA_UNUSED)
{
   return strdup("PROGRAMS");
}


static Evas_Object *
gl_programs_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
   if (strcmp("elm.swallow.icon", part)) return NULL;

   Evas_Object *image = elm_image_add(obj);
   elm_image_file_set(image, EDJE_PATH, "navi_program");

   return image;
}


/* Part Related */

static void
gl_part_del_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   part_item_data *item_data = data;
   free(item_data);
}

static char *
gl_part_text_get_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    const char *part EINA_UNUSED)
{
   part_item_data *item_data = data;
   return strdup(item_data->text);
}

static Evas_Object *
gl_part_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
   if (strcmp("elm.swallow.icon", part)) return NULL;
   part_item_data *item_data = data;

   Evas_Object *image = elm_image_add(obj);
   const char *group;

   switch (item_data->type)
     {
      case EDJE_PART_TYPE_RECTANGLE:
         group = "navi_rect";
         break;
      case EDJE_PART_TYPE_TEXT:
         group = "navi_text";
         break;
      case EDJE_PART_TYPE_IMAGE:
         group = "navi_image";
         break;
      case EDJE_PART_TYPE_SWALLOW:
         group = "navi_swallow";
         break;
      case EDJE_PART_TYPE_TEXTBLOCK:
         group = "navi_textblock";
         break;
      case EDJE_PART_TYPE_SPACER:
         group = "navi_spacer";
         break;
      default:
         group = "navi_unknown";
         break;
     }

   elm_image_file_set(image, EDJE_PATH, group);

   return image;
}

static void
gl_part_selected_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   navi_data *nd = data;
   Elm_Object_Item *it = event_info;

   //TODO: Search Current Part

   states_reload(nd, it);
}

static void
parts_reload(navi_data *nd, Elm_Object_Item *group_it)
{
   Eina_List *l;
   Elm_Object_Item *it;

   //Remove Previous Parts

   //FIXME: Maybe we could optimize if parts list hasn't been changed.
   EINA_LIST_FREE(nd->part_items, it) elm_object_item_del(it);
   nd->state_items = NULL;
   edje_edit_string_list_free(nd->part_list);

   //Append Parts
   Evas_Object *enventor = base_enventor_get();
   nd->part_list = enventor_object_parts_list_get(enventor);
   char *name;
   part_item_data *data;
   Edje_Part_Type part_type;

   EINA_LIST_FOREACH(nd->part_list, l, name)
     {
        part_type = enventor_object_part_type_get(enventor, name);
        data = malloc(sizeof(part_item_data));
        data->text = name;
        data->type = part_type;

        it = elm_genlist_item_append(nd->genlist,
                                     nd->part_itc,          /* item class */
                                     data,                  /* item data */
                                     group_it,              /* parent */
                                     ELM_GENLIST_ITEM_NONE, /* item type */
                                     gl_part_selected_cb,   /* select cb */
                                     nd);                   /* select cb data */
        nd->part_items = eina_list_append(nd->part_items, it);
     }
}

/* Group Related */

static Evas_Object *
gl_group_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
   if (strcmp("elm.swallow.icon", part)) return NULL;

   Evas_Object *image = elm_image_add(obj);
   elm_image_file_set(image, EDJE_PATH, "navi_group");

   return image;
}

static void
gl_group_selected_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *it = event_info;

   //TODO: Search Current Group
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

void
edc_navigator_group_update(const char *cur_group)
{
   navi_data *nd = g_nd;
   if (!nd) return;

   //Cancel item selection if group was not indicated. 
   if (!cur_group)
     {
        Elm_Object_Item *it = elm_genlist_selected_item_get(nd->genlist);
        if (it) elm_genlist_item_selected_set(it, EINA_FALSE);
        return;
     }

   //If edc_navigator_reload() is not called yet?
   if (!nd->group_list)
     {
        edc_navigator_reload(cur_group);
        return;
     }

   Eina_List *l;
   Elm_Object_Item *it;

   //Find a current group item and select it.
   Elm_Object_Item *group_it = NULL;
   int cur_group_len = strlen(cur_group);

   EINA_LIST_FOREACH(nd->group_items, l, it)
     {
        const char *group_name = elm_object_item_data_get(it);
        if (!group_name) continue;

        if (!strcmp(group_name, cur_group) &&
            (strlen(group_name) == cur_group_len))
          {
             elm_genlist_item_selected_set(it, EINA_TRUE);
             group_it = it;
             break;
          }
     }

   //We couldn't find a group... ?
   if (!group_it) return;

   parts_reload(nd, group_it);
   programs_reload(nd, group_it);
}

void
edc_navigator_reload(const char *cur_group)
{
   navi_data *nd = g_nd;
   if (!nd) return;

   //Reset Navigator resource.

   //FIXME: Maybe we could optimize if group list hasn't been changed.
   nd->group_items = eina_list_free(nd->group_items);
   nd->part_items = NULL;
   nd->state_items = NULL;
   nd->program_items = NULL;

   elm_genlist_clear(nd->genlist);
   edje_file_collection_list_free(nd->group_list);

   if (!cur_group)
     {
        nd->group_list = NULL;
        return;
     }

   nd->group_list = edje_file_collection_list(config_output_path_get());

   //Update Group
   Eina_List *l;
   char *name;
   Elm_Object_Item *it;
   int cur_group_len = strlen(cur_group);

   EINA_LIST_FOREACH(nd->group_list, l, name)
     {
        it = elm_genlist_item_append(nd->genlist,
                                     nd->group_itc,         /* item class */
                                     name,                  /* item data */
                                     NULL,                  /* parent */
                                     ELM_GENLIST_ITEM_NONE, /* item type */
                                     gl_group_selected_cb,  /* select cb */
                                     name);                 /* select cb data */

        nd->group_items = eina_list_append(nd->group_items, it);

        //Update a current group
        if ((cur_group && !strcmp(name, cur_group)) &&
            (strlen(name) == cur_group_len))
          {
             edc_navigator_group_update(cur_group);
          }
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
   elm_object_focus_allow_set(genlist, EINA_FALSE);

   //Group Item Class
   Elm_Genlist_Item_Class *itc;

   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_text_get_cb;
   itc->func.content_get = gl_group_content_get_cb;

   nd->group_itc = itc;

   //Part Item Class
   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_part_text_get_cb;
   itc->func.content_get = gl_part_content_get_cb;
   itc->func.del = gl_part_del_cb;

   nd->part_itc = itc;

   //State Item Class
   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_text_get_cb;
   itc->func.content_get = gl_state_content_get_cb;

   nd->state_itc = itc;

   //Programs Item Class
   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_programs_text_get_cb;
   itc->func.content_get = gl_programs_content_get_cb;
   itc->func.del = gl_programs_del_cb;

   nd->programs_itc = itc;

   //Program Item Class
   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_text_get_cb;
   itc->func.content_get = gl_program_content_get_cb;

   nd->program_itc = itc;

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
   eina_list_free(nd->program_items);

   edje_file_collection_list_free(nd->group_list);
   edje_edit_string_list_free(nd->part_list);
   edje_edit_string_list_free(nd->state_list);
   edje_edit_string_list_free(nd->program_list);

   elm_genlist_item_class_free(nd->group_itc);
   elm_genlist_item_class_free(nd->part_itc);
   elm_genlist_item_class_free(nd->state_itc);
   elm_genlist_item_class_free(nd->programs_itc);
   elm_genlist_item_class_free(nd->program_itc);

   free(nd);
   g_nd = NULL;
}
