#include "common.h"

#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT
#include <Edje_Edit.h>

#define IDX_MAX 999999
#define PROGRAM_IDX (IDX_MAX - 1)

#define ESCAPE_GOTO_END() \
   p++; \
   p = strstr(p, "\""); \
   if (!p) goto end; \
   p++; \
   continue

#define ESCAPE_RET_NULL() \
   p++; \
   p = strstr(p, "\""); \
   if (!p) return NULL; \
   p++; \
   continue

typedef struct group_it_s group_it;
typedef struct part_it_s part_it;
typedef struct state_it_s state_it;
typedef struct programs_it_s programs_it;
typedef struct program_it_s program_it;
typedef struct list_it_s list_it;

typedef struct edc_navigator_s
{
   Evas_Object *base_layout;
   Evas_Object *genlist;

   Eina_List *groups;

   Elm_Genlist_Item_Class *group_itc;
   Elm_Genlist_Item_Class *part_itc;
   Elm_Genlist_Item_Class *state_itc;
   Elm_Genlist_Item_Class *programs_itc;
   Elm_Genlist_Item_Class *program_itc;

   Eina_Bool selected : 1;

} navi_data;

typedef enum
{
   Item_Type_Group = 0,
   Item_Type_Part = 1,
   Item_Type_State = 2,
   Item_Type_Programs = 3,
   Item_Type_Program = 4
} Item_Type;

struct list_it_s
{
   Item_Type type;
   int idx;
};

struct programs_it_s
{
   list_it tag;
   Elm_Object_Item *it;
   Eina_List *programs;
   group_it *git;
};

struct group_it_s
{
   list_it tag;
   char *name;
   Elm_Object_Item *it;
   Eina_List *parts;
   navi_data *nd;

   programs_it programs;

   Eina_Bool discarded: 1;
};

struct part_it_s
{
   list_it tag;
   char *name;
   Elm_Object_Item *it;
   Edje_Part_Type type;
   Eina_List *states;
   group_it *git;

   Eina_Bool discarded : 1;
};

struct state_it_s
{
   list_it tag;
   char *name;
   Elm_Object_Item *it;
   part_it *pit;

   Eina_Bool discarded : 1;
};

struct program_it_s
{
   list_it tag;
   char *name;
   Elm_Object_Item *it;
   programs_it *pit;

   Eina_Bool discarded : 1;
};

static navi_data *g_nd = NULL;

static const char *RECT_TYPE_STR = "rect";
static const char *TEXT_TYPE_STR = "text";
static const char *IMAGE_TYPE_STR = "image";
static const char *SWALLOW_TYPE_STR = "swallow";
static const char *TEXTBLOCK_TYPE_STR = "textblock";
static const char *SPACER_TYPE_STR = "spacer";
static const char *PART_TYPE_STR = "part";

static void group_contract(group_it *git);
static void part_contract(part_it *pit);
static void programs_contract(programs_it *pit);

/*****************************************************************************/
/* Internal method implementation                                            */
/*****************************************************************************/
static void
gl_selected_cb(void *data, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   navi_data *nd = data;
   nd->selected = EINA_TRUE;
}

static void
gl_unselected_cb(void *data, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   navi_data *nd = data;
   nd->selected = EINA_FALSE;
}

static void
navigator_item_deselect(navi_data *nd)
{
   Elm_Object_Item *it = elm_genlist_selected_item_get(nd->genlist);
   if (it) elm_genlist_item_selected_set(it, EINA_FALSE);
}

static int
gl_comp_func(const void *pa, const void *pb)
{
   const Elm_Object_Item *a = pa;
   const Elm_Object_Item *b = pb;

   list_it *it_a = (list_it *) elm_object_item_data_get(a);
   list_it *it_b = (list_it *) elm_object_item_data_get(b);

   return (it_a->idx - it_b->idx);
}

static void
navigator_state_free(state_it *sit)
{
   elm_object_item_del(sit->it);
   free(sit->name);
   free(sit);
}

static void
navigator_states_clear(part_it *pit)
{
   state_it *sit;
   EINA_LIST_FREE(pit->states, sit)
     navigator_state_free(sit);

   pit->states = NULL;
}

static void
navigator_program_free(program_it *pit)
{
   elm_object_item_del(pit->it);
   free(pit->name);
   free(pit);
}

static void
navigator_sub_programs_clear(programs_it *pit)
{
   program_it *spit;
   EINA_LIST_FREE(pit->programs, spit)
     navigator_program_free(spit);

   pit->programs = NULL;
}

static void
navigator_part_free(part_it *pit)
{
   state_it *sit;
   EINA_LIST_FREE(pit->states, sit)
     navigator_state_free(sit);

   elm_object_item_del(pit->it);
   free(pit->name);
   free(pit);
}

