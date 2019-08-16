/***
 * Compatible ABI for Win32
 ***/
#ifdef _WIN32
# ifdef EAPI
#  undef EAPI
# endif
# ifdef EOAPI
#  undef EOAPI
# endif
# ifdef ENVENTOR_WIN32_BUILD_SUPPORT
#  define EAPI __declspec(dllexport)
#  define EOAPI __declspec(dllexport)
# else
#  define EAPI __declspec(dllimport)
#  define EOAPI __declspec(dllimport)
# endif
#endif
#include "enventor_object.eo.h"
