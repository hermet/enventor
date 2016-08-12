#define LIVE_EDIT_REL1 0.25
#define LIVE_EDIT_REL2 0.75
#define LIVE_EDIT_FONT "Sans"
#define LIVE_EDIT_FONT_SIZE 12
#define LIVE_EDIT_MAX_DIST 999999
#define LIVE_EDIT_AUTO_ALIGN_DIST 10

Evas_Object *live_edit_init(Evas_Object *parent);
void live_edit_term(void);
Eina_Bool live_edit_cancel(Eina_Bool phase_in);
Eina_Bool live_edit_get(void);
void live_edit_update(void);
Eina_List *live_edit_tools_create(Evas_Object *parent);
Evas_Object *live_edit_fixed_bar_get();