static void
navigator_parts_clear(group_it *git)
{
   part_it *pit;
   EINA_LIST_FREE(git->parts, pit)
     navigator_part_free(pit);

   git->parts = NULL;
}

static void
navigator_programs_clear(group_it *git)
{
   programs_it *pit = &git->programs;

   if (!pit->it) return;

   program_it *spit;
   EINA_LIST_FREE(pit->programs, spit)
     navigator_program_free(spit);

   pit->programs = NULL;
   elm_object_item_del(pit->it);
   pit->it = NULL;
}

static void
navigator_group_free(group_it *git)
{
   navigator_parts_clear(git);
   navigator_programs_clear(git);
   elm_object_item_del(git->it);
   free(git->name);
   free(git);
}

static void
navigator_groups_clear(navi_data *nd)
{
   group_it *git;
   EINA_LIST_FREE(nd->groups, git)
     navigator_group_free(git);

   nd->groups = NULL;
}

static const char *
part_type_get(part_it *pit)
{
   switch (pit->type)
     {
      case EDJE_PART_TYPE_RECTANGLE:
         return RECT_TYPE_STR;
      case EDJE_PART_TYPE_TEXT:
         return TEXT_TYPE_STR;
      case EDJE_PART_TYPE_IMAGE:
         return IMAGE_TYPE_STR;
      case EDJE_PART_TYPE_SWALLOW:
         return SWALLOW_TYPE_STR;
      case EDJE_PART_TYPE_TEXTBLOCK:
         return TEXTBLOCK_TYPE_STR;
      case EDJE_PART_TYPE_SPACER:
         return SPACER_TYPE_STR;
      default:
         return PART_TYPE_STR;
     }
}

static char *
find_group_proc_internal(char *utf8, char *utf8_end, const char *group_name)
{
   char *p = utf8;
   char *result = NULL;

   int group_name_len = strlen(group_name);

   //Find group
   while (utf8_end > p)
     {
        //Skip " ~ " Section
        if (*p == '\"')
          {
             ESCAPE_RET_NULL();
          }

        if (!strncmp("group", p, strlen("group")))
          {
             p = strstr((p + 5), "\"");
             if (!p) return NULL;
             p++;
             if (!strncmp(group_name, p, group_name_len))
               {
                  //Compare Elaborately
                  char *next_quote = strstr(p, "\"");
                  if (group_name_len == (next_quote - p))
                    {
                       result = p;
                       break;
                    }
                  else
                    {
                       ESCAPE_RET_NULL();
                    }
               }
             else
               {
                  ESCAPE_RET_NULL();
               }
          }
        p++;
     }

   return result;
}

static void
find_group_proc(const char *group_name)
{
   if (!group_name) return;

   const char *text = enventor_item_text_get(file_mgr_focused_item_get());

   if (!text) return;

   char *utf8 = elm_entry_markup_to_utf8(text);
   char *utf8_end = utf8 + strlen(utf8);
   char *result = find_group_proc_internal(utf8, utf8_end, group_name);

   //No found
   if (!result) goto end;

   //Got you!
   enventor_item_select_region_set(file_mgr_focused_item_get(),
                                   (result - utf8),
                                   (result - utf8) + strlen(group_name));
end:
   free(utf8);
}

static char *
find_part_proc_internal(char *utf8, char *utf8_end, const char* group_name,
                        const char *part_name, const char *part_type)
{
   char *p = find_group_proc_internal(utf8, utf8_end, group_name);

   //No found
   if (!p) return NULL;

   p = strstr(p, "\"");
   if (!p) return NULL;
   p++;

   char *result = NULL;

   //goto parts
   p = strstr(p, "parts");
   if (!p) return NULL;

   int part_name_len = strlen(part_name);

   //Find part
   while (utf8_end > p)
     {
        //Skip " ~ " Section
        if (*p == '\"')
          {
             ESCAPE_RET_NULL();
          }

        if (!strncmp(part_type, p, strlen(part_type)))
          {
             p = strstr((p + strlen(part_type)), "\"");
             if (!p) return NULL;
             p++;
             if (!strncmp(part_name, p, part_name_len))
               {
                  //Compare Elaborately
                  char *next_quote = strstr(p, "\"");
                  if (part_name_len == (next_quote - p))
                    {
                       result = p;
                       break;
                    }
                  else
                    {
                       ESCAPE_RET_NULL();
                    }
               }
             else
               {
                  ESCAPE_RET_NULL();
               }
          }

        //compatibility: "part"
        if (!strncmp("part", p, strlen("part")))
          {
             p = strstr((p + 4), "\"");
             if (!p) return NULL;
             p++;
             if (!strncmp(part_name, p, strlen(part_name)))
               {
                  //Compare Elaborately
                  char *next_quote = strstr(p, "\"");
                  if (part_name_len == (next_quote - p))
                    {
                       result = p;
                       break;
                    }
                  else
                    {
                       ESCAPE_RET_NULL();
                    }
               }
             else
               {
                  ESCAPE_RET_NULL();
               }
          }

        p++;
     }

   return result;
}

