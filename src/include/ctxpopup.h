typedef enum attr_value_type
{
   ATTR_VALUE_INTEGER = 1,
   ATTR_VALUE_FLOAT = 2,
   ATTR_VALUE_CONSTANT = 4,
   ATTR_VALUE_PART = 8,
   ATTR_VALUE_STATE = 16,
   ATTR_VALUE_IMAGE = 32,
   ATTR_VALUE_PROGRAM = 64
} attr_value_type;

struct attr_value_s
{
   Eina_List *strs;
   float min;
   float max;
   attr_value_type type;
   Eina_Bool program : 1;
};

Evas_Object * ctxpopup_candidate_list_create(edit_data *ed, attr_value *attr, double slider_val, Evas_Smart_Cb ctxpopup_dismiss_cb, Evas_Smart_Cb ctxpopup_selected_cb);
Evas_Object * ctxpopup_img_preview_create(edit_data *ed, const char *imgpath, Evas_Smart_Cb ctxpopup_dismiss_cb, Evas_Smart_Cb ctxpopup_relay_cb);



