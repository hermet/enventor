#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "common.h"

#define UNSUPPORTED_FONT_CNT 28
#define UNSUPPORTED_FONT_MAX_LEN 32
//#define COLOR_KEYWORD_MAX_CNT 76   old style
#define COLOR_KEYWORD_MAX_CNT 66
#define SYNTAX_TEMPLATE_MAX_LEN 3072
#define SYNTAX_TEMPLATE_FONT_SIZE 10
#define SYNTAX_COLOR_LEN 7

typedef struct color_keyword_s
{
   int pos_begin;
   int pos_end;
   int color_type;
} color_keyword;

typedef struct text_setting_s
{
   Evas_Object *layout;
   Evas_Object *text_edit_entry;
   Evas_Object *color_ctxpopup;

   Evas_Object *slider_font;
   Evas_Object *spinner_font;
   Evas_Object *list_font_name;
   Evas_Object *list_font_style;

   color_keyword *color_keyword_list;
   char *syntax_template_format;
   char *syntax_template_str;

   const char *font_name;
   const char *font_style;
   double font_scale;
} text_setting_data;

static char unsupported_font_list[UNSUPPORTED_FONT_CNT][UNSUPPORTED_FONT_MAX_LEN] =
{
   "Dingbats", "KacstArt", "KacstBook", "KacstDecorative", "KacstDigital",
   "KacstFarsi", "KacstLetter", "KacstNaskh", "KacstOffice", "KacstPen",
   "KacstPoster", "KacstQurn", "KacstScreen", "KacstTitle", "KacstTitleL",
   "LKLUG", "Lohit Bengali", "Lohit Gujarati", "Lohit Punjabi", "Lohit Tamil",
   "OpenSymbol", "Pothana2000", "Saab", "Standard Symbols L", "Symbol",
   "Vemana2000", "ori1Uni", "mry_KacstQurn"
};

static char color_val[ENVENTOR_SYNTAX_COLOR_LAST][SYNTAX_COLOR_LEN] = {{0}};

static int color_type_list[COLOR_KEYWORD_MAX_CNT] =
{
   //comment ...  #define ...
   ENVENTOR_SYNTAX_COLOR_COMMENT,        ENVENTOR_SYNTAX_COLOR_MACRO,
   //rect { ...
   ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD,   ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_STRING,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //desc { ...
   ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD,   ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_STRING,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //rel1 { ...
   ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD,   ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //relative ...
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //0.0; rel2 ...
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD,
   //.relative ...
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,
   //: 1.0 1.0; ...
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //color: ...
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //RECT_COLOR; ...
   ENVENTOR_SYNTAX_COLOR_MACRO,          ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //} } ...
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //program { ...
   ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD,   ENVENTOR_SYNTAX_COLOR_SYMBOL,
   // "mouse_down"; ...
   ENVENTOR_SYNTAX_COLOR_STRING,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //signal: ...
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   // "mouse,down,1" ...
   ENVENTOR_SYNTAX_COLOR_STRING,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //source: ...
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //"rect" ...
   ENVENTOR_SYNTAX_COLOR_STRING,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //action: ...
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //STATE_SET ...
   ENVENTOR_SYNTAX_COLOR_CONSTANT,       ENVENTOR_SYNTAX_COLOR_STRING,
   //0.0; target ...
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,
   //: "rect" ...
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_STRING,
   //"; } ...
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //script { ...
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //public flag = ...
   ENVENTOR_SYNTAX_COLOR_SCRIPT_KEYWORD, ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //0; public ...
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SCRIPT_KEYWORD,
   //func() { if ...
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SCRIPT_KEYWORD,
   //(!get_int ...
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SCRIPT_FUNC,
   //(flag))...
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //set_int(...
   ENVENTOR_SYNTAX_COLOR_SCRIPT_FUNC,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //flag, 1);... 
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   //} }
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SYMBOL
};


/* old style */
#if 0
static int color_type_list[COLOR_KEYWORD_MAX_CNT] =
{
   ENVENTOR_SYNTAX_COLOR_COMMENT,        ENVENTOR_SYNTAX_COLOR_MACRO,
   ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD,   ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_STRING,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_CONSTANT,       ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD,   ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_STRING,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD,   ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_MACRO,          ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD,   ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_STRING,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_STRING,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_STRING,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_CONSTANT,       ENVENTOR_SYNTAX_COLOR_STRING,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_STRING,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SCRIPT_KEYWORD, ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SCRIPT_KEYWORD,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SCRIPT_KEYWORD,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SCRIPT_FUNC,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SCRIPT_FUNC,    ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,         ENVENTOR_SYNTAX_COLOR_SYMBOL
};
#endif

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/