static void
find_part_proc(const char *group_name, const char *part_name,
               const char *part_type)
{
   if (!group_name || !part_name) return;

   const char *text = enventor_item_text_get(file_mgr_focused_item_get());
   if (!text) return;

   char *utf8 = elm_entry_markup_to_utf8(text);
   char *utf8_end = utf8 + strlen(utf8);

   const char *result = find_part_proc_internal(utf8, utf8_end, group_name,
                                                part_name, part_type);
   if (!result) goto end;

   //Got you!
   enventor_item_select_region_set(file_mgr_focused_item_get(),
                                   (result - utf8),
                                   (result - utf8) + strlen(part_name));
end:
   free(utf8);
}

static void
find_state_proc(const char *group_name, const char *part_name,
                const char *part_type, const char *state_name)
{
   if (!group_name || !part_name) return;

   const char *text = enventor_item_text_get(file_mgr_focused_item_get());
   if (!text) return;

   char *utf8 = elm_entry_markup_to_utf8(text);
   char *utf8_end = utf8 + strlen(utf8);

   char *p = find_part_proc_internal(utf8, utf8_end, group_name,
                                     part_name, part_type);
   if (!p) goto end;

   p = strstr(p, "\"");
   if (!p) goto end;
   p++;

   char *result = NULL;

   int state_name_len = strlen(state_name);

   //Find programs
   while (utf8_end > p)
     {
        //Skip " ~ " Section
        if (*p == '\"')
          {
             ESCAPE_GOTO_END();
          }

        if (!strncmp("desc", p, strlen("desc")))
          {
             p = strstr((p + 4), "\"");
             if (!p) goto end;
             p++;
             if (!strncmp(state_name, p, state_name_len))
               {
                  //Compare Elaborately
                  char *next_quote = strstr(p, "\"");
                  if (state_name_len == (next_quote - p))
                    {
                       result = p;
                       break;
                    }
                  else
                    {
                       ESCAPE_GOTO_END();
                    }
               }
             else
               {
                  ESCAPE_GOTO_END();
               }
          }
        p++;
     }

   if (!result) goto end;

   //Got you!
   enventor_item_select_region_set(file_mgr_focused_item_get(),
                                   (result - utf8),
                                   (result - utf8) + strlen(state_name));
end:
   free(utf8);
}

static char*
find_programs_proc_internal(char *utf8, char *utf8_end, const char *group_name)
{
   char *p = find_group_proc_internal(utf8, utf8_end, group_name);
   if (!p) return NULL;

   p = strstr(p, "\"");
   if (!p) return NULL;
   p++;

   char *result = NULL;

   //Find programs
   while (utf8_end > p)
     {
        //Skip " ~ " Section
        if (*p == '\"')
          {
             ESCAPE_RET_NULL();
          }

        if (!strncmp("programs", p, strlen("programs")))
          {
             result = p;
             break;
          }
        p++;
     }

   return result;
}

static void
find_programs_proc(const char *group_name)
{
   if (!group_name) return;

   const char *text = enventor_item_text_get(file_mgr_focused_item_get());
   if (!text) return;

   char *utf8 = elm_entry_markup_to_utf8(text);
   char *utf8_end = utf8 + strlen(utf8);

   char *result = find_programs_proc_internal(utf8, utf8_end, group_name);

   //No found
   if (!result) goto end;

   //Got you!
   enventor_item_select_region_set(file_mgr_focused_item_get(),
                                   (result - utf8),
                                   (result - utf8) + strlen("programs"));
end:
   free(utf8);
}

static void
find_program_proc(const char *group_name, const char *program_name)
{
   if (!group_name || !program_name) return;

   const char *text = enventor_item_text_get(file_mgr_focused_item_get());
   if (!text) return;

   char *utf8 = elm_entry_markup_to_utf8(text);
   char *utf8_end = utf8 + strlen(utf8);
   char *p = find_programs_proc_internal(utf8, utf8_end, group_name);
   if (!p) goto end;

   char *result = NULL;

   p += strlen("programs");

   int program_name_len = strlen(program_name);

   //Find program
   while (utf8_end > p)
     {
        //Skip " ~ " Section
        if (*p == '\"')
          {
             ESCAPE_GOTO_END();
          }

        if (!strncmp("program", p, strlen("program")))
          {
             p = strstr((p + 6), "\"");
             if (!p) goto end;
             p++;
             if (!strncmp(program_name, p, program_name_len))
               {
                  //Compare Elaborately
                  char *next_quote = strstr(p, "\"");
                  if (program_name_len == (next_quote - p))
                    {
                       result = p;
                       break;
                    }
                  else
                    {
                       ESCAPE_GOTO_END();
                    }
               }
             else
               {
                  ESCAPE_GOTO_END();
               }
          }
        p++;
     }

   //No found
   if (!result) goto end;

   //Got you!
   enventor_item_select_region_set(file_mgr_focused_item_get(),
                                   (result - utf8),
                                   (result - utf8) + strlen(program_name));
end:
   free(utf8);
}

