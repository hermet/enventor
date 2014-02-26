#ifndef __COMMON_H__
#define __COMMON_H__

typedef struct viewer_s view_data;
typedef struct statusbar_s stats_data;
typedef struct editor_s edit_data;
typedef struct syntax_color_s color_data;
typedef struct parser_s parser_data;
typedef struct attr_value_s attr_value;
typedef struct syntax_helper_s syntax_helper;
typedef struct indent_s indent_data;
typedef struct search_s search_data;

#include "edc_editor.h"
#include "menu.h"
#include "edj_viewer.h"
#include "statusbar.h"
#include "syntax_helper.h"
#include "syntax_color.h"
#include "config_data.h"
#include "edc_parser.h"
#include "dummy_obj.h"
#include "ctxpopup.h"
#include "indent.h"
#include "edj_mgr.h"
#include "globals.h"
#include "build.h"
#include "hotkeys.h"
#include "base_gui.h"
#include "search.h"

#endif