static void
syntax_template_set(char *syntax_template_str, char *syntax_template_format,
                    double font_scale)
{
   if (!syntax_template_str || !syntax_template_format) return;

   snprintf(syntax_template_str, SYNTAX_TEMPLATE_MAX_LEN,
            syntax_template_format,
            (int) ((SYNTAX_TEMPLATE_FONT_SIZE * font_scale) + 0.5),
            color_val[color_type_list[0]],  color_val[color_type_list[1]],
            color_val[color_type_list[2]],  color_val[color_type_list[3]],
            color_val[color_type_list[4]],  color_val[color_type_list[5]],
            color_val[color_type_list[6]],  color_val[color_type_list[7]],
            color_val[color_type_list[8]],  color_val[color_type_list[9]],
            color_val[color_type_list[10]], color_val[color_type_list[11]],
            color_val[color_type_list[12]], color_val[color_type_list[13]],
            color_val[color_type_list[14]], color_val[color_type_list[15]],
            color_val[color_type_list[16]], color_val[color_type_list[17]],
            color_val[color_type_list[18]], color_val[color_type_list[19]],
            color_val[color_type_list[20]], color_val[color_type_list[21]],
            color_val[color_type_list[22]], color_val[color_type_list[23]],
            color_val[color_type_list[24]], color_val[color_type_list[25]],
            color_val[color_type_list[26]], color_val[color_type_list[27]],
            color_val[color_type_list[28]], color_val[color_type_list[29]],
            color_val[color_type_list[30]], color_val[color_type_list[31]],
            color_val[color_type_list[32]], color_val[color_type_list[33]],
            color_val[color_type_list[34]], color_val[color_type_list[35]],
            color_val[color_type_list[36]], color_val[color_type_list[37]],
            color_val[color_type_list[38]], color_val[color_type_list[39]],
            color_val[color_type_list[40]], color_val[color_type_list[41]],
            color_val[color_type_list[42]], color_val[color_type_list[43]],
            color_val[color_type_list[44]], color_val[color_type_list[45]],
            color_val[color_type_list[46]], color_val[color_type_list[47]],
            color_val[color_type_list[48]], color_val[color_type_list[49]],
            color_val[color_type_list[50]], color_val[color_type_list[51]],
            color_val[color_type_list[52]], color_val[color_type_list[53]],
            color_val[color_type_list[54]], color_val[color_type_list[55]],
            color_val[color_type_list[56]], color_val[color_type_list[57]],
            color_val[color_type_list[58]], color_val[color_type_list[59]],
            color_val[color_type_list[60]], color_val[color_type_list[61]],
            color_val[color_type_list[62]], color_val[color_type_list[63]],
            color_val[color_type_list[64]], color_val[color_type_list[65]],
            color_val[color_type_list[66]], color_val[color_type_list[67]],
            color_val[color_type_list[68]], color_val[color_type_list[69]],
            color_val[color_type_list[70]], color_val[color_type_list[71]],
            color_val[color_type_list[72]], color_val[color_type_list[73]],
            color_val[color_type_list[74]], color_val[color_type_list[75]]);
}

static void
syntax_template_apply(text_setting_data *tsd)
{
   Evas_Object *layout = tsd->layout;
   if (!layout) return;

   Evas_Object *entry = elm_object_part_content_get(layout,
                                                    "elm.swallow.text_setting");
   syntax_template_set(tsd->syntax_template_str, tsd->syntax_template_format,
                       tsd->font_scale);
   elm_entry_entry_set(entry, tsd->syntax_template_str);
}

static void
text_setting_syntax_color_load(void)
{
   const char *color;
   Enventor_Syntax_Color_Type color_type = ENVENTOR_SYNTAX_COLOR_STRING;
   for (; color_type < ENVENTOR_SYNTAX_COLOR_LAST; color_type++)
     {
        color = config_syntax_color_get(color_type);
        if (color) strncpy(color_val[color_type], color, 6);
        else strncpy(color_val[color_type], "FFFFFF", 6);
     }
}

static void
text_setting_syntax_color_reset(text_setting_data *tsd)
{
   text_setting_syntax_color_load();
   syntax_template_apply(tsd);
}

static void
text_setting_syntax_color_save(void)
{
   Enventor_Syntax_Color_Type color_type = ENVENTOR_SYNTAX_COLOR_STRING;
   for (; color_type < ENVENTOR_SYNTAX_COLOR_LAST; color_type++)
     {
        config_syntax_color_set(color_type, color_val[color_type]);
     }
}