/* State Related */

static char *
gl_state_text_get_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     const char *part EINA_UNUSED)
{
   state_it *sit = data;
   return strdup(sit->name);
}

static void
gl_state_selected_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   state_it *sit = data;
   find_state_proc(sit->pit->git->name, sit->pit->name, part_type_get(sit->pit),
                   sit->name);
}

static void
states_update(navi_data *nd, part_it *pit)
{
   Enventor_Object *enventor = base_enventor_get();
   Eina_List *state_list = enventor_object_part_states_list_get(enventor,
                                                                pit->name);
   char *name;
   Eina_List *l, *ll;
   state_it *sit;
   int idx = 0;

   if (!state_list)
     {
        navigator_states_clear(pit);
        return;
     }

   //1. Prepare for validation.
   EINA_LIST_FOREACH(pit->states, l, sit)
     {
        sit->discarded = EINA_TRUE;
        sit->tag.idx = IDX_MAX;
     }

   //2. New States
   EINA_LIST_FOREACH(state_list, l, name)
     {
        Eina_Bool new_state = EINA_TRUE;
        idx++;

        //Check if it is existed?
        EINA_LIST_FOREACH(pit->states, ll, sit)
          {
             if (!strcmp(name, sit->name) &&
                 (strlen(name) == strlen(sit->name)))
               {
                  sit->discarded = EINA_FALSE;
                  new_state = EINA_FALSE;
                  //update index of the item
                  sit->tag.idx = idx;
                  break;
               }

          }
        if (!new_state) continue;

        //Ok, this state is newly added
        sit = calloc(1, sizeof(state_it));
        if (!sit)
          {
             EINA_LOG_ERR(_("Failed to allocate Memory!"));
             continue;
          }
        sit->tag.type = Item_Type_State;
        sit->tag.idx = idx;
        //Parsing "default" "0.00". We don't take care 0.00 in the state name.
        const char *brk = strpbrk(name, " ");
        if (brk) sit->name = strndup(name, brk - name);
        else sit->name = strdup(name);

        sit->pit = pit;
        sit->it = elm_genlist_item_sorted_insert(nd->genlist,
                                                 nd->state_itc,
                                                 sit,
                                                 pit->it,
                                                 ELM_GENLIST_ITEM_NONE,
                                                 gl_comp_func,
                                                 gl_state_selected_cb,
                                                 sit);
        pit->states = eina_list_append(pit->states, sit);
     }

   //3. Update states
   EINA_LIST_FOREACH_SAFE(pit->states, l, ll, sit)
     {
        //Remove them from the previous list.
        if (sit->discarded)
          {
             pit->states = eina_list_remove_list(pit->states, l);
             navigator_state_free(sit);
             continue;
          }
     }

   edje_edit_string_list_free(state_list);
}

static Evas_Object *
gl_state_content_get_cb(void *data EINA_UNUSED, Evas_Object *obj,
                        const char *part)
{
   if (strcmp("elm.swallow.icon", part)) return NULL;

   Evas_Object *image = elm_image_add(obj);
   elm_image_file_set(image, EDJE_PATH, "navi_state");

   return image;
}

/* Program Related */

static void
program_btn_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   program_it *pit = data;
   enventor_object_program_run(base_enventor_get(), pit->name);

   if (!config_stats_bar_get()) return;

   char buf[256];
   snprintf(buf, sizeof(buf),_("Program Run: \"%s\""), pit->name);
   stats_info_msg_update(buf);
}

static char *
gl_program_text_get_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       const char *part EINA_UNUSED)
{
   program_it *spit = data;
   return strdup(spit->name);
}

static Evas_Object *
gl_program_content_get_cb(void *data EINA_UNUSED, Evas_Object *obj,
                          const char *part)
{
   //1. Icon
   if (!strcmp("elm.swallow.icon", part))
     {
        Evas_Object *image = elm_image_add(obj);
        elm_image_file_set(image, EDJE_PATH, "navi_state");
        return image;
     }

   //2. Play Button
   program_it *pit = data;

   //Box
   Evas_Object *box = elm_box_add(obj);
   elm_object_tooltip_text_set(box, "Play the program.");

   //Button
   Evas_Object *btn = elm_button_add(box);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_scale_set(btn, 0.5);
   evas_object_smart_callback_add(btn, "clicked", program_btn_clicked_cb,
                                  pit);
   evas_object_show(btn);

   //Image
   Evas_Object *img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, "navi_play");
   elm_object_content_set(btn, img);

   elm_box_pack_end(box, btn);

   return box;
}

