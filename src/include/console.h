#define DEFAULT_CONSOLE_SIZE 0.135

Evas_Object *console_create(Evas_Object *parent);
void console_text_set(Evas_Object *console, const char *text);
static char *error_msg_syntax_color_set(char *text);