static void
text_setting_syntax_color_update(Evas_Object *ctxpopup,
                                 color_keyword *selected_color_keyword)
{
   Evas_Object *box = elm_object_content_get(ctxpopup);
   Evas_Object *layout;
   Evas_Object *slider;
   Eina_List *box_children = elm_box_children_get(box);
   Eina_List *l;
   char color[SYNTAX_COLOR_LEN] = {0};
   char buf[3];

   if (eina_list_count(box_children) == 0) return;

   //Extract color value from sliders
   EINA_LIST_FOREACH(box_children, l, layout)
     {
        slider = elm_object_part_content_get(layout,
                                             "elm.swallow.slider");
        snprintf(buf, sizeof(buf), "%02X",
                 (int) roundf(elm_slider_value_get(slider)));
        strncat(color, buf, strlen(buf));
     }
   color[SYNTAX_COLOR_LEN - 1] = '\0';

   //Set the extracted color value to the selected color keyword
   strncpy(color_val[selected_color_keyword->color_type], color, 7);
}

static void
color_ctxpopup_dismiss_cb(void *data EINA_UNUSED, Evas_Object *obj,
                          void *event_info EINA_UNUSED)
{
   evas_object_del(obj);
}

static void
color_ctxpopup_del_cb(void *data, Evas *e EINA_UNUSED,
                      Evas_Object *obj, void *event_info EINA_UNUSED)
{
   text_setting_data *tsd = data;
   color_keyword *selected_color_keyword;
   selected_color_keyword = evas_object_data_get(obj, "color_keyword");

   elm_object_disabled_set(tsd->layout, EINA_FALSE);
   elm_object_focus_set(tsd->slider_font, EINA_TRUE);

   text_setting_syntax_color_update(obj, selected_color_keyword);
   syntax_template_apply(tsd);

   elm_config_focus_autoscroll_mode_set(ELM_FOCUS_AUTOSCROLL_MODE_SHOW);

   tsd->color_ctxpopup = NULL;
}

static void
color_slider_changed_cb(void *data, Evas_Object *obj,
                        void *event_info EINA_UNUSED)
{
   text_setting_data *tsd = data;

   Evas_Object *ctxpopup = evas_object_data_get(obj, "ctxpopup");
   color_keyword *selected_color_keyword;
   selected_color_keyword = evas_object_data_get(ctxpopup, "color_keyword");

   text_setting_syntax_color_update(ctxpopup, selected_color_keyword);
   syntax_template_apply(tsd);
}

static Evas_Object *
color_slider_layout_create(text_setting_data *tsd, Evas_Object *parent,
                           Evas_Object *ctxpopup, const char *type,
                           double slider_val)
{
   //Layout
   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "slider_layout");
   evas_object_show(layout);

   //Type
   if (type)
     elm_object_part_text_set(layout, "elm.text.type", type);

   //Slider
   Evas_Object *slider = elm_slider_add(layout);
   evas_object_data_set(slider, "ctxpopup", ctxpopup);
   elm_slider_span_size_set(slider, 120);
   elm_slider_indicator_show_set(slider, EINA_FALSE);
   elm_slider_min_max_set(slider, 0, 255);
   elm_slider_value_set(slider, slider_val);

   char slider_min[16];
   char slider_max[16];
   snprintf(slider_min, sizeof(slider_min), "%1.0f", 0.0);
   snprintf(slider_max, sizeof(slider_max), "%1.0f", 255.0);
   elm_object_part_text_set(layout, "elm.text.slider_min", slider_min);
   elm_object_part_text_set(layout, "elm.text.slider_max", slider_max);
   elm_object_part_content_set(layout, "elm.swallow.slider", slider);
   evas_object_smart_callback_add(slider, "changed", color_slider_changed_cb,
                                  tsd);
   return layout;
}

static int
convert_hexadecimal_to_decimal(char *hexadecimal)
{
   int i;
   int len;
   int decimal = 0;
   char digit;

   if (!hexadecimal) return 0;

   len = strlen(hexadecimal);
   if (len == 0) return 0;

   for (i = 0; i < len; i++)
     {
        digit = hexadecimal[i];

        if ((digit >= 'a') && (digit <= 'f'))
          decimal += ((digit - 'a') + 10) * pow(16, (len - i - 1));
        else if ((digit >= 'A') && (digit <= 'F'))
          decimal += ((digit - 'A') + 10) * pow(16, (len - i - 1));
        else if ((digit >= '0') && (digit <= '9'))
          decimal += atoi(&digit) * pow(16, (len - i - 1));
     }
   return decimal;
}

