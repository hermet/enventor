#include <Elementary.h>
#include "common.h"


struct indent_s
{
   Eina_Strbuf *strbuf;
};

indent_data *
indent_init(Eina_Strbuf *strbuf)
{
   indent_data *id = malloc(sizeof(indent_data));
   return id;
}

void
indent_term(indent_data *id)
{
   free(id);
}

int
indent_next_line_step(indent_data *id)
{

}
