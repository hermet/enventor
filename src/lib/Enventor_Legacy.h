/***
 * Compatible ABI for Win32
 ***/
#ifdef _WIN32
# ifdef EAPI
#  undef EAPI
# endif
# ifdef ENVENTOR_WIN32_BUILD_SUPPORT
#  define EAPI __declspec(dllexport)
# else
#  define EAPI __declspec(dllimport)
# endif
# ifdef EOAPI
#  undef EOAPI
# endif
#define EOAPI EAPI
#endif

typedef enum
{
   ENVENTOR_CTXPOPUP_TYPE_SLIDER = 0,
   ENVENTOR_CTXPOPUP_TYPE_LIST,
   ENVENTOR_CTXPOPUP_TYPE_TOGGLE,
   ENVENTOR_CTXPOPUP_TYPE_IMAGE
} Enventor_Ctxpopup_Type;

typedef enum
{
   ENVENTOR_PATH_TYPE_EDJ = 0,
   ENVENTOR_PATH_TYPE_IMAGE,
   ENVENTOR_PATH_TYPE_SOUND,
   ENVENTOR_PATH_TYPE_FONT,
   ENVENTOR_PATH_TYPE_DATA,
   ENVENTOR_PATH_TYPE_LAST
} Enventor_Path_Type;

typedef struct
{
   Evas_Coord x;
   Evas_Coord y;
   float relx;
   float rely;
} Enventor_Live_View_Cursor;

typedef struct
{
   Evas_Coord w;
   Evas_Coord h;
} Enventor_Live_View_Size;

typedef struct
{
   int cur_line;
   int max_line;
} Enventor_Cursor_Line;

typedef struct
{
   Eina_Bool self_changed : 1;
} Enventor_EDC_Modified;

typedef enum {
   ENVENTOR_TEMPLATE_INSERT_DEFAULT,
   ENVENTOR_TEMPLATE_INSERT_LIVE_EDIT
} Enventor_Template_Insert_Type;

typedef enum {
   ENVENTOR_SYNTAX_COLOR_STRING,
   ENVENTOR_SYNTAX_COLOR_COMMENT,
   ENVENTOR_SYNTAX_COLOR_MACRO,
   ENVENTOR_SYNTAX_COLOR_SYMBOL,
   ENVENTOR_SYNTAX_COLOR_MAIN_KEYWORD,
   ENVENTOR_SYNTAX_COLOR_SUB_KEYWORD,
   ENVENTOR_SYNTAX_COLOR_CONSTANT,
   ENVENTOR_SYNTAX_COLOR_SCRIPT_FUNC,
   ENVENTOR_SYNTAX_COLOR_SCRIPT_KEYWORD,
   ENVENTOR_SYNTAX_COLOR_LAST
} Enventor_Syntax_Color_Type;

EAPI int enventor_init(int argc, char **argv);
EAPI int enventor_shutdown(void);
EAPI Evas_Object *enventor_object_add(Evas_Object *parent);
EAPI Eina_Bool enventor_object_file_set(Evas_Object *obj, const char *file);

#include "enventor_object.eo.legacy.h"
