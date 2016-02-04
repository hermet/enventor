#define LIVE_EDIT_REL1 0.25
#define LIVE_EDIT_REL2 0.75
#define LIVE_EDIT_FONT "Sans"
#define LIVE_EDIT_FONT_SIZE 10

void live_edit_init(Evas_Object *trigger);
void live_edit_term(void);
Eina_Bool live_edit_cancel(void);
Eina_Bool live_edit_get(void);
void live_edit_update(void);
Evas_Object *live_edit_tools_create(Evas_Object *parent);
