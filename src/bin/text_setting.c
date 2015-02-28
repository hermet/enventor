#include "common.h"
#include "text_setting.h"

#define COLOR_KEYWORD_MAX_CNT 76
#define SYNTAX_TEMPLATE_MAX_LEN 3072
#define SYNTAX_TEMPLATE_FONT_SIZE 10
#define SYNTAX_COLOR_LEN 7

static char color_val[ENVENTOR_SYNTAX_COLOR_LAST][SYNTAX_COLOR_LEN] = {{0}};

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

static text_setting_data *g_tsd = NULL;

static void
syntax_template_set(char *syntax_template_str, char *syntax_template_format,
                    double font_scale)
{
   if (!syntax_template_str || !syntax_template_format) return;

   snprintf(syntax_template_str, SYNTAX_TEMPLATE_MAX_LEN, syntax_template_format,
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
syntax_template_apply(void)
{
   text_setting_data *tsd = g_tsd;
   Evas_Object *layout = tsd->text_setting_layout;
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

void
text_setting_syntax_color_reset(void)
{
   text_setting_syntax_color_load();

   syntax_template_apply();
}

void
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
        strcat(color, buf);
     }
   color[SYNTAX_COLOR_LEN - 1] = '\0';

   //Set the extracted color value to the selected color keyword
   strncpy(color_val[selected_color_keyword->color_type], color, 7);
}

static void
color_btn_up_cb(void *data, Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   Evas_Object *layout = data;
   Evas_Object *slider = elm_object_part_content_get(layout,
                                                     "elm.swallow.slider");
   Evas_Object *entry = elm_object_part_content_get(layout,
                                                    "elm.swallow.entry");
   double value = elm_slider_value_get(slider);
   char buf[128];

   value += 1;

   snprintf(buf, sizeof(buf), "%1.0f", value);
   elm_object_text_set(entry, buf);
}

static void
color_btn_down_cb(void *data, Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   Evas_Object *layout = data;
   Evas_Object *slider = elm_object_part_content_get(layout,
                                                     "elm.swallow.slider");
   Evas_Object *entry = elm_object_part_content_get(layout,
                                                    "elm.swallow.entry");
   double value = elm_slider_value_get(slider);
   char buf[128];

   value -= 1;

   snprintf(buf, sizeof(buf), "%1.0f", value);
   elm_object_text_set(entry, buf);
}

static void
color_ctxpopup_dismiss_cb(void *data EINA_UNUSED, Evas_Object *obj,
                          void *event_info EINA_UNUSED)
{
   evas_object_del(obj);
}

static void
color_ctxpopup_del_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                      Evas_Object *obj, void *event_info EINA_UNUSED)
{
   text_setting_data *tsd = g_tsd;
   color_keyword *selected_color_keyword;
   selected_color_keyword = evas_object_data_get(obj, "color_keyword");

   elm_object_disabled_set(tsd->text_setting_layout, EINA_FALSE);
   elm_object_focus_set(tsd->slider_font, EINA_TRUE);

   text_setting_syntax_color_update(obj, selected_color_keyword);
   syntax_template_apply();

   elm_config_focus_autoscroll_mode_set(ELM_FOCUS_AUTOSCROLL_MODE_SHOW);

   tsd->color_ctxpopup = NULL;
}

static void
color_slider_changed_cb(void *data, Evas_Object *obj,
                        void *event_info EINA_UNUSED)
{
   Evas_Object *entry = data;
   double val = elm_slider_value_get(obj);
   char buf[128];

   snprintf(buf, sizeof(buf), "%1.0f", val);
   elm_object_text_set(entry, buf);

   Evas_Object *ctxpopup = evas_object_data_get(obj, "ctxpopup");
   color_keyword *selected_color_keyword;
   selected_color_keyword = evas_object_data_get(ctxpopup, "color_keyword");

   text_setting_syntax_color_update(ctxpopup, selected_color_keyword);
   syntax_template_apply();
}

static void
color_entry_changed_cb(void *data, Evas_Object *obj,
                       void *event_info EINA_UNUSED)
{
   Evas_Object *slider = data;
   double text_val, val, min_val, max_val;
   char buf[128];

   text_val = atof(elm_object_text_get(obj));
   elm_slider_min_max_get(slider, &min_val, &max_val);

   if (text_val < min_val) val = min_val;
   else if (text_val > max_val) val = max_val;
   else val = text_val;

   if (val != text_val)
     {
        snprintf(buf, sizeof(buf), "%1.0f", val);
        elm_object_text_set(obj, buf);
     }
   else
     elm_slider_value_set(slider, val);
}

