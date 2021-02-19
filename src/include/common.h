#ifndef __COMMON_H__
#define __COMMON_H__

#define ENVENTOR_HELP_EXAMPLES \
"Examples of Enventor command line usage:\n \
$ enventor\n \
$ enventor input.edc -i ./images\n \
$ enventor input.edc output.edj -i ./images\n \
$ enventor -t\n \
$ enventor newfile.edc -t\n \
$ enventor input.edc output.edj -i ./images -s ./sounds -w ./workspace\n"

#define ENVENTOR_USAGE \
"%prog [input file] [output file] [-t] [-i image path] [-s sound path] [-f font path] [-d data path] [-w workspace]"

#define ENVENTOR_INFO \
"This is an EDC editor with some convenient functions.\n \
Enventor supports text edit and preview function for EDC source code.\n \
You can edit EDC source code to create complex application layout\n \
using Enventor. This was started near the begining of June 2013,\n \
so expecting it to do everything a mature script editor does is\n \
a bit premature. However, considering its young age, it does a lot.\n \
\n http://www.enlightenment.org \n \
https://phab.enlightenment.org/w/projects/enventor/ \n \
http://www.openhub.net/p/enventor\n"

#define ENVENTOR_COPYRIGHT \
"Copyright (C) 2013-2015 Hermet Park and various contributors (see AUTHORS)\n \
\nAll rights reserved.\n \
\nRedistribution and use in source and binary forms, with or without\n \
modification, are permitted provided that the following conditions are met:\n \
\n   1. Redistributions of source code must retain the above copyright\n \
      notice, this list of conditions and the following disclaimer.\n \
\n   2. Redistributions in binary form must reproduce the above copyright \n \
      notice, this list of conditions and the following disclaimer in the \n \
      documentation and/or other materials provided with the distribution.\n"

#define ENVENTOR_LICENSE \
"\nTHIS SOFTWARE IS PROVIDED \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, \n \
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND\n \
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE\n \
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,\n \
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n \
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,\n \
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF\n \
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n \
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,\n \
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"

#define TRANSIT_TIME 0.25
#define EOL "<br/>"
#define EOL_LEN 5
#define REL1_X 0.25f
#define REL1_Y 0.25f
#define REL2_X 0.75f
#define REL2_Y 0.75f
#define INIT_WIN_W 1100
#define INIT_WIN_H 600

#define ROUNDING(x, dig) (floor((x) * pow(10, dig) + 0.5) / pow(10, dig))

#define ENVENTOR_CONFIG_VERSION 15

#define EVENT_KEY_MODIFIER_CHECK(NAME, MASK) \
  ((MASK & ECORE_EVENT_MODIFIER_##NAME) && \
   !((0xFF ^ ECORE_EVENT_MODIFIER_##NAME) & (MASK & 0x0F)))

#define WIN_DEFAULT_W 900
#define WIN_DEFAULT_H 475

#define ENVENTOR_BETA_API_SUPPORT 1
#include <Enventor.h>
#include <assert.h>
#include "menu.h"
#include "statusbar.h"
#include "config_data.h"
#include "utils.h"
#include "tools.h"
#include "base_gui.h"
#include "search.h"
#include "goto.h"
#include "newfile.h"
#include "setting.h"
#include "live_edit.h"
#include "console.h"
#include "file_mgr.h"
#include "build_setting.h"
#include "preference_setting.h"
#include "text_setting.h"
#include "help.h"
#include "file_tab.h"

/* localization */
#ifdef HAVE_GETTEXT
   #include <libintl.h>
   #define _(string) gettext(string)
   #define N_(string) string
   #define NGETTEXT(single, plur, n) ngettext(single, plur, n)
#else
   #define _(string) string
   #define N_(string) string
   #define NGETTEXT(single, plur, n) (((n)==1)? (single):(plur))
#endif /* localization */

#define ENVENTOR_LOCK_FILE ".enventor.lock"

#endif