static void
color_slider_layout_set(text_setting_data *tsd, Evas_Object *ctxpopup)
{
   Eina_Array *type_array;
   Eina_Stringshare *type = NULL;
   Eina_Array_Iterator itr;
   unsigned int i;
   const char *color;
   char color_rgb_str[3][3] = {{0}};
   int color_rgb_val[3] = {0};
   color_keyword *selected_color_keyword;
   selected_color_keyword = evas_object_data_get(ctxpopup, "color_keyword");

   //Box
   Evas_Object *box = elm_box_add(ctxpopup);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   //Layout
   Evas_Object *layout;

   type_array = eina_array_new(3);
   eina_array_push(type_array, eina_stringshare_add("R"));
   eina_array_push(type_array, eina_stringshare_add("G"));
   eina_array_push(type_array, eina_stringshare_add("B"));

   color = config_syntax_color_get(selected_color_keyword->color_type);

   for (i = 0; i < 3; i++)
     {
        strncpy(color_rgb_str[i], &color[i * 2], 2);
        color_rgb_val[i] = convert_hexadecimal_to_decimal(color_rgb_str[i]);
     }

   EINA_ARRAY_ITER_NEXT(type_array, i, type, itr)
     {
        layout = color_slider_layout_create(tsd, box, ctxpopup, type,
                                            color_rgb_val[i]);
        if (i % 2) elm_object_signal_emit(layout, "odd,item,set", "");
        elm_box_pack_end(box, layout);
     }

   while (eina_array_count(type_array))
     eina_stringshare_del(eina_array_pop(type_array));
   eina_array_free(type_array);

   elm_object_content_set(ctxpopup, box);
}

static Evas_Object *
color_ctxpopup_create(text_setting_data *tsd,
                      color_keyword *selected_color_keyword)
{
   Evas_Object *ctxpopup = elm_ctxpopup_add(tsd->layout);
   if (!ctxpopup) return NULL;

   elm_config_focus_autoscroll_mode_set(ELM_FOCUS_AUTOSCROLL_MODE_NONE);

   elm_object_style_set(ctxpopup, ENVENTOR_NAME);
   evas_object_data_set(ctxpopup, "color_keyword", selected_color_keyword);
   elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_RIGHT,
                                       ELM_CTXPOPUP_DIRECTION_LEFT,
                                       ELM_CTXPOPUP_DIRECTION_UP,
                                       ELM_CTXPOPUP_DIRECTION_DOWN);
   color_slider_layout_set(tsd, ctxpopup);

   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL,
                                  color_ctxpopup_del_cb, tsd);
   evas_object_smart_callback_add(ctxpopup, "dismissed",
                                  color_ctxpopup_dismiss_cb, NULL);
   return ctxpopup;
}

static color_keyword *
color_keyword_list_create(char *syntax_template_str)
{
   char *cur = syntax_template_str;
   char *next = NULL;
   int i;

   color_keyword *color_keyword_list = calloc(COLOR_KEYWORD_MAX_CNT,
                                              sizeof(color_keyword));
   if (!color_keyword_list)
     {
        mem_fail_msg();
        return NULL;
     }

   for (i = 0; i < COLOR_KEYWORD_MAX_CNT; i++)
     {
        next = strstr(cur, "<color=#");
        cur = next + 15; //Move position to first character of keyword
        (color_keyword_list + i)->pos_begin = cur - syntax_template_str;

        next = strstr(cur, "</color>");
        cur = next - 1; //Move position to last character of keyword
        (color_keyword_list + i)->pos_end = cur - syntax_template_str;

        (color_keyword_list + i)->color_type = color_type_list[i];
     }

   return color_keyword_list;
}

static char *
syntax_template_format_create(text_setting_data *tsd)
{
   text_setting_syntax_color_load();

   char file_path[PATH_MAX];
   snprintf(file_path, sizeof(file_path), "%s/color/syntax_template.dat",
            elm_app_data_dir_get());

   Eina_File *file = NULL;
   file = eina_file_open(file_path, EINA_FALSE);
   if (!file)
     {
        EINA_LOG_ERR(_("Failed to open file \"%s\""), file_path);
        return NULL;
     }

   char *utf8 = eina_file_map_all(file, EINA_FILE_POPULATE);
   if (!utf8) goto err;

   char *syntax_template_format = calloc(1, sizeof(char) * (strlen(utf8) + 1));
   if (!syntax_template_format) goto err;
   strncpy(syntax_template_format, utf8, strlen(utf8) + 1);

   tsd->syntax_template_format = syntax_template_format;

   eina_file_close(file);

   return tsd->syntax_template_format;

err:
   mem_fail_msg();
   if (utf8) free(utf8);

   eina_file_close(file);

   return NULL;
}

