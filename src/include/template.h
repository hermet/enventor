#define REL1_X 0.25f
#define REL1_Y 0.25f
#define REL2_X 0.75f
#define REL2_Y 0.75f

typedef enum {
   TEMPLATE_INSERT_DEFAULT,
   TEMPLATE_INSERT_LIVE_EDIT
} Template_Insert_Type;

void template_part_insert(edit_data *ed, Edje_Part_Type part_type, Template_Insert_Type insert_type, float rel1_x, float rel1_y, float rel2_x, float rel2_y, const Eina_Stringshare *group_name);
void template_insert(edit_data *ed, Template_Insert_Type insert_type);

