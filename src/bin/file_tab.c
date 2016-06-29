#include "common.h"

typedef struct file_tab_s
{
   Evas_Object *in_box;
   Evas_Object *scroller;
   Evas_Object *out_box;
} file_data;

typedef struct file_tab_it_s
{
   Enventor_Item *it;
   file_data *fd;

} file_tab_it;

file_data *g_fd = NULL;

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

Eina_Bool file_tab_it_add(Enventor_Item *enventor_it)
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

   Evas_Object *btn = elm_button_add(fd->in_box);
   elm_object_text_set(btn, filename);
   elm_object_style_set(btn, "enventor");
   evas_object_size_hint_weight_set(btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0, 0.5);
   evas_object_show(btn);

   elm_box_pack_end(fd->in_box, btn);

   free(filename);

   return EINA_TRUE;

err:
   free(fti);
   return EINA_FALSE;
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
   Evas_Object *out_box = elm_box_add(parent);
   elm_box_padding_set(out_box, ELM_SCALE_SIZE(5), 0);
   elm_box_horizontal_set(out_box, EINA_TRUE);

   //Left Button
   Evas_Object *left_btn = elm_button_add(out_box);
   elm_object_style_set(left_btn, ENVENTOR_NAME);
   evas_object_size_hint_weight_set(left_btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(left_btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(left_btn);
   elm_box_pack_end(out_box, left_btn);

   //Left Button Icon
   Evas_Object *img1 = elm_image_add(left_btn);
   elm_image_file_set(img1, EDJE_PATH, "left_arrow");
   elm_object_content_set(left_btn, img1);

   //Right Button
   Evas_Object *right_btn = elm_button_add(out_box);
   evas_object_size_hint_weight_set(right_btn, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(right_btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_style_set(right_btn, ENVENTOR_NAME);
   evas_object_show(right_btn);
   elm_box_pack_end(out_box, right_btn);

   //Right Button Icon
   Evas_Object *img2 = elm_image_add(right_btn);
   elm_image_file_set(img2, EDJE_PATH, "right_arrow");
   elm_object_content_set(right_btn, img2);

   //Scroller
   Evas_Object *scroller = elm_scroller_add(out_box);
   elm_object_style_set(scroller, ENVENTOR_NAME);
   elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF,
                           ELM_SCROLLER_POLICY_OFF);
   evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(scroller);
   elm_box_pack_end(out_box, scroller);

   //Inner Box
   Evas_Object *in_box = elm_box_add(scroller);
   elm_box_horizontal_set(in_box, EINA_TRUE);
   evas_object_size_hint_weight_set(in_box, 0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(in_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(scroller, in_box);

   fd->out_box = out_box;
   fd->in_box = in_box;
   fd->scroller = scroller;

   return out_box;
}

void
file_tab_term(void)
{
   file_data *fd = g_fd;
   if (!fd) return;

   evas_object_del(fd->out_box);

   free(fd);
   g_fd = NULL;
}
