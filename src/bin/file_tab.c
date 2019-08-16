#include "common.h"

typedef struct file_tab_s
{
   Evas_Object *list;
   Evas_Object *box;
   Elm_Object_Item *selected_it; //list selected item
} file_data;

typedef struct file_tab_it_s
{
   Enventor_Item *enventor_it;
   Elm_Object_Item *it;
   Ecore_Animator *animator;
} file_tab_it;

file_data *g_fd = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/
static void
left_btn_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   file_data *fd = data;

   Elm_Object_Item *it = elm_list_selected_item_get(fd->list);
   //just in case
   if (!it) it = elm_list_last_item_get(fd->list);
   if (!it) return;
   it = elm_list_item_prev(it);
   if (!it) return;

   elm_list_item_selected_set(it, EINA_TRUE);
   elm_list_item_bring_in(it);
}

static void
right_btn_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   file_data *fd = data;

   Elm_Object_Item *it = elm_list_selected_item_get(fd->list);
   //just in case
   if (!it) it = elm_list_first_item_get(fd->list);
   if (!it) return;
   it = elm_list_item_next(it);
   if (!it) return;

   elm_list_item_selected_set(it, EINA_TRUE);
   elm_list_item_bring_in(it);
}

static Eina_Bool
list_item_anim_cb(void *data)
{
   file_tab_it *fti = data;

   elm_list_item_bring_in(fti->it);
   file_mgr_file_focus(fti->enventor_it);
   fti->animator = NULL;

   return ECORE_CALLBACK_CANCEL;
}

static void
list_item_selected_cb(void *data, Evas_Object *obj EINA_UNUSED,
                      void *event_info)
{
   file_data *fd = g_fd;
   if (!fd) return;

   Elm_Object_Item *it = event_info;

   if (fd->selected_it == it) return;

   //When list item's close button is clicked, this selection is triggered
   //prior to the close button clicked callback.
   //It's useless to perform this selected callback anyway this file tab
   //item is gonna removed soon. But, due to the list's behavior,
   //it's unavoidable so we need this tricky animator.
   file_tab_it *fti = data;
   ecore_animator_del(fti->animator);
   fti->animator = ecore_animator_add(list_item_anim_cb, fti);
}

static void
file_tab_it_remove_internal(file_tab_it *fti)
{
   Evas_Object *list = elm_object_item_widget_get(fti->it);

   //FIXME: If this item is main, then it needs to close project.

   //If the focused item is removed, then enable next item.
   if (elm_list_selected_item_get(list) == fti->it)
     {
        //Next?
        Elm_Object_Item *it;
        it = elm_list_item_next(fti->it);

        //Prev?
        if (!it) it = elm_list_item_prev(fti->it);

        if (it)
          {
             file_tab_it *next_fti = elm_object_item_data_get(it);
             file_mgr_file_focus(next_fti->enventor_it);
          }
     }

   //Remove item.
   enventor_item_del(fti->enventor_it);
   elm_object_item_del(fti->it);
   ecore_animator_del(fti->animator);
   free(fti);

   elm_list_go(list);
}

static void
close_btn_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   file_tab_it *fti = data;
   file_mgr_file_del(fti->enventor_it);
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/
void
file_tab_it_remove(Enventor_Item *enventor_it)
{
   file_data *fd = g_fd;
   EINA_SAFETY_ON_NULL_RETURN(fd);

   Eina_List *list = (Eina_List*) elm_list_items_get(fd->list);
   Eina_List *l;
   Elm_Object_Item *it;

   EINA_LIST_FOREACH(list, l, it)
     {
        file_tab_it *fti = elm_object_item_data_get(it);
        if (fti->enventor_it == enventor_it)
          {
             file_tab_it_remove_internal(fti);
             break;
          }
     }
}

Eina_Bool
file_tab_it_select(Enventor_Item *enventor_it)
{
   file_data *fd = g_fd;
   EINA_SAFETY_ON_NULL_RETURN_VAL(fd, EINA_FALSE);

   Eina_List *list = (Eina_List*) elm_list_items_get(fd->list);
   Eina_List *l;
   Elm_Object_Item *it;

   EINA_LIST_FOREACH(list, l, it)
     {
        file_tab_it *fti = elm_object_item_data_get(it);
        if (fti->enventor_it != enventor_it) continue;
        fd->selected_it = it;
        elm_list_item_selected_set(it, EINA_TRUE);
        elm_list_item_bring_in(it);
        return EINA_TRUE;
     }

   fd->selected_it = NULL;
   return EINA_FALSE;
}

void
file_tab_clear(void)
{
   file_data *fd = g_fd;
   EINA_SAFETY_ON_NULL_RETURN(fd);

   Eina_List *list = (Eina_List*) elm_list_items_get(fd->list);
   Eina_List *l;
   Elm_Object_Item *it;

   EINA_LIST_FOREACH(list, l, it)
     {
        file_tab_it *fti = elm_object_item_data_get(it);
        ecore_animator_del(fti->animator);
        free(fti);
     }
   elm_list_clear(fd->list);
}