static char *
syntax_template_create(text_setting_data *tsd)
{
   char *syntax_template_str = NULL;
   char *syntax_template_format = syntax_template_format_create(tsd);
   if (!syntax_template_format) goto err;

   syntax_template_str = calloc(1, sizeof(char) * SYNTAX_TEMPLATE_MAX_LEN);
   if (!syntax_template_str) goto err;

   syntax_template_set(syntax_template_str, syntax_template_format,
                       tsd->font_scale);

   color_keyword *color_keyword_list;
   color_keyword_list = color_keyword_list_create(syntax_template_str);
   if (!color_keyword_list) goto err;

   tsd->syntax_template_str = syntax_template_str;
   tsd->color_keyword_list = color_keyword_list;

   return tsd->syntax_template_str;

err:
   mem_fail_msg();

   if (syntax_template_format)
     {
        free(syntax_template_format);
        tsd->syntax_template_format = NULL;
     }
   if (syntax_template_str) free(syntax_template_str);

   return NULL;
}

static int
color_keyword_pos_get(const char *syntax_template_str, const char *selected_str)
{
   Eina_Bool left_arrow_found = EINA_FALSE;
   char *cur;
   int i;
   int len = 0;
   int pos = 0;

   cur = strstr(syntax_template_str, selected_str);
   len = strlen(cur);
   for (i = 0; i < len; i++)
     {
        if (left_arrow_found)
          {
             if (*(cur + i) == '>')
               left_arrow_found = EINA_FALSE;
             continue;
          }

        if (*(cur + i) == '<')
          {
             left_arrow_found = EINA_TRUE;
          }
        else
          {
             pos = (cur + i) - syntax_template_str;
             break;
          }
     }

   return pos;
}

static void
text_setting_double_clicked_cb(void *data, Evas_Object *obj,
                               void *event_info EINA_UNUSED)
{
   text_setting_data *tsd = data;
   color_keyword *selected_color_keyword;
   Evas_Object *ctxpopup;
   const char *syntax_template_str;
   const char *selected_str;
   int i;
   int pos;
   int x, y;

   syntax_template_str = elm_entry_entry_get(obj);
   if (!syntax_template_str) return;

   selected_str = elm_entry_selection_get(obj);
   if (!selected_str) return;

   pos = color_keyword_pos_get(syntax_template_str, selected_str);

   for (i = 0; i < COLOR_KEYWORD_MAX_CNT; i++)
     {
        selected_color_keyword = tsd->color_keyword_list + i;
        if ((pos >= selected_color_keyword->pos_begin) &&
            (pos <= selected_color_keyword->pos_end))
          {
             ctxpopup = color_ctxpopup_create(tsd, selected_color_keyword);
             if (!ctxpopup) return;

             evas_pointer_output_xy_get(evas_object_evas_get(obj), &x, &y);
             evas_object_move(ctxpopup, x, y);
             evas_object_show(ctxpopup);
             tsd->color_ctxpopup = ctxpopup;
             break;
          }
     }
}

static Evas_Object *
label_create(Evas_Object *parent, const char *text)
{
   Evas_Object *label = elm_label_add(parent);
   elm_object_text_set(label, text);
   evas_object_show(label);

   return label;
}

static Evas_Object *
list_create(Evas_Object *parent)
{
   Evas_Object *list = elm_list_add(parent);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(list);

   return list;
}

static void
font_scale_slider_changed_cb(void *data, Evas_Object *obj,
                             void *event_info EINA_UNUSED)
{
   text_setting_data *tsd = data;
   double val = elm_slider_value_get(obj);
   tsd->font_scale = val;

   syntax_template_apply(tsd);
   elm_spinner_value_set(tsd->spinner_font, val);
}

static void
font_scale_spinner_changed_cb(void *data, Evas_Object *obj,
                              void *event_info EINA_UNUSED)
{
   text_setting_data *tsd = data;
   double val = elm_spinner_value_get(obj);
   tsd->font_scale = val;

   syntax_template_apply(tsd);
   elm_slider_value_set(tsd->slider_font, val);
}

