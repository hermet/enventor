#define DEFAULT_EDITOR_SIZE 0.5

Evas_Object *panes_init(Evas_Object *parent);
void panes_term(void);
void panes_text_editor_full_view(void);
void panes_live_view_full_view(void);
void panes_editors_full_view(Eina_Bool full_view);
void panes_console_full_view(void);
void panes_live_view_set(Evas_Object *live_view);
void panes_text_editor_set(Evas_Object *text_editor);
void panes_console_set(Evas_Object *console);
Eina_Bool panes_editors_full_view_get(void);
void panes_live_view_tools_set(Evas_Object *tools);
void panes_live_view_fixed_bar_set(Evas_Object *live_view_fixed_bar);
void panes_text_editor_tools_set(Evas_Object *tools);
void panes_live_view_tools_visible_set(Eina_Bool visible);
void panes_live_view_fixed_bar_visible_set(Eina_Bool visible);
void panes_text_editor_tools_visible_set(Eina_Bool visible);
