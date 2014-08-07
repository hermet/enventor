#include <Elementary.h>
#include "common.h"

const char *NAME_SEED = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const int NAME_SEED_LEN = 52;

const char *
template_part_first_line_get(void)
{
   static char buf[40];
   char name[8];
   int i;

   for (i = 0; i < 8; i++)
     name[i] = NAME_SEED[(rand() % NAME_SEED_LEN)];

   snprintf(buf, sizeof(buf), "part { name: \"%s\";<br/>", name);

   return (const char *) buf;
}
