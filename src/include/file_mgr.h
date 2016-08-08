Eina_Bool file_mgr_warning_is_opened(void);
void file_mgr_warning_close(void);
void file_mgr_warning_open(void);
void file_mgr_init(void);
void file_mgr_term(void);
int file_mgr_edc_modified_get(void);
void file_mgr_reset(void);
void file_mgr_edc_save(void);
Enventor_Item *file_mgr_main_file_set(const char *path);
Enventor_Item *file_mgr_sub_file_add(const char *path);
Enventor_Item *file_mgr_focused_item_get(void);
void file_mgr_file_focus(Enventor_Item *it);
Eina_Bool file_mgr_save_all(void);
Enventor_Item *file_mgr_main_item_get(void);
Eina_Bool file_mgr_modified_get(void);
Eina_Bool file_mgr_file_open(const char *file_path);
void file_mgr_file_del(Enventor_Item *it);

