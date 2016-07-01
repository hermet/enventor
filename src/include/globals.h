extern const char *DEFAULT_EDC_FORMAT;
extern char EDJE_PATH[PATH_MAX];
extern Eina_Prefix *PREFIX;
extern const char *ENVENTOR_NAME;
void mem_fail_msg(void);
Enventor_Item *facade_main_file_set(const char *path);
Enventor_Item *facade_sub_file_add(const char *path);
void facade_it_select(Enventor_Item *it);
