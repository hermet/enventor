#ifndef __COMMON_H__
#define __COMMON_H__

typedef struct statusbar_s stats_data;


#define ENVENTOR_HELP_EXAMPLES \
"Examples of Enventor command line usage:\n \
$ enventor\n \
$ enventor --to\n \
$ enventor newfile.edc --to\n \
$ enventor sample.edc --id ./images --sd ./sounds\n" 

#define ENVENTOR_USAGE \
"%prog [input file] [--to] [--id image path] [--sd sound path] [--fd font path] [--dd data path]"

#define ENVENTOR_INFO \
"\nThis is an EDC editor with some convenient functions.\n \
It's brand new and was only started near the begining of June 2013,\n \
so expecting it to do everything a mature script editor does \n \
is a bit premature, but considering it's young age, it does a lot. \n \
\nPlease see next sites for more information on these.\n \
\n http://www.enlightenment.org \n \
\n https://phab.enlightenment.org/w/projects/enventor/ \n \
\n http://www.openhub.net/p/enventor\n"

#define ENVENTOR_COPYRIGHT \
"Copyright (C) 2013-2015 ChunEon Park and various contributors (see AUTHORS)\n \
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


#define EOL "<br/>"
#define EOL_LEN 5
#define REL1_X 0.25f
#define REL1_Y 0.25f
#define REL2_X 0.75f
#define REL2_Y 0.75f

#define ENVENTOR_BETA_API_SUPPORT 1
#include <Enventor.h>
#include <assert.h>
#include "menu.h"
#include "statusbar.h"
#include "config_data.h"
#include "globals.h"
#include "tools.h"
#include "base_gui.h"
#include "search.h"
#include "goto.h"
#include "newfile.h"
#include "setting.h"
#include "live_edit.h"
#include "console.h"
#include "file_mgr.h"

#endif