Eina_Bool
file_tab_it_add(Enventor_Item *enventor_it)
{
   if (!enventor_it)
     {
        EINA_LOG_ERR("enventor_it = NULL?");
        return EINA_FALSE;
     }

   file_data *fd = g_fd;
   if (!fd) return EINA_FALSE;

   file_tab_it *fti = NULL;
   fti = calloc(1, sizeof(file_tab_it));
   if (!fti)
     {
        mem_fail_msg();
        return EINA_FALSE;
     }

   const char *filepath = enventor_item_file_get(enventor_it);
   if (!filepath)
     {
        EINA_LOG_ERR("No file path??");
        goto err;
     }

   //Filter out file path and just have a file name without extension.
   char *filename = ecore_file_strip_ext(ecore_file_file_get(filepath));
   if (!filename)
     {
        EINA_LOG_ERR("no filename??");
        goto err;
     }

   fti->enventor_it = enventor_it;

   //Close Button
   Evas_Object *btn = elm_button_add(fd->list);
   elm_object_style_set(btn, ENVENTOR_NAME);
   elm_object_focus_allow_set(btn, EINA_FALSE);

   Evas_Object *img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, "close");
   elm_object_content_set(btn, img);

   fti->it = elm_list_item_append(fd->list, filename, NULL, btn,
                                  list_item_selected_cb, fti);

   elm_object_item_tooltip_text_set(fti->it, filepath);

   elm_list_go(fd->list);

   elm_list_item_bring_in(fti->it);

   evas_object_smart_callback_add(btn, "clicked", close_btn_clicked_cb, fti);

   free(filename);

   return EINA_TRUE;

err:
   free(fti);
   return EINA_FALSE;
}

void
file_tab_disabled_set(Eina_Bool disabled)
{
   file_data *fd = g_fd;
   EINA_SAFETY_ON_NULL_RETURN(fd);
   elm_object_disabled_set(fd->list, disabled);

   if (disabled) return;

   //Re-select item. This is a little tricky.
   //When we disable a list, its selected item is dismissed.
   //So, we manually select the item when list is enabled again.
   if (!fd->selected_it) return;
   elm_list_item_selected_set(fd->selected_it, EINA_FALSE);
   elm_list_item_selected_set(fd->selected_it, EINA_TRUE);
}

Evas_Object *
file_tab_init(Evas_Object *parent)
{
   file_data *fd = calloc(1, sizeof(file_data));
   if (!fd)
     {
        mem_fail_msg();
        return NULL;
     }
   g_fd = fd;

   //Outer Box
   Evas_Object *box = elm_box_add(parent);
   elm_box_padding_set(box, ELM_SCALE_SIZE(3), 0);
   elm_box_horizontal_set(box, EINA_TRUE);

   //Left Button
   Evas_Object *left_btn = elm_button_add(box);
   elm_object_style_set(left_btn, ENVENTOR_NAME);
   elm_object_focus_allow_set(left_btn, EINA_FALSE);
   evas_object_size_hint_weight_set(left_btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(left_btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(left_btn, "clicked", left_btn_clicked_cb, fd);
   evas_object_show(left_btn);
   elm_box_pack_end(box, left_btn);

   //Left Button Icon
   Evas_Object *img1 = elm_image_add(left_btn);
   elm_image_file_set(img1, EDJE_PATH, "left_arrow");
   elm_object_content_set(left_btn, img1);

   //Right Button
   Evas_Object *right_btn = elm_button_add(box);
   elm_object_style_set(right_btn, ENVENTOR_NAME);
   elm_object_focus_allow_set(right_btn, EINA_FALSE);
   evas_object_size_hint_weight_set(right_btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(right_btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(right_btn, "clicked",
                                  right_btn_clicked_cb, fd);
   evas_object_show(right_btn);
   elm_box_pack_end(box, right_btn);

   //Right Button Icon
   Evas_Object *img2 = elm_image_add(right_btn);
   elm_image_file_set(img2, EDJE_PATH, "right_arrow");
   elm_object_content_set(right_btn, img2);

   //List
   Evas_Object *list = elm_list_add(box);
   elm_object_style_set(list, ENVENTOR_NAME);
   elm_object_focus_allow_set(list, EINA_FALSE);
   elm_list_horizontal_set(list, EINA_TRUE);
   elm_scroller_policy_set(list, ELM_SCROLLER_POLICY_OFF,
                           ELM_SCROLLER_POLICY_OFF);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(list);
   elm_box_pack_end(box, list);

   fd->box = box;
   fd->list = list;

   return box;
}

void
file_tab_term(void)
{
   file_data *fd = g_fd;
   EINA_SAFETY_ON_NULL_RETURN(fd);

   file_tab_it *fti;
   Eina_List *l;
   Elm_Object_Item *it;
   Eina_List *children = (Eina_List *)elm_list_items_get(fd->list);
   EINA_LIST_FOREACH(children, l, it)
     {
        fti = elm_object_item_data_get(it);
        ecore_animator_del(fti->animator);
        free(fti);
     }

   evas_object_del(fd->box);

   free(fd);
   g_fd = NULL;
}

