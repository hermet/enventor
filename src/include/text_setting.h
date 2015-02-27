typedef struct color_keyword_s
{
   int pos_begin;
   int pos_end;
   int color_type;
} color_keyword;

struct text_setting_s
{
   Evas_Object *text_setting_layout;
   Evas_Object *color_ctxpopup;

   Evas_Object *slider_font;
   Evas_Object *toggle_linenum;
   Evas_Object *toggle_indent;
   Evas_Object *toggle_autocomp;

   color_keyword *color_keyword_list;
   char *syntax_template_format;
   char *syntax_template_str;

   const char *font_name;
   const char *font_style;
   const char *orig_font_tag;
   const char *cur_font_tag;
   double font_scale;
};

typedef struct text_setting_s text_setting_data;

Evas_Object *text_setting_layout_create(Evas_Object *parent);
void text_setting_layout_show(Evas_Object *setting_layout, Evas_Object *tabbar, Evas_Object *apply_btn, Evas_Object *reset_btn, Evas_Object *cancel_btn);
void text_setting_syntax_color_reset(void);
void text_setting_syntax_color_save(void);
void text_setting_config_set(void);
void text_setting_font_set(const char *font_name, const char *font_style);
void text_setting_font_scale_set(double font_scale);
void text_setting_linenumber_set(Eina_Bool enabled);
void text_setting_auto_indent_set(Eina_Bool enabled);
void text_setting_auto_complete_set(Eina_Bool enabled);
void text_setting_term(void);
void text_setting_init(void);