static void
gl_program_selected_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   program_it *spit = data;
   find_program_proc(spit->pit->git->name, spit->name);
}

static void
sub_programs_update(navi_data *nd, programs_it *pit)
{
   navigator_sub_programs_clear(pit);

   Enventor_Object *enventor = base_enventor_get();
   Eina_List *program_list = enventor_object_programs_list_get(enventor);
   if (!program_list) return;

   //Append program list
   char *name;
   Eina_List *l;
   program_it *spit;
   int idx = 0;

   EINA_LIST_FOREACH(program_list, l, name)
     {
        idx++;

        spit = calloc(1, sizeof(program_it));
        if (!spit)
          {
             EINA_LOG_ERR(_("Failed to allocate Memory!"));
             continue;
          }

        spit->tag.type = Item_Type_Program;
        spit->tag.idx = idx;
        spit->name = strdup(name);
        spit->pit = pit;
        spit->it = elm_genlist_item_sorted_insert(nd->genlist,
                                                  nd->program_itc,
                                                  spit,
                                                  pit->it,
                                                  ELM_GENLIST_ITEM_NONE,
                                                  gl_comp_func,
                                                  gl_program_selected_cb,
                                                  spit);
        pit->programs = eina_list_append(pit->programs, spit);
     }

   edje_edit_string_list_free(program_list);
}

/* Programs Related */

static void
programs_btn_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                        void *event_info EINA_UNUSED)
{
   enventor_object_programs_stop(base_enventor_get());

   if (!config_stats_bar_get()) return;

   stats_info_msg_update(_("Stop all running programs."));
}

static void
programs_expand(programs_it *pit)
{
   if (elm_genlist_item_expanded_get(pit->it)) return;

   elm_genlist_item_expanded_set(pit->it, EINA_TRUE);

   sub_programs_update(pit->git->nd, pit);
}

static void
programs_contract(programs_it *pit)
{
   if (!elm_genlist_item_expanded_get(pit->it)) return;

   elm_genlist_item_expanded_set(pit->it, EINA_FALSE);

   navigator_sub_programs_clear(pit);
}

static void
gl_programs_selected_cb(void *data, Evas_Object *obj EINA_UNUSED,
                        void *event_info EINA_UNUSED)
{
   programs_it *pit = data;

   find_programs_proc(pit->git->name);
}

static void
programs_update(navi_data *nd, group_it *git)
{
   Enventor_Object *enventor = base_enventor_get();
   Eina_List *program_list = enventor_object_programs_list_get(enventor);

   //oh, no programs.. 
   if (!program_list)
     {
        navigator_programs_clear(git);
        return;
     }

   programs_it *pit = &git->programs;

   //Create a programs item first time.
   if (!pit->it)
     {
        pit->tag.type = Item_Type_Programs;
        pit->tag.idx = PROGRAM_IDX;
        pit->git = git;
        pit->it = elm_genlist_item_append(nd->genlist,
                                          nd->programs_itc,
                                          pit,
                                          git->it,
                                          ELM_GENLIST_ITEM_TREE,
                                          gl_programs_selected_cb,
                                          pit);
        return;
     }

   if (!elm_genlist_item_expanded_get(pit->it)) return;

   //programs item is already created, it may need to update progam lists.

   //1. Prepare for validation.
   Eina_List *l, *ll;
   program_it *spit;
   char *name;
   int idx = 0;

   EINA_LIST_FOREACH(pit->programs, l, spit)
     {
        spit->discarded = EINA_TRUE;
        spit->tag.idx = IDX_MAX;
     }

   //2. New programs
   EINA_LIST_FOREACH(program_list, l, name)
     {
        Eina_Bool new_program = EINA_TRUE;
        idx++;

        //Check if it is existed?
        EINA_LIST_FOREACH(pit->programs, ll, spit)
          {
             if (!strcmp(name, spit->name) &&
                 (strlen(name) == strlen(spit->name)))
               {
                  spit->discarded = EINA_FALSE;
                  new_program = EINA_FALSE;
                  //update index of the item
                  spit->tag.idx = idx;
                  break;
               }
          }
        if (!new_program) continue;

        //Ok, this program is newly added.
        spit = calloc(1, sizeof(program_it));
        if (!spit)
          {
             EINA_LOG_ERR(_("Failed to allocate Memory!"));
             continue;
          }

        spit->tag.type = Item_Type_Program;
        spit->tag.idx = idx;
        spit->name = strdup(name);
        spit->pit = pit;
        spit->it = elm_genlist_item_sorted_insert(nd->genlist,
                                                  nd->program_itc,
                                                  spit,
                                                  pit->it,
                                                  ELM_GENLIST_ITEM_NONE,
                                                  gl_comp_func,
                                                  gl_program_selected_cb,
                                                  spit);
        pit->programs = eina_list_append(pit->programs, spit);
     }

   //3. Discarded programs
   EINA_LIST_FOREACH_SAFE(pit->programs, l, ll, spit)
     {
        if (!spit->discarded) continue;

        //Remove them from the previous list.
        pit->programs = eina_list_remove_list(pit->programs, l);
        navigator_program_free(spit);
     }

   edje_edit_string_list_free(program_list);
}