static Evas_Object *
color_slider_layout_create(Evas_Object *parent, Evas_Object *ctxpopup,
                           const char *type, double slider_val)
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

   //Entry
   char buf[128];
   Evas_Object *entry = elm_entry_add(layout);
   elm_entry_context_menu_disabled_set(entry, EINA_TRUE);
   elm_entry_single_line_set(entry, EINA_TRUE);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   snprintf(buf, sizeof(buf), "%1.0f", slider_val);
   elm_object_text_set(entry, buf);
   elm_object_part_content_set(layout, "elm.swallow.entry", entry);

   Elm_Entry_Filter_Accept_Set digits_filter_data;
   Elm_Entry_Filter_Limit_Size limit_filter_data;
   digits_filter_data.accepted = "0123456789";
   digits_filter_data.rejected = NULL;
   limit_filter_data.max_char_count = 4;
   elm_entry_markup_filter_append(entry, elm_entry_filter_accept_set,
                                  &digits_filter_data);
   elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size,
                                  &limit_filter_data);

   evas_object_smart_callback_add(slider, "changed", color_slider_changed_cb,
                                  entry);
   evas_object_smart_callback_add(entry, "changed", color_entry_changed_cb,
                                  slider);

   Evas_Object *btn;
   Evas_Object *img;

   //Up Button
   btn = elm_button_add(layout);
   evas_object_smart_callback_add(btn, "clicked", color_btn_up_cb, layout);
   elm_object_part_content_set(layout, "elm.swallow.up", btn);

   //Up Image
   img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, "up");
   elm_object_content_set(btn, img);

   //Down Button
   btn = elm_button_add(layout);
   evas_object_smart_callback_add(btn, "clicked", color_btn_down_cb, layout);
   elm_object_part_content_set(layout, "elm.swallow.down", btn);

   //Down Image
   img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, "down");
   elm_object_content_set(btn, img);

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
color_slider_layout_set(Evas_Object *ctxpopup)
{
   Eina_Array *type_array;
   Eina_Stringshare *type;
   Eina_Array_Iterator itr;
   unsigned int i;
   const char *color;
   char color_rgb_str[3][3] = {{0}};
   int color_rgb_val[3];
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
        layout = color_slider_layout_create(box, ctxpopup, type,
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
color_ctxpopup_create(Evas_Object *parent,
                      color_keyword *selected_color_keyword)
{
   Evas_Object *ctxpopup = elm_ctxpopup_add(parent);
   if (!ctxpopup) return NULL;

   elm_config_focus_autoscroll_mode_set(ELM_FOCUS_AUTOSCROLL_MODE_NONE);

   elm_object_style_set(ctxpopup, elm_app_name_get());
   evas_object_data_set(ctxpopup, "color_keyword", selected_color_keyword);
   elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_RIGHT,
                                       ELM_CTXPOPUP_DIRECTION_LEFT,
                                       ELM_CTXPOPUP_DIRECTION_UP,
                                       ELM_CTXPOPUP_DIRECTION_DOWN);
   color_slider_layout_set(ctxpopup);

   evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL,
                                  color_ctxpopup_del_cb, NULL);
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
        EINA_LOG_ERR("Failed to allocate Memory!");
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
syntax_template_format_create(void)
{
   text_setting_data *tsd = g_tsd;

   text_setting_syntax_color_load();

   char file_path[PATH_MAX];
   snprintf(file_path, sizeof(file_path), "%s/color/syntax_template.dat",
            elm_app_data_dir_get());

   Eina_File *file = NULL;
   file = eina_file_open(file_path, EINA_FALSE);
   if (!file)
     {
        EINA_LOG_ERR("Failed to open file \"%s\"", file_path);
        return NULL;
     }

   char *utf8 = eina_file_map_all(file, EINA_FILE_POPULATE);
   if (!utf8) goto syntax_template_format_create_err;

   char *syntax_template_format = calloc(1, sizeof(char) * (strlen(utf8) + 1));
   if (!syntax_template_format) goto syntax_template_format_create_err;
   strcpy(syntax_template_format, utf8);

   tsd->syntax_template_format = syntax_template_format;

   eina_file_close(file);

   return tsd->syntax_template_format;

syntax_template_format_create_err:
   EINA_LOG_ERR("Failed to allocate Memory!");
   if (utf8) free(utf8);

   eina_file_close(file);

   return NULL;
}

static char *
syntax_template_create(double font_scale)
{
   text_setting_data *tsd = g_tsd;
   char *syntax_template_format = syntax_template_format_create();
   if (!syntax_template_format) goto syntax_template_create_err;

   char *syntax_template_str = NULL;
   syntax_template_str = calloc(1, sizeof(char) * SYNTAX_TEMPLATE_MAX_LEN);
   if (!syntax_template_str) goto syntax_template_create_err;

   syntax_template_set(syntax_template_str, syntax_template_format, font_scale);

   color_keyword *color_keyword_list;
   color_keyword_list = color_keyword_list_create(syntax_template_str);
   if (!color_keyword_list) goto syntax_template_create_err;

   tsd->syntax_template_str = syntax_template_str;
   tsd->color_keyword_list = color_keyword_list;

   return tsd->syntax_template_str;

syntax_template_create_err:
   EINA_LOG_ERR("Failed to allocate Memory!");
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
             ctxpopup = color_ctxpopup_create(tsd->text_setting_layout,
                                              selected_color_keyword);
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
toggle_create(Evas_Object *parent, const char *text, Eina_Bool state)
{
   Evas_Object *toggle = elm_check_add(parent);
   elm_object_style_set(toggle, "toggle");
   elm_check_state_set(toggle, state);
   evas_object_size_hint_weight_set(toggle, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(toggle, EVAS_HINT_FILL, 0);
   elm_object_text_set(toggle, text);
   evas_object_show(toggle);

   return toggle;
}

static void
font_scale_slider_changed_cb(void *data, Evas_Object *obj,
                             void *event_info EINA_UNUSED)
{
   text_setting_data *tsd = data;
   double val = elm_slider_value_get(obj);
   tsd->font_scale = val;

   syntax_template_apply();
}

Evas_Object *
text_setting_layout_create(Evas_Object *parent)
{
   text_setting_data *tsd = g_tsd;

   //Layout
   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "text_setting_layout");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(layout);

   //Text Editor
   Evas_Object *entry = elm_entry_add(layout);
   elm_entry_context_menu_disabled_set(entry, EINA_TRUE);
   elm_entry_line_wrap_set(entry, ELM_WRAP_NONE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_FALSE);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);

   tsd->font_scale = (double) config_font_scale_get();
   char *syntax_template_str = syntax_template_create(tsd->font_scale);
   elm_entry_entry_set(entry, syntax_template_str);
   evas_object_smart_callback_add(entry, "clicked,double",
                                  text_setting_double_clicked_cb, tsd);
   elm_object_focus_set(entry, EINA_TRUE);
   elm_object_part_content_set(layout, "elm.swallow.text_setting", entry);

   //Preference
   Evas_Object *scroller = elm_scroller_add(layout);
   elm_object_part_content_set(layout, "elm.swallow.preference", scroller);

   //Box
   Evas_Object *box = elm_box_add(scroller);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, 0);
   evas_object_show(box);

   elm_object_content_set(scroller, box);

   //Font Size (Box)
   Evas_Object *box2 = elm_box_add(box);
   elm_box_horizontal_set(box2, EINA_TRUE);
   evas_object_size_hint_weight_set(box2, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(box2, EVAS_HINT_FILL, 0);
   evas_object_show(box2);

   elm_box_pack_end(box, box2);

   //Font Size (Slider)
   Evas_Object *slider_font = elm_slider_add(box2);
   evas_object_size_hint_weight_set(slider_font, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(slider_font, EVAS_HINT_FILL, 0);
   elm_slider_span_size_set(slider_font, 190);
   elm_slider_indicator_show_set(slider_font, EINA_FALSE);
   elm_slider_unit_format_set(slider_font, "%1.1fx");
   elm_slider_min_max_set(slider_font, MIN_FONT_SCALE, MAX_FONT_SCALE);
   elm_slider_value_set(slider_font, tsd->font_scale);
   elm_object_text_set(slider_font, "Font Size ");
   evas_object_smart_callback_add(slider_font, "changed",
                                  font_scale_slider_changed_cb, tsd);
   evas_object_show(slider_font);
   elm_object_focus_set(slider_font, EINA_TRUE);

   elm_box_pack_end(box2, slider_font);

   //Toggle (Line Number)
   Evas_Object *toggle_linenum = toggle_create(box, "Line Number",
                                               config_linenumber_get());
   elm_box_pack_end(box, toggle_linenum);

   //Toggle (Auto Indentation)
   Evas_Object *toggle_indent = toggle_create(box, "Auto Indentation",
                                              config_auto_indent_get());
   elm_box_pack_end(box, toggle_indent);

   //Toggle (Auto Completion)
   Evas_Object *toggle_autocomp = toggle_create(box, "Auto Completion",
                                                config_auto_complete_get());
   elm_box_pack_end(box, toggle_autocomp);

   tsd->text_setting_layout = layout;
   tsd->slider_font = slider_font;
   tsd->toggle_linenum = toggle_linenum;
   tsd->toggle_indent = toggle_indent;
   tsd->toggle_autocomp = toggle_autocomp;

   return layout;
}

void
text_setting_layout_show(Evas_Object *setting_layout, Evas_Object *tabbar,
                         Evas_Object *apply_btn, Evas_Object *reset_btn,
                         Evas_Object *cancel_btn)
{
   text_setting_data *tsd = g_tsd;
   Evas_Object *content;

   if (!setting_layout) return;

   content = elm_object_part_content_get(setting_layout,
                                         "elm.swallow.content");

   if (content == tsd->text_setting_layout) return;

   elm_object_part_content_unset(setting_layout, "elm.swallow.content");
   evas_object_hide(content);

   elm_object_part_content_set(setting_layout, "elm.swallow.content",
                               tsd->text_setting_layout);
   elm_object_focus_set(tsd->slider_font, EINA_TRUE);

   //Set a custom chain to set the focus order.
   Eina_List *custom_chain = NULL;
   custom_chain = eina_list_append(custom_chain, tabbar);
   custom_chain = eina_list_append(custom_chain, tsd->text_setting_layout);
   custom_chain = eina_list_append(custom_chain, apply_btn);
   custom_chain = eina_list_append(custom_chain, reset_btn);
   custom_chain = eina_list_append(custom_chain, cancel_btn);
   elm_object_focus_custom_chain_set(setting_layout, custom_chain);
}

void
text_setting_config_set(void)
{
   text_setting_data *tsd = g_tsd;

   config_font_scale_set((float) elm_slider_value_get(tsd->slider_font));
   config_linenumber_set(elm_check_state_get(tsd->toggle_linenum));
   config_auto_indent_set(elm_check_state_get(tsd->toggle_indent));
   config_auto_complete_set(elm_check_state_get(tsd->toggle_autocomp));
}

void
text_setting_font_scale_set(double font_scale)
{
   text_setting_data *tsd = g_tsd;
   tsd->font_scale = font_scale;
   elm_slider_value_set(tsd->slider_font, tsd->font_scale);
}

void
text_setting_linenumber_set(Eina_Bool enabled)
{
   text_setting_data *tsd = g_tsd;
   elm_check_state_set(tsd->toggle_linenum, enabled);
}

void
text_setting_auto_indent_set(Eina_Bool enabled)
{
   text_setting_data *tsd = g_tsd;
   elm_check_state_set(tsd->toggle_indent, enabled);
}

void
text_setting_auto_complete_set(Eina_Bool enabled)
{
   text_setting_data *tsd = g_tsd;
   elm_check_state_set(tsd->toggle_autocomp, enabled);
}

void
text_setting_init(void)
{
   text_setting_data *tsd = g_tsd;
   if (tsd) return;

   tsd = calloc(1, sizeof(text_setting_data));
   if (!tsd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return;
     }
   g_tsd = tsd;
}

void
text_setting_term(void)
{
   text_setting_data *tsd = g_tsd;
   if (!tsd) return;

   if (tsd->color_ctxpopup)
     evas_object_del(tsd->color_ctxpopup);

   if (tsd->color_keyword_list)
     free(tsd->color_keyword_list);
   if (tsd->syntax_template_format)
     free(tsd->syntax_template_format);
   if (tsd->syntax_template_str)
     free(tsd->syntax_template_str);
   free(tsd);
   g_tsd = NULL;
}
