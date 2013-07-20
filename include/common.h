#ifndef __COMMON_H__
#define __COMMON_H__

#define DEBUG_MODE 1

#ifdef DEBUG_MODE
  #define DFUNC_BEGIN() printf("%s - begin\n", __func__)
  #define DFUNC_END() printf("%s - end\n", __func__)
  #define DFUNC_NAME() printf("%s(%d)\n", __func__, __LINE__)
#else
  #define DFUNC_BEGIN()
  #define DFUNC_END()
  #define DFUNC_NAME()
#endif

extern const char *PROTO_EDC_PATH;
extern char EDJE_PATH[PATH_MAX];

struct attr_value_s
{
   Eina_List *strs;
   float min;
   float max;
   Eina_Bool integer : 1;
};

typedef struct menu_s menu_data;
typedef struct viewer_s view_data;
typedef struct app_s app_data;
typedef struct statusbar_s stats_data;
typedef struct editor_s edit_data;
typedef struct syntax_color_s color_data;
typedef struct config_s option_data;
typedef struct parser_s parser_data;
typedef struct attr_value_s attr_value;
typedef struct fake_obj_s fake_obj;

#include "edc_editor.h"
#include "menu.h"
#include "edc_viewer.h"
#include "statusbar.h"
#include "syntax_color.h"
#include "config_data.h"
#include "edc_parser.h"
#include "panes.h"
#include "fake_obj.h"

#endif