static char *
gl_programs_text_get_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                        const char *part EINA_UNUSED)
{
   return strdup("PROGRAMS");
}

static Evas_Object *
gl_programs_content_get_cb(void *data EINA_UNUSED, Evas_Object *obj,
                           const char *part)
{
   //1. Icon
   if (!strcmp("elm.swallow.icon", part))
     {
        Evas_Object *image = elm_image_add(obj);
        elm_image_file_set(image, EDJE_PATH, "navi_program");

        return image;
     }

   //2. Stop All Button
   //Box
   Evas_Object *box = elm_box_add(obj);
   elm_object_tooltip_text_set(box, "Stop all programs.");

   //Button
   Evas_Object *btn = elm_button_add(box);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_scale_set(btn, 0.5);
   evas_object_smart_callback_add(btn, "clicked", programs_btn_clicked_cb,
                                  NULL);
   evas_object_show(btn);

   //Image
   Evas_Object *img = elm_image_add(btn);
   elm_image_file_set(img, EDJE_PATH, "navi_stop");
   elm_object_content_set(btn, img);

   elm_box_pack_end(box, btn);

   return box;
}


/* Part Related */

static char *
gl_part_text_get_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    const char *part EINA_UNUSED)
{
   part_it *pit = data;
   return strdup(pit->name);
}

static Evas_Object *
gl_part_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
   if (strcmp("elm.swallow.icon", part)) return NULL;
   part_it *pit = data;

   Evas_Object *image = elm_image_add(obj);
   const char *group;

   switch (pit->type)
     {
      case EDJE_PART_TYPE_RECTANGLE:
         group = "navi_rect";
         break;
      case EDJE_PART_TYPE_TEXT:
         group = "navi_text";
         break;
      case EDJE_PART_TYPE_IMAGE:
         group = "navi_image";
         break;
      case EDJE_PART_TYPE_SWALLOW:
         group = "navi_swallow";
         break;
      case EDJE_PART_TYPE_TEXTBLOCK:
         group = "navi_textblock";
         break;
      case EDJE_PART_TYPE_SPACER:
         group = "navi_spacer";
         break;
      default:
         group = "navi_unknown";
         break;
     }

   elm_image_file_set(image, EDJE_PATH, group);

   return image;
}

static void
gl_part_selected_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   part_it *pit = data;

   //Find text cursor position
   const char *part_type = part_type_get(pit);
   find_part_proc(pit->git->name, pit->name, part_type);
}

static void
parts_update(navi_data *nd, group_it *git)
{
   Enventor_Object *enventor = base_enventor_get();
   Eina_List *part_list = enventor_object_parts_list_get(enventor);
   Eina_List *l, *ll;
   part_it *pit;
   char *name;
   int idx = 0;

   //1. Prepare for validation.
   EINA_LIST_FOREACH(git->parts, l, pit)
     {
        pit->discarded = EINA_TRUE;
        pit->tag.idx = IDX_MAX;
     }

   //2. New parts
   EINA_LIST_FOREACH(part_list, l, name)
     {
        Eina_Bool new_part = EINA_TRUE;
        idx++;

        //Check if it is existed?
        EINA_LIST_FOREACH(git->parts, ll, pit)
          {
             if (!strcmp(name, pit->name) &&
                 (strlen(name) == strlen(pit->name)))
               {
                  pit->discarded = EINA_FALSE;
                  new_part = EINA_FALSE;
                  //update index of the item
                  pit->tag.idx = idx;
                  break;
               }

          }
        if (!new_part) continue;

        //Ok, this part is newly added.
        pit = calloc(1, sizeof(part_it));
        if (!pit)
          {
             EINA_LOG_ERR(_("Failed to allocate Memory!"));
             continue;
          }

        pit->tag.type = Item_Type_Part;
        pit->tag.idx = idx;
        pit->name = strdup(name);
        pit->type = enventor_object_part_type_get(enventor, name);
        pit->git = git;
        pit->it = elm_genlist_item_sorted_insert(nd->genlist,
                                                 nd->part_itc,
                                                 pit,
                                                 git->it,
                                                 ELM_GENLIST_ITEM_TREE,
                                                 gl_comp_func,
                                                 gl_part_selected_cb,
                                                 pit);
        git->parts = eina_list_append(git->parts, pit);
     }

   //3. Update parts
   EINA_LIST_FOREACH_SAFE(git->parts, l, ll, pit)
     {
        //Remove them from the previous list.
        if (pit->discarded)
          {
             git->parts = eina_list_remove_list(git->parts, l);
             navigator_part_free(pit);
             continue;
          }

        //Update parts states only if they are expanded.
        if (!elm_genlist_item_expanded_get(pit->it)) continue;

        states_update(nd, pit);
     }

   edje_edit_string_list_free(part_list);
}