static void
text_setting_font_apply(text_setting_data *tsd)
{
   char text_class_name[32];
   snprintf(text_class_name, sizeof(text_class_name), "%s_setting_entry",
            "enventor");

   char *font = NULL;
   if (tsd->font_name)
     font = elm_font_fontconfig_name_get(tsd->font_name, tsd->font_style);
   edje_text_class_set(text_class_name, font, -100);
   elm_font_fontconfig_name_free(font);

   elm_entry_calc_force(tsd->text_edit_entry);
}

static int
font_cmp_cb(const void *data1,
            const void *data2)
{
   if (!data1) return 1;
   if (!data2) return -1;
   return strcmp(data1, data2);
}

static void
font_style_selected_cb(void *data, Evas_Object *obj,
                       void *event_info EINA_UNUSED)
{
   text_setting_data *tsd = data;
   Elm_Object_Item *font_style_it = elm_list_selected_item_get(obj);
   const char *font_style = elm_object_item_text_get(font_style_it);

   eina_stringshare_replace(&tsd->font_style, font_style);
   text_setting_font_apply(tsd);
}

static void
font_name_selected_cb(void *data, Evas_Object *obj,
                      void *event_info EINA_UNUSED)
{
   text_setting_data *tsd = data;
   Elm_Object_Item *font_name_it = elm_list_selected_item_get(obj);
   Elm_Object_Item *font_style_it = NULL;
   const char *sel_font_name = elm_object_item_text_get(font_name_it);
   const char *font_name = NULL;
   const char *font_style = NULL;

   config_font_get(&font_name, &font_style);

   elm_list_clear(tsd->list_font_style);

   //Append Items of Font Style List
   Elm_Font_Properties *efp;
   Eina_List *font_list;
   Eina_List *l, *ll;
   char *font, *style;
   font_list = evas_font_available_list(evas_object_evas_get(obj));
   font_list = eina_list_sort(font_list, eina_list_count(font_list),
                              font_cmp_cb);
   EINA_LIST_FOREACH(font_list, l, font)
     {
        efp = elm_font_properties_get(font);
        if (efp)
          {
             if (!strcmp(sel_font_name, efp->name))
               {
                  EINA_LIST_FOREACH(efp->styles, ll, style)
                    {
                       Elm_Object_Item *it
                          = elm_list_item_append(tsd->list_font_style, style,
                                                 NULL,
                                                 NULL, font_style_selected_cb,
                                                 tsd);
                       if (font_name && !strcmp(font_name, efp->name) &&
                           font_style && !strcmp(font_style, style))
                         font_style_it = it;
                    }
               }
             elm_font_properties_free(efp);
          }
     }
   elm_list_go(tsd->list_font_style);
   if (font_style_it) elm_list_item_selected_set(font_style_it, EINA_TRUE);

   eina_stringshare_replace(&tsd->font_name, sel_font_name);
   eina_stringshare_replace(&tsd->font_style, NULL);
   text_setting_font_apply(tsd);
}

