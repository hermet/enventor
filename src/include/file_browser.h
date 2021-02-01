#define DEFAULT_FILE_BROWSER_SIZE 0.3

Evas_Object *file_browser_init(Evas_Object *parent);
void file_browser_term(void);
void file_browser_workspace_set(const char *workspace_dir);
void file_browser_tools_set(void);
void file_browser_tools_visible_set(Eina_Bool visible);
void file_browser_refresh(void);
void file_browser_selected_file_main_set(void);
void file_brwser_refresh(void);
void file_browser_main_file_unset(void);
void file_browser_show(void);
void file_browser_hide(void);
