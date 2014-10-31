#ifndef ENVENTOR_H
#define ENVENTOR_H

#ifndef ENVENTOR_BETA_API_SUPPORT
#error "Enventor APIs still unstable. It's under BETA and changeable!! If you really want to use the APIs, Please define ENVENTOR_BETA_API_SUPPORT"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include <Efl_Config.h>
#include <Elementary.h>

#ifndef EFL_NOLEGACY_API_SUPPORT
#include <Enventor_Legacy.h>
#endif

#ifdef EFL_EO_API_SUPPORT
#include <Enventor_Eo.h>
#endif

EAPI int enventor_init(int argc, char **argv);
EAPI int enventor_shutdown(void);
EAPI Evas_Object *enventor_object_add(Evas_Object *parent);

#ifdef __cplusplus
}
#endif

#endif
