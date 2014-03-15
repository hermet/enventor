typedef enum attr_value_type
{
   ATTR_VALUE_INTEGER = 1,
   ATTR_VALUE_FLOAT = 2,
   ATTR_VALUE_CONSTANT = 4,
} attr_value_type;

struct attr_value_s
{
   Eina_List *strs;
   float min;
   float max;
   attr_value_type type;
};

Evas_Object * ctxpopup_candidate_list_create(Evas_Object *parent, attr_value *attr, double slider_val, Evas_Smart_Cb ctxpopup_dismiss_cb, Evas_Smart_Cb ctxpopup_selected_cb, void *data);
Evas_Object * ctxpopup_img_preview_create(Evas_Object *parent, const char *imgpath, Evas_Smart_Cb ctxpopup_dismiss_cb, Evas_Smart_Cb ctxpopup_relay_cb, void *data);



