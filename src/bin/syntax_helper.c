#include <Elementary.h>
#include "common.h"


struct syntax_helper_s
{
   color_data *cd;
   indent_data *id;
   Eina_Strbuf *strbuf;
   Ecore_Timer *buf_flush_timer;
};

static Eina_Bool
buf_flush_timer_cb(void *data)
{
   syntax_helper *sh = data;
   /* At this moment, I have no idea the policy of the eina strbuf.
      If the string buffer wouldn't reduce the buffer size, it needs to prevent
      the buffer size not to be grown endlessly. */
   eina_strbuf_free(sh->strbuf);
   sh->strbuf = eina_strbuf_new();

   return ECORE_CALLBACK_RENEW;
}

syntax_helper *
syntax_init(void)
{
   syntax_helper *sh = malloc(sizeof(syntax_helper));
   if (!sh)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }
   sh->strbuf = eina_strbuf_new();
   sh->buf_flush_timer = ecore_timer_add(1800, buf_flush_timer_cb, sh);

   sh->cd = color_init(sh->strbuf);
   sh->id = indent_init(sh->strbuf);

   return sh;
}

void
syntax_term(syntax_helper *sh)
{
   color_term(sh->cd);
   indent_term(sh->id);

   ecore_timer_del(sh->buf_flush_timer);
   eina_strbuf_free(sh->strbuf);
   free(sh);
}

color_data *
syntax_color_data_get(syntax_helper *sh)
{
   return sh->cd;
}

indent_data *
syntax_indent_data_get(syntax_helper *sh)
{
   return sh->id;
}