static void
part_expand(part_it *pit)
{
   if (elm_genlist_item_expanded_get(pit->it)) return;

   elm_genlist_item_expanded_set(pit->it, EINA_TRUE);

   states_update(pit->git->nd, pit);
}

static void
part_contract(part_it *pit)
{
   if (!elm_genlist_item_expanded_get(pit->it)) return;

   elm_genlist_item_expanded_set(pit->it, EINA_FALSE);

   navigator_states_clear(pit);
}

/* Group Related */

static void
group_update(navi_data *nd, group_it *git)
{
   parts_update(nd, git);
   programs_update(nd, git);
}

static void
group_expand(group_it *git)
{
   if (elm_genlist_item_expanded_get(git->it)) return;

   elm_genlist_item_expanded_set(git->it, EINA_TRUE);

   group_update(git->nd, git);
}

static void
group_contract(group_it *git)
{
   if (!elm_genlist_item_expanded_get(git->it)) return;

   elm_genlist_item_expanded_set(git->it, EINA_FALSE);
   navigator_parts_clear(git);
   navigator_programs_clear(git);
}

static char *
gl_group_text_get_cb(void *data, Evas_Object *obj EINA_UNUSED,
                    const char *part EINA_UNUSED)
{
   group_it *git = data;
   return strdup(git->name);
}

static Evas_Object *
gl_group_content_get_cb(void *data EINA_UNUSED, Evas_Object *obj,
                        const char *part)
{
   if (!strcmp("elm.swallow.icon", part))
     {
        Evas_Object *image = elm_image_add(obj);
        elm_image_file_set(image, EDJE_PATH, "navi_group");
        return image;
     }

   return NULL;
}

static void
gl_group_selected_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   group_it *git = data;
   find_group_proc(git->name);
}

static void
gl_expand_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                     void *event_info)
{
   Elm_Object_Item *it = event_info;
   list_it *_it = (list_it *)elm_object_item_data_get(it);

   switch(_it->type)
     {
      case Item_Type_Group:
         group_expand((group_it*)_it);
         break;
      case Item_Type_Part:
         part_expand((part_it*)_it);
         break;
      case Item_Type_Programs:
         programs_expand((programs_it*)_it);
         break;
      default:
         break;
     }
}

static void
gl_contract_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                       void *event_info)
{
   Elm_Object_Item *it = event_info;
   list_it *_it = (list_it *)elm_object_item_data_get(it);

   switch(_it->type)
     {
      case Item_Type_Group:
         group_contract((group_it*)_it);
         break;
      case Item_Type_Part:
         part_contract((part_it*)_it);
         break;
      case Item_Type_Programs:
         programs_contract((programs_it*)_it);
         break;
      default:
         break;
     }
}

/*****************************************************************************/
/* Externally accessible calls                                               */
/*****************************************************************************/
void
edc_navigator_deselect(void)
{
   navi_data *nd = g_nd;
   EINA_SAFETY_ON_NULL_RETURN(nd);

   if (!nd->selected) return;
   navigator_item_deselect(nd);
}

