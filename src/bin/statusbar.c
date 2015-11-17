#include "common.h"

typedef struct statusbar_s
{
   Evas_Object *layout;
   Eina_Stringshare *group_name;
   int cur_line;
   int max_line;
} stats_data;

stats_data *g_sd = NULL;

void
stats_line_num_update(int cur_line, int max_line)
{
   stats_data *sd = g_sd;

   char buf[20];
   snprintf(buf, sizeof(buf), "%d", cur_line);
   elm_object_part_text_set(sd->layout, "elm.text.line_cur", buf);
   snprintf(buf, sizeof(buf), "%d", max_line);
   elm_object_part_text_set(sd->layout, "elm.text.line_max", buf);

   sd->cur_line = cur_line;
   sd->max_line = max_line;
}

void
stats_edc_group_update(Eina_Stringshare *group_name)
{
   stats_data *sd = g_sd;
   elm_object_part_text_set(sd->layout, "elm.text.group_name", group_name);
   sd->group_name = eina_stringshare_add(group_name);
}

Evas_Object *
stats_init(Evas_Object *parent)
{
   stats_data *sd = calloc(1, sizeof(stats_data));
   if (!sd)
     {
        EINA_LOG_ERR(_("Failed to allocate Memory!"));
        return NULL;
     }
   g_sd = sd;

   Evas_Object *layout = elm_layout_add(parent);
   elm_layout_file_set(layout, EDJE_PATH, "statusbar_layout");

   sd->layout = layout;

   stats_cursor_pos_update(0, 0, 0, 0);
   stats_edc_group_update(NULL);

   return layout;
}

Eina_Stringshare *stats_group_name_get(void)
{
   stats_data *sd = g_sd;
   return sd->group_name;
}

void
stats_term(void)
{
   stats_data *sd = g_sd;
   eina_stringshare_del(sd->group_name);
   free(sd);
}

void
stats_info_msg_update(const char *msg)
{
   if (!config_stats_bar_get()) return;

   stats_data *sd = g_sd;
   elm_object_part_text_set(sd->layout, "elm.text.info_msg", msg);
   elm_object_signal_emit(sd->layout, "elm,action,info_msg,show", "");
}

void
stats_view_size_update(Evas_Coord w, Evas_Coord h)
{
   stats_data *sd = g_sd;

   char buf[10];
   snprintf(buf, sizeof(buf), "%d", w);
   elm_object_part_text_set(sd->layout, "elm.text.size_w", buf);
   snprintf(buf, sizeof(buf), "%d", h);
   elm_object_part_text_set(sd->layout, "elm.text.size_h", buf);
}

void
stats_cursor_pos_update(Evas_Coord x, Evas_Coord y, float rel_x, float rel_y)
{
   stats_data *sd = g_sd;

   char buf[10];
   snprintf(buf, sizeof(buf), "%d", x);
   elm_object_part_text_set(sd->layout, "elm.text.cursor_pxx", buf);
   snprintf(buf, sizeof(buf), "%d", y);
   elm_object_part_text_set(sd->layout, "elm.text.cursor_pxy", buf);

   snprintf(buf, sizeof(buf), "%0.2f", rel_x);
   elm_object_part_text_set(sd->layout, "elm.text.cursor_relx", buf);
   snprintf(buf, sizeof(buf), "%0.2f", rel_y);
   elm_object_part_text_set(sd->layout, "elm.text.cursor_rely", buf);
}