static Eina_Bool
is_supported_font(const char *font_name)
{
   if (!font_name) return EINA_FALSE;

   int i;
   for (i = 0; i < UNSUPPORTED_FONT_CNT; i++)
     {
        if (!strcmp(font_name, unsupported_font_list[i]))
          return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void
text_setting_font_set(text_setting_data *tsd, const char *font_name,
                      const char *font_style)
{
   eina_stringshare_replace(&tsd->font_name, font_name);
   eina_stringshare_replace(&tsd->font_style, font_style);

   if (!tsd->list_font_name) return;

   const Eina_List *it_list = elm_list_items_get(tsd->list_font_name);
   const Eina_List *l;
   Elm_Object_Item *it;

   EINA_LIST_FOREACH(it_list, l, it)
     {
        const char *name = elm_object_item_text_get(it);
        if (font_name && !strcmp(font_name, name))
          {
             elm_list_item_selected_set(it, EINA_TRUE);
             break;
          }
     }
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/

Evas_Object *
text_setting_content_get(text_setting_data *tsd, Evas_Object *parent)
{
   if (!tsd) return NULL;
   if (tsd->layout) return tsd->layout;

   //Layout
   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "text_setting_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);

   //Text Editor
   Evas_Object *entry = elm_entry_add(layout);
   char style_name[128];
   snprintf(style_name, sizeof(style_name), "%s_setting", "enventor");
   elm_object_style_set(entry, style_name);
   elm_entry_context_menu_disabled_set(entry, EINA_TRUE);
   elm_entry_line_wrap_set(entry, ELM_WRAP_NONE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_FALSE);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_layout_text_set(layout, "text_setting_guide", _("Double click a keyword to change its color :"));

   //Font Scale Information
   tsd->font_scale = (double) config_font_scale_get();
   char *syntax_template_str = syntax_template_create(tsd);
   elm_entry_entry_set(entry, syntax_template_str);
   evas_object_smart_callback_add(entry, "clicked,double",
                                  text_setting_double_clicked_cb, tsd);
   elm_object_focus_set(entry, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.text_setting", entry);

   //Box
   Evas_Object *box = elm_box_add(layout);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, 0);
   evas_object_show(box);

   elm_object_part_content_set(layout, "elm.swallow.font_size", box);

   //Font Size (Box)
   Evas_Object *box2 = elm_box_add(box);
   elm_box_horizontal_set(box2, EINA_TRUE);
   evas_object_size_hint_weight_set(box2, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(box2, EVAS_HINT_FILL, 0);
   evas_object_show(box2);

   elm_box_pack_end(box, box2);

   //Font Size (Slider)
   Evas_Object *slider_font = elm_slider_add(box2);
   evas_object_size_hint_weight_set(slider_font, 0, 0);
   evas_object_size_hint_align_set(slider_font, EVAS_HINT_FILL, 0.5);
   elm_slider_span_size_set(slider_font, 450);
   elm_slider_indicator_show_set(slider_font, EINA_FALSE);
   elm_slider_min_max_set(slider_font, MIN_FONT_SCALE, MAX_FONT_SCALE);
   elm_slider_value_set(slider_font, tsd->font_scale);
   double step = 0.01 / (double) (MAX_FONT_SCALE - MIN_FONT_SCALE);
   elm_slider_step_set(slider_font, step);
   evas_object_smart_callback_add(slider_font, "changed",
                                  font_scale_slider_changed_cb, tsd);
   evas_object_show(slider_font);
   elm_object_focus_set(slider_font, EINA_TRUE);

   elm_box_pack_end(box2, slider_font);

   Evas_Object *padding = elm_box_add(box2);
   evas_object_size_hint_min_set(padding, 5, 20);
   elm_box_pack_end(box2, padding);

   // Spinner font
   Evas_Object *spinner_box = elm_box_add(box2);
   evas_object_size_hint_weight_set(spinner_box, EVAS_HINT_EXPAND, 0.5);
   evas_object_size_hint_align_set(spinner_box, EVAS_HINT_FILL, 0.5);
   evas_object_show(spinner_box);

   elm_box_pack_end(box2, spinner_box);

   Evas_Object *spinner_font = elm_spinner_add(spinner_box);
   elm_spinner_label_format_set(spinner_font, "%1.2fx");
   elm_spinner_step_set(spinner_font, 0.01);
   elm_spinner_wrap_set(spinner_font, EINA_TRUE);
   elm_spinner_editable_set(spinner_font, EINA_TRUE);
   elm_spinner_min_max_set(spinner_font, MIN_FONT_SCALE, MAX_FONT_SCALE);
   evas_object_size_hint_align_set(spinner_font, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_spinner_value_set(spinner_font, tsd->font_scale);
   evas_object_size_hint_min_set(spinner_font, 50, 15);
   elm_box_pack_end(spinner_box, spinner_font);
   evas_object_show(spinner_font);
   evas_object_smart_callback_add(spinner_font, "changed", font_scale_spinner_changed_cb, tsd);

   //Font Name and Style (Box)
   box = elm_box_add(layout);
   elm_box_padding_set(box, 0, ELM_SCALE_SIZE(5));
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_part_content_set(layout, "elm.swallow.font", box);

   //Font Name (Box)
   box2 = elm_box_add(box);
   evas_object_size_hint_weight_set(box2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box2);

   elm_box_pack_end(box, box2);

   //Font Name (Label)

   /* This layout is intended to put the label aligned to left side
      far from 3 pixels. */
   Evas_Object *layout_padding3 = elm_layout_add(box2);
   elm_layout_file_set(layout_padding3, EDJE_PATH, "padding3_layout");
   evas_object_size_hint_align_set(layout_padding3, 0.0, EVAS_HINT_FILL);
   evas_object_show(layout_padding3);

   elm_box_pack_end(box2, layout_padding3);

   Evas_Object *label_font_name = label_create(layout_padding3, _("Font Name"));
   elm_object_part_content_set(layout_padding3, "elm.swallow.content",
                               label_font_name);

   //Font Name (List)
   Evas_Object *list_font_name = list_create(box2);
   elm_box_pack_end(box2, list_font_name);

   //Font Style (Box)
   box2 = elm_box_add(box);
   evas_object_size_hint_weight_set(box2, EVAS_HINT_EXPAND, 0.6);
   evas_object_size_hint_align_set(box2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box2);

   elm_box_pack_end(box, box2);

   //Font Style (Label)

   /* This layout is intended to put the label aligned to left side
      far from 3 pixels. */
   layout_padding3 = elm_layout_add(box2);
   elm_layout_file_set(layout_padding3, EDJE_PATH, "padding3_layout");
   evas_object_size_hint_align_set(layout_padding3, 0.0, EVAS_HINT_FILL);
   evas_object_show(layout_padding3);

   elm_box_pack_end(box2, layout_padding3);

   Evas_Object *label_font_style = label_create(layout_padding3, _("Font Style"));
   elm_object_part_content_set(layout_padding3, "elm.swallow.content",
                               label_font_style);

   //Font Style (List)
   tsd->list_font_style = list_create(box2);
   elm_box_pack_end(box2, tsd->list_font_style);

   //Font Name and Style Information
   const char *font_name;
   const char *font_style;
   config_font_get(&font_name, &font_style);
   eina_stringshare_replace(&tsd->font_name, font_name);
   eina_stringshare_replace(&tsd->font_style, font_style);

   //Append Items of Font Name List
   Elm_Font_Properties *efp;
   Eina_List *font_list;
   Eina_List *l;
   Elm_Object_Item *font_name_it = NULL;
   char *font;
   char prev_font[128] = {0};
   font_list = evas_font_available_list(evas_object_evas_get(parent));
   font_list = eina_list_sort(font_list, eina_list_count(font_list),
                              font_cmp_cb);
   EINA_LIST_FOREACH(font_list, l, font)
     {
        efp = elm_font_properties_get(font);
        if (efp)
          {
             if (strcmp(prev_font, efp->name) && is_supported_font(efp->name))
               {
                  Elm_Object_Item *it
                     = elm_list_item_append(list_font_name, efp->name, NULL,
                                            NULL, font_name_selected_cb,
                                            tsd);
                  if (font_name && !strcmp(font_name, efp->name))
                    font_name_it = it;
                  snprintf(prev_font, sizeof(prev_font), "%s", efp->name);
               }
             elm_font_properties_free(efp);
          }
     }
   elm_list_go(list_font_name);
   if (font_name_it)
     {
        elm_list_item_selected_set(font_name_it, EINA_TRUE);
        elm_list_item_show(font_name_it);
     }

   tsd->layout = layout;
   tsd->text_edit_entry = entry;
   tsd->slider_font = slider_font;
   tsd->list_font_name = list_font_name;
   tsd->spinner_font = spinner_font;

   return layout;
}

void
text_setting_focus_set(text_setting_data *tsd)
{
   EINA_SAFETY_ON_NULL_RETURN(tsd);

   elm_object_focus_set(tsd->slider_font, EINA_TRUE);
}

void
text_setting_config_set(text_setting_data *tsd)
{
   if (!tsd) return;

   config_font_set(tsd->font_name, tsd->font_style);
   config_font_scale_set((float) elm_slider_value_get(tsd->slider_font));

   text_setting_syntax_color_save();
}

void
text_setting_reset(text_setting_data *tsd)
{
   if (!tsd) return;

   //font scale
   tsd->font_scale = (double) config_font_scale_get();
   elm_slider_value_set(tsd->slider_font, tsd->font_scale);
   elm_spinner_value_set(tsd->spinner_font, tsd->font_scale);

   //font reset
   const char *font_name, *font_style;
   config_font_get(&font_name, &font_style);
   text_setting_font_set(tsd, font_name, font_style);

   text_setting_syntax_color_reset(tsd);
}

text_setting_data *
text_setting_init(void)
{
   text_setting_data *tsd = calloc(1, sizeof(text_setting_data));
   if (!tsd)
     {
        mem_fail_msg();
        return NULL;
     }
   return tsd;
}

void
text_setting_term(text_setting_data *tsd)
{
   if (!tsd) return;

   evas_object_del(tsd->color_ctxpopup);
   free(tsd->color_keyword_list);
   free(tsd->syntax_template_format);
   free(tsd->syntax_template_str);
   eina_stringshare_del(tsd->font_name);
   eina_stringshare_del(tsd->font_style);
   evas_object_del(tsd->layout);
   free(tsd);
}