void
edc_navigator_group_update(const char *cur_group)
{
   navi_data *nd = g_nd;
   EINA_SAFETY_ON_NULL_RETURN(nd);

   //FIXME: This function is unnecessarily called... why?

   //Cancel item selection if group was not indicated. 
   if (!cur_group) navigator_item_deselect(nd);
   Eina_List *group_list =
      enventor_item_group_list_get(file_mgr_focused_item_get());

   unsigned int cur_group_len = 0;
   group_it *git;
   Eina_List *l, *ll;
   char *name;
   int idx = 0;

   //1. Prepare for validation.
   EINA_LIST_FOREACH(nd->groups, l, git)
     {
        git->discarded = EINA_TRUE;
        git->tag.idx = IDX_MAX;
     }

   //2. New groups
   EINA_LIST_FOREACH(group_list, l, name)
     {
        idx++;

        Eina_Bool new_group = EINA_TRUE;

        //Check if it is existed?
        EINA_LIST_FOREACH(nd->groups, ll, git)
          {
             if (!strcmp(name, git->name) &&
                 (strlen(name) == strlen(git->name)))
               {
                  git->discarded = EINA_FALSE;
                  new_group = EINA_FALSE;
                  //update index of the item
                  git->tag.idx = idx;
                  break;
               }
          }
        if (!new_group) continue;

        //Ok, this group is newly added.
        git = calloc(1, sizeof(group_it));
        if (!git)
          {
             EINA_LOG_ERR(_("Failed to allocate Memory!"));
             continue;
          }

        git->tag.type = Item_Type_Group;
        git->tag.idx = idx;
        git->name = strdup(name);
        git->nd = nd;
        git->it = elm_genlist_item_append(nd->genlist,
                                          nd->group_itc,
                                          git,
                                          NULL,
                                          ELM_GENLIST_ITEM_TREE,
                                          gl_group_selected_cb,
                                          git);
        nd->groups = eina_list_append(nd->groups, git);
     }

   //3. Update groups
   if (cur_group) cur_group_len = strlen(cur_group);

   EINA_LIST_FOREACH_SAFE(nd->groups, l, ll, git)
     {
        //remove them from the previous list.
        if (git->discarded)
          {
             nd->groups = eina_list_remove_list(nd->groups, l);
             navigator_group_free(git);
             continue;
          }

        //Update group parts only if they are expanded.
        if (!elm_genlist_item_expanded_get(git->it)) continue;

        //update only current group
        if (cur_group && !strcmp(cur_group, git->name) &&
            (cur_group_len == strlen(git->name)))
            group_update(nd, git);
     }

   EINA_LIST_FREE(group_list, name) eina_stringshare_del(name);
}

Evas_Object *
edc_navigator_init(Evas_Object *parent)
{
   navi_data *nd = calloc(1, sizeof(navi_data));
   if (!nd)
     {
        EINA_LOG_ERR("Failed to allocate Memory!");
        return NULL;
     }
   g_nd = nd;

   //Base layout
   Evas_Object *base_layout = elm_layout_add(parent);
   elm_layout_file_set(base_layout, EDJE_PATH, "tools_layout");
   evas_object_size_hint_weight_set(base_layout, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(base_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(base_layout);

   //Genlist
   Evas_Object *genlist = elm_genlist_add(parent);
   elm_object_focus_allow_set(genlist, EINA_FALSE);
   evas_object_smart_callback_add(genlist, "expand,request",
                                  gl_expand_request_cb, nd);
   evas_object_smart_callback_add(genlist, "contract,request",
                                  gl_contract_request_cb, nd);
   evas_object_smart_callback_add(genlist, "selected",
                                  gl_selected_cb, nd);
   evas_object_smart_callback_add(genlist, "selected",
                                  gl_unselected_cb, nd);
   evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(base_layout, genlist);

   //Group Item Class
   Elm_Genlist_Item_Class *itc;

   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_group_text_get_cb;
   itc->func.content_get = gl_group_content_get_cb;

   nd->group_itc = itc;

   //Part Item Class
   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_part_text_get_cb;
   itc->func.content_get = gl_part_content_get_cb;

   nd->part_itc = itc;

   //State Item Class
   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_state_text_get_cb;
   itc->func.content_get = gl_state_content_get_cb;

   nd->state_itc = itc;

   //Programs Item Class
   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_programs_text_get_cb;
   itc->func.content_get = gl_programs_content_get_cb;

   nd->programs_itc = itc;

   //Program Item Class
   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = gl_program_text_get_cb;
   itc->func.content_get = gl_program_content_get_cb;

   nd->program_itc = itc;

   nd->base_layout = base_layout;
   nd->genlist = genlist;

   return base_layout;
}

void
edc_navigator_term(void)
{
   navi_data *nd = g_nd;
   EINA_SAFETY_ON_NULL_RETURN(nd);

   navigator_groups_clear(nd);

   elm_genlist_item_class_free(nd->group_itc);
   elm_genlist_item_class_free(nd->part_itc);
   elm_genlist_item_class_free(nd->state_itc);
   elm_genlist_item_class_free(nd->programs_itc);
   elm_genlist_item_class_free(nd->program_itc);

   evas_object_del(nd->base_layout);

   free(nd);
   g_nd = NULL;
}

void
edc_navigator_tools_set(void)
{
   navi_data *nd = g_nd;
   EINA_SAFETY_ON_NULL_RETURN(nd);

   Evas_Object *rect =
      evas_object_rectangle_add(evas_object_evas_get(nd->base_layout));
   evas_object_color_set(rect, 0, 0, 0, 0);
   elm_object_part_content_set(nd->base_layout, "elm.swallow.tools", rect);
}

void
edc_navigator_tools_visible_set(Eina_Bool visible)
{
   navi_data *nd = g_nd;
   EINA_SAFETY_ON_NULL_RETURN(nd);

   if (visible)
     elm_object_signal_emit(nd->base_layout, "elm,state,tools,show", "");
   else
     elm_object_signal_emit(nd->base_layout, "elm,state,tools,hide", "");
}
